#include <dynarec/compiler.hpp>
#include <dynarec/core_dynarec.hpp>
#include <dynarec/orcexecutor.hpp>
#include <analysis/repository.hpp>
#include <dynarec/function.hpp>

// Called by guest code to read memory.
static uint8_t guestMemoryRead(Cpu::Memory *memory, uint16_t address) {
  return memory->read(address);
}

// Called by guest code to read 16-bits of memory at once.
static uint16_t guestMemoryRead16(Cpu::Memory *memory, uint16_t address) {
  return memory->read16(address);
}

// Called by guest code to write memory.
static void guestMemoryWrite(Cpu::Memory *memory, uint16_t address, uint8_t value) {
  return memory->write(address, value);
}

// Called by guest code to trace instructions.
static void guestTrace(const char *message) {
  fprintf(stderr, "; %s\n", message);
}

// Called by guest code to trace instructions verbosely.
static void guestTraceVerbose(const char *message, Cpu::State &state) {
  Cpu::Flags f = state.flags();
  fprintf(stderr, "; %s  A %02x X %02x Y %02x S %02x [%c%c%c%c%c%c%c]\n",
          message,
          state.a, state.x, state.y, state.s,
          (f.testFlag(Cpu::Flag::Carry) ? 'C' : 'c'),
          (f.testFlag(Cpu::Flag::Zero) ? 'Z' : 'z'),
          (f.testFlag(Cpu::Flag::Interrupt) ? 'I' : 'i'),
          (f.testFlag(Cpu::Flag::Decimal) ? 'D' : 'd'),
          (f.testFlag(Cpu::Flag::Break) ? 'B' : 'b'),
          (f.testFlag(Cpu::Flag::Overflow) ? 'V' : 'v'),
          (f.testFlag(Cpu::Flag::Negative) ? 'N' : 'n')
          );
}

static void guestDebug(const char *message, uint8_t value) {
  fprintf(stderr, "        +- %s %02x\n", message, value);
}

namespace Dynarec {
struct CorePrivate {
  Core *core;
  Compiler compiler;
  OrcExecutor executor;
  Analysis::Repository<Function> repository;

  CorePrivate(const Cpu::Memory::Ptr &mem, Core *c)
    : core(c), repository(mem, [](Analysis::Function &base){ return new Function(base); })
  {
    this->compiler.addVariable("memory", mem.get());
    this->compiler.addVariable("ram", mem->ram());

    llvm::LLVMContext &ctx = this->compiler.context();
    llvm::Type *voidTy = llvm::Type::getVoidTy(ctx);
    llvm::Type *voidPtrTy = llvm::PointerType::get(voidTy, 0);
    llvm::Type *int8Ty = llvm::Type::getInt8Ty(ctx);
    llvm::Type *int8PtrTy = llvm::PointerType::get(int8Ty, 0);
    llvm::Type *int16Ty = llvm::Type::getInt16Ty(ctx);
    llvm::Type *statePtrTy = llvm::PointerType::get(this->compiler.stateType(), 0);

    this->compiler.addFunction("mem.read", reinterpret_cast<void *>(&guestMemoryRead),
                               llvm::FunctionType::get(int8Ty, { voidPtrTy, int16Ty }, false));

    this->compiler.addFunction("mem.read16", reinterpret_cast<void *>(&guestMemoryRead16),
                               llvm::FunctionType::get(int16Ty, { voidPtrTy, int16Ty }, false));

    this->compiler.addFunction("mem.write", reinterpret_cast<void *>(&guestMemoryWrite),
                               llvm::FunctionType::get(voidTy, { voidPtrTy, int16Ty, int8Ty }, false));

    this->compiler.addFunction("trace", reinterpret_cast<void *>(&guestTrace),
                               llvm::FunctionType::get(voidTy, { int8PtrTy }, false));

    this->compiler.addFunction("trace.verbose", reinterpret_cast<void *>(&guestTraceVerbose),
                               llvm::FunctionType::get(voidTy, { int8PtrTy, statePtrTy }, false));

    this->compiler.addFunction("debug", reinterpret_cast<void *>(&guestDebug),
                               llvm::FunctionType::get(voidTy, { int8PtrTy, int8Ty}, false));
  }

  void callFunction(Cpu::State &state) {
    using Cpu::State;

    bool running = true;
    while (running && state.cycles > 0) {
      this->callFunctionOnce(state);

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

  void callFunctionOnce(Cpu::State &state) {
    Function *function = this->repository.get(state.pc);

    // Compile this function if it hasn't already.
    llvm::Function *llvmFunc = function->compiledFunction();
    if (!llvmFunc) llvmFunc = this->compiler.compile(function);

    // Call guest code.
    this->executor.callFunction(function, state);

    if (!function->analyzed().cacheable()) delete function;
  }
};

Core::Core(const Cpu::Memory::Ptr &mem, Cpu::State state, QObject *parent)
  : Base(mem, state, parent), d(new CorePrivate(mem, this))
{
}

Core::~Core() {
  delete this->d;
}

int Core::run(int cycles) {
  this->m_state.cycles = cycles;
  this->d->callFunction(this->m_state);
  return this->m_state.cycles;
}

void Core::jump(uint16_t address) {
  this->m_state.pc = address;
}
}
