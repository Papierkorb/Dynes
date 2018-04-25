#include <amd64/core_amd64.hpp>

#include <amd64/functiontranslator.hpp>
#include <amd64/memorymanager.hpp>
#include <amd64/function.hpp>
#include <amd64/symbolregistry.hpp>

#include <analysis/repository.hpp>
#include <core/disassembler.hpp>
#include <cpu/state.hpp>

#include <interpret/core_interpret.hpp>
#include <functional>

namespace {
extern "C" {
uint8_t memRead(Cpu::Memory *mem, uint16_t addr) {
  return mem->read(addr);
}

uint16_t memRead16(Cpu::Memory *mem, uint16_t addr) {
  return mem->read16(addr);
}

void memWrite(Cpu::Memory *mem, uint16_t addr, uint8_t value) {
  mem->write(addr, value);
}

// Implemented in src/amd64/guest_call.s - Trampoline to amd64_core_log_instruction_impl()
void amd64_core_log_instruction(Cpu::State *state, uint8_t, uint8_t, uint16_t);

void amd64_core_log_instruction_impl(Cpu::State *state, uint8_t cmd, uint8_t mode, uint16_t op) {
  ::Core::Instruction instr(::Core::Instruction::Command(cmd), ::Core::Instruction::Addressing(mode), 0, op);

  fprintf(stderr, "[%04x] %s %s", state->pc, instr.commandName(), instr.addressingName());
  switch (instr.operandSize()) {
  case 1:
    fprintf(stderr, " %02x", instr.op8);
    break;
  case 2:
    fprintf(stderr, " %02x %02x", (instr.op16 >> 8) & 0xFF, instr.op16 & 0xFF);
    break;
  }

  auto flags = state->flags();
  fprintf(stderr, "  A %02x X %02x Y %02x S %02x P %02x [%c%c%c%c%c%c%c]\n",
          state->a, state->x, state->y, state->s, state->p,
          (flags.testFlag(Cpu::Flag::Carry) ? 'C' : 'c'),
          (flags.testFlag(Cpu::Flag::Zero) ? 'Z' : 'z'),
          (flags.testFlag(Cpu::Flag::Interrupt) ? 'I' : 'i'),
          (flags.testFlag(Cpu::Flag::Decimal) ? 'D' : 'd'),
          (flags.testFlag(Cpu::Flag::Break) ? 'B' : 'b'),
          (flags.testFlag(Cpu::Flag::Overflow) ? 'V' : 'v'),
          (flags.testFlag(Cpu::Flag::Negative) ? 'N' : 'n')
         );
}
}
}

namespace Amd64 {
struct CoreImpl {
  Core *core;
  Cpu::State &state;
  Analysis::Repository<Function> repository;
  MemoryManager memory;
  SymbolRegistry symbols;

  CoreImpl(Core *q, Cpu::State &s, const Cpu::Memory::Ptr &mem)
    : core(q),
      state(s),
      repository(mem, [this](Analysis::Function &b){ return this->compileAnalyzed(b); })
  {
    this->symbols.add("Memory", mem.get());
    this->symbols.add("Ram", mem->ram());
    this->symbols.add("Stack", mem->ram() + Cpu::STACK_BASE);
    this->symbols.add("State", &s);
    this->symbols.add("read", reinterpret_cast<void *>(&memRead));
    this->symbols.add("read16", reinterpret_cast<void *>(&memRead16));
    this->symbols.add("write", reinterpret_cast<void *>(&memWrite));
    this->symbols.add("log", reinterpret_cast<void *>(&amd64_core_log_instruction));
  }

  ~CoreImpl() {
    this->repository.clear();
  }

  Function *compileAnalyzed(Analysis::Function &base) {
    FunctionTranslator t;

    for (const Analysis::Branch *branch : base.branches())
      t.addBranch(*branch);

    void *execPtr = t.link(base.begin(), this->symbols, this->memory);
    return new Function(base, this->memory, execPtr);
  }

  void run(Cpu::State &state) {
    using Cpu::State;

    bool running = true;
    while (running && state.cycles > 0) {
      Function *func = this->repository.get(state.pc);
      func->call(state);

      if (!func->analyzed().cacheable()) delete func;

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

Core::Core(const Cpu::Memory::Ptr &mem, Cpu::State state, QObject *parent)
  : Base(mem, state, parent)
{
  this->impl = new CoreImpl(this, this->m_state, mem);
}

Core::~Core() {
  delete this->impl;
}

int Core::run(int cycles) {
  Cpu::State &state = this->m_state;

  state.cycles = cycles;
  this->impl->run(state);
  return state.cycles;
}

void Core::jump(uint16_t address) {
  this->m_state.pc = address;
}

}
