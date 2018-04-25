#include <cpu/base.hpp>

#include <interpret/core_interpret.hpp>

#ifdef DYNES_CORE_DYNAREC_LLVM
#include <dynarec/core_dynarec.hpp>
#endif

#ifdef DYNES_CORE_LUA
#include <lua/core_lua.hpp>
#endif

#ifdef DYNES_CORE_DYNAREC_AMD64
#include <amd64/core_amd64.hpp>
#endif

namespace Cpu {
Base::Base(const Memory::Ptr &memory, State state, QObject *parent)
  : QObject(parent), m_hook(nullptr), m_state(state), m_mem(memory)
{
}

Base::~Base() {
  // C++ does the rest.
}

void Base::jumpToVector(Interrupt intr) {
  // An indiret jump, like `JMP (VECTOR)`
  uint16_t indirect = this->m_mem->read16(Cpu::interruptVectorAddress(intr));
  this->jump(indirect);
}

void Base::interrupt(Interrupt intr, bool force) {
  if (!force) {
    if (Cpu::isInterruptMaskable(intr) && this->m_state.flags().testFlag(Flag::Interrupt))
      return; // The interrupt is masked, ignore it.
  }

  uint8_t psw = this->m_state.p | static_cast<uint8_t>(Flag::AlwaysOne);

  if (intr == Break)
    psw |= static_cast<uint8_t>(Flag::Break);
  else
    psw &= ~static_cast<uint8_t>(Flag::Break);

  this->push(uint16_t(this->m_state.pc));
  this->push(psw);

  this->m_state.p |= static_cast<uint8_t>(Flag::Interrupt);
  this->jumpToVector(intr);
}

uint8_t Base::pull() {
  this->m_state.s += sizeof(uint8_t);
  return this->m_mem->read(Cpu::STACK_BASE + this->m_state.s);
}

uint16_t Base::pull16() {
  uint16_t lo = static_cast<uint16_t>(this->pull());
  uint16_t hi = static_cast<uint16_t>(this->pull());

  return (hi << 8) | lo;
}

void Base::push(uint8_t value) {
  this->m_mem->write(Cpu::STACK_BASE + this->m_state.s, value);
  this->m_state.s -= sizeof(uint8_t);
}

void Base::push(uint16_t value) {
  this->push(static_cast<uint8_t>(value >> 8));
  this->push(static_cast<uint8_t>(value));
}

Memory::Ptr &Base::mem() {
  return this->m_mem;
}

Base *Base::createByName(const QString &name, const Memory::Ptr &mem, QObject *parent) {
  if (name == QStringLiteral("interpret")) {
    return new Interpret::Core(mem, State(), parent);
#ifdef DYNES_CORE_DYNAREC_LLVM
  } else if (name == QStringLiteral("dynarec")) {
    return new Dynarec::Core(mem, State(), parent);
#endif
#ifdef DYNES_CORE_LUA
  } else if (name == QStringLiteral("lua")) {
    return new Lua::Core(mem, State(), parent);
#endif
#ifdef DYNES_CORE_DYNAREC_AMD64
  } else if (name == QStringLiteral("amd64")) {
    return new Amd64::Core(mem, State(), parent);
#endif
  } else {
    throw std::runtime_error("Unknown CPU implementation");
  }
}

State &Base::state() {
  return this->m_state;
}

Hook *Base::hook() const {
  return this->m_hook;
}

void Base::setHook(Hook *hook) {
  this->m_hook = hook;
}

QMap<QString, QString> Base::availableImplementations() {
  return {
    { "interpret", tr("Interpret") },
#ifdef DYNES_CORE_DYNAREC_LLVM
    { "dynarec", tr("Dynamically recompiler (LLVM JIT)") },
#endif
#ifdef DYNES_CORE_DYNAREC_AMD64
    { "amd64", tr("Dynamic recompiler (AMD64)") },
#endif
#ifdef DYNES_CORE_LUA
    { "lua", tr("Dynamic transpiler (Lua)") },
#endif
  };
}

}
