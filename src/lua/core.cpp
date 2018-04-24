#include <lua/codegenerator.hpp>
#include <lua/core_lua.hpp>
#include <lua/function.hpp>

#include <analysis/repository.hpp>

#include <lua.hpp>

namespace Lua {
// Lua guest functions

// uint8_t guestRead(Cpu::Memory *mem, uint16_t address);
static int guestRead(lua_State *lua) {
  Cpu::Memory *mem = reinterpret_cast<Cpu::Memory *>(lua_touserdata(lua, lua_upvalueindex(1)));
  lua_Integer address = lua_tointegerx(lua, 1, nullptr);

  lua_Unsigned value = mem->read(static_cast<uint16_t>(address));

  lua_pushinteger(lua, static_cast<lua_Integer>(value));
  return 1;
}

// uint16_t guestRead16(Cpu::Memory *mem, uint16_t address);
static int guestRead16(lua_State *lua) {
  Cpu::Memory *mem = reinterpret_cast<Cpu::Memory *>(lua_touserdata(lua, lua_upvalueindex(1)));
  lua_Integer address = lua_tointegerx(lua, 1, nullptr);

  lua_Unsigned value = mem->read16(static_cast<uint16_t>(address));

  lua_pushinteger(lua, static_cast<lua_Integer>(value));
  return 1;
}

// void guestWrite(Cpu::Memory *mem, uint16_t address, uint8_t value);
static int guestWrite(lua_State *lua) {
  Cpu::Memory *mem = reinterpret_cast<Cpu::Memory *>(lua_touserdata(lua, lua_upvalueindex(1)));
  lua_Integer address = lua_tointegerx(lua, 1, nullptr);
  lua_Integer value = lua_tointegerx(lua, 2, nullptr);

  mem->write(static_cast<uint16_t>(address), static_cast<uint8_t>(value));
  return 0;
}

// void guestLog(string message);
static int guestLog(lua_State *lua) {
  size_t len = 0;
  const char *message = lua_tolstring(lua, 1, &len);

  fwrite(message, len, 1, stderr);
  fwrite("\n", 1, 1, stderr);

  return 0;
}

struct CorePrivate {
  Core *core;
  Analysis::Repository<Function> repository;
  CodeGenerator generator;
  lua_State *lua;

  CorePrivate(const Cpu::Memory::Ptr &mem, Core *parent)
    : core(parent),
      repository(mem, [this](Analysis::Function &b){ return this->compileAnalyzed(b); }),
      generator(CodeGenerator::Lua53)
  {
  }

  ~CorePrivate() {
    // Clear the repository first so that all functions free their resources.
    // Only then close the Lua state to avoid use-after-free.
    this->repository.clear();
    lua_close(this->lua);
  }

  std::string pullError() {
    size_t len = 0;
    const char *ptr = lua_tolstring(this->lua, 1, &len);
    std::string message(ptr, len);

    lua_pop(this->lua, 1);
    return message;
  }

  Function *compileAnalyzed(Analysis::Function &base) {
    std::string code = this->generator.translate(base);

    // Parse the string
    luaL_loadstring(this->lua, code.c_str());
//    fprintf(stderr, "===============================================\n%s", code.c_str());

    // Call it so we get the implementing function onto the stack:
    if (lua_pcallk(this->lua, 0, 1, 0, 0, nullptr) != 0) {
      throw std::runtime_error("Lua compile error: " + this->pullError());
    }

    // The implementation of the 6502 function is now at the top of the stack.
    // Push it into the registry.
    int ref = luaL_ref(this->lua, LUA_REGISTRYINDEX);

    return new Function(base, this->lua, ref);
  }

  void callOnce(Cpu::State &state) {
    // Prototype of the Lua function, as defined in CodeGenerator:
    // { a, x, y, s, p, cycles, pc, reason } func(a, x, y, s, p, cycles);
    Function *func = this->repository.get(state.pc);
    func->pushOntoStack();
    lua_pushinteger(this->lua, state.a);
    lua_pushinteger(this->lua, state.x);
    lua_pushinteger(this->lua, state.y);
    lua_pushinteger(this->lua, state.s);
    lua_pushinteger(this->lua, state.p);
    lua_pushinteger(this->lua, state.cycles);

    // Execute 6502 emulation
    if (lua_pcallk(this->lua, /* nargs = */ 6, /* nresults = */ 8, 0, 0, nullptr) != 0) {
      throw std::runtime_error("Lua call error: " + this->pullError());
    }

    // Retrieve results from the stack
    state.a = static_cast<uint8_t>(lua_tointegerx(this->lua, 1, nullptr));
    state.x = static_cast<uint8_t>(lua_tointegerx(this->lua, 2, nullptr));
    state.y = static_cast<uint8_t>(lua_tointegerx(this->lua, 3, nullptr));
    state.s = static_cast<uint8_t>(lua_tointegerx(this->lua, 4, nullptr));
    state.p = static_cast<uint8_t>(lua_tointegerx(this->lua, 5, nullptr));
    state.cycles = static_cast<int32_t>(lua_tointegerx(this->lua, 6, nullptr));
    state.pc = static_cast<uint16_t>(lua_tointegerx(this->lua, 7, nullptr));
    state.reason = static_cast<Cpu::State::Reason>(lua_tointegerx(this->lua, 8, nullptr));

    // Clear Lua's stack:
    lua_pop(this->lua, 8);

    if (!func->analyzed().cacheable()) delete func;
  }

  void run(Cpu::State &state) {
    using Cpu::State;

    bool running = true;
    while (running && state.cycles > 0) {
      this->callOnce(state);

      switch (state.reason) {
      case State::Reason::Break:
        // Jump to IRQ handler in the BRK context.
        this->core->interrupt(Cpu::Break, true);
        break;
      case State::Reason::CyclesExhausted:
        // This one is already handeled above.  We shouldn't end up in here.
        break;
      case State::Reason::Return:
      case State::Reason::Jump:
        // The guest code wants to jump somewhere.  It already updated the
        // state.pc, so nothing to do here.
        break;
      case State::Reason::InfiniteLoop:
        // We don't have to waste host cycles on infinite loops, just claim that
        // all guest cycles were exhausted and get out.
        state.cycles = 0;
        running = false;
        break;
      case State::Reason::UnknownInstruction:
        throw std::runtime_error("Unknown 6502 instruction encountered");
      }
    }
  }
};

Core::Core(const Cpu::Memory::Ptr &memory, Cpu::State state, QObject *parent)
  : Base(memory, state, parent)
{
  this->d = new CorePrivate(memory, this);
  this->d->lua = luaL_newstate();

  // Push the `memory` into the functions as upvalue
  lua_pushlightuserdata(this->d->lua, memory.get());
  lua_pushcclosure(this->d->lua, &guestRead, 1);
  lua_setglobal(this->d->lua, "read");

  lua_pushlightuserdata(this->d->lua, memory.get());
  lua_pushcclosure(this->d->lua, &guestRead16, 1);
  lua_setglobal(this->d->lua, "read16");

  lua_pushlightuserdata(this->d->lua, memory.get());
  lua_pushcclosure(this->d->lua, &guestWrite, 1);
  lua_setglobal(this->d->lua, "write");

  lua_pushcclosure(this->d->lua, &guestLog, 0);
  lua_setglobal(this->d->lua, "log");
}

Core::~Core() {
  delete this->d;
}

int Core::run(int cycles) {
  Cpu::State &state = this->m_state;

  state.cycles = cycles;
  this->d->run(state);
  return state.cycles;
}

void Core::jump(uint16_t address) {
  this->m_state.pc = address;
}
}
