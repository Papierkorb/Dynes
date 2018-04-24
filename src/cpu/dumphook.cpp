#include <cpu/dumphook.hpp>

#include <cstdio>
#include <cstdlib>

namespace Cpu {
DumpHook::DumpHook() {
}

DumpHook::~DumpHook() {
}

void DumpHook::beforeInstruction(Core::Instruction instruction, State &state) {
  Q_UNUSED(instruction);
  this->m_currentPc = state.pc;
}

void DumpHook::afterInstruction(Core::Instruction instruction, State &state) {
  using Cpu::Flag;

  fprintf(stderr, "[%04x] %s %s",
          this->m_currentPc, instruction.commandName(), instruction.addressingName()
          );

  if (false && instruction.addressing == Core::Instruction::Rel) {
    fprintf(stderr, " %+i => %04x", instruction.ops8, state.pc + instruction.ops8);
  } else {
    switch (instruction.operandSize()) {
    case 1:
      fprintf(stderr, " %02x", instruction.op8);
      break;
    case 2:
      fprintf(stderr, " %02x %02x", instruction.op16 >> 8, instruction.op16 & 0x00FF);
      break;
    }
  }

  Cpu::Flags f = state.flags();
  fprintf(stderr, "  A %02x X %02x Y %02x S %02x P %02x [%c%c%c%c%c%c%c]\n",
          state.a, state.x, state.y, state.s, state.p,
          (f.testFlag(Flag::Carry) ? 'C' : 'c'),
          (f.testFlag(Flag::Zero) ? 'Z' : 'z'),
          (f.testFlag(Flag::Interrupt) ? 'I' : 'i'),
          (f.testFlag(Flag::Decimal) ? 'D' : 'd'),
          (f.testFlag(Flag::Break) ? 'B' : 'b'),
          (f.testFlag(Flag::Overflow) ? 'V' : 'v'),
          (f.testFlag(Flag::Negative) ? 'N' : 'n')
          );
}

}
