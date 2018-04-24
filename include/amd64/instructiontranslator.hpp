#ifndef AMD64_INSTRUCTIONTRANSLATOR_HPP
#define AMD64_INSTRUCTIONTRANSLATOR_HPP

#include "assembler.hpp"

#include <analysis/branch.hpp>

#include <cpu.hpp>
#include <cpu/state.hpp>

namespace Amd64 {

/**
 * Translator for individual 6502 instructions to AMD64 instructions.  A single
 * translator will only translate a single instruction.
 */
class InstructionTranslator {
public:
  InstructionTranslator(Section &section);

  /**
   * Translates the \a instr at \a address.  Returns \c true if the instruction
   * did \b NOT end in a branching-instruction.  Returns \c false if it did end
   * in a branching instruction.
   */
  std::pair<bool, uint16_t> translate(uint16_t address, const Analysis::Branch::Instruction &instr);
  void translate(uint16_t address, Analysis::ConditionalInstruction instr);
  std::pair<bool, uint16_t> translate(uint16_t address, ::Core::Instruction instr);
private:
  Section &m_sec;

  void traceInstruction(uint16_t address, ::Core::Instruction instr);
  void logInstruction(uint16_t address, ::Core::Instruction instr);
  void adc(Register value);
  void countCycles(int cycles);
  void compare(Register reg, Register mem);
  void setNz(uint8_t addMask = 0);
  void updateFlag(Cpu::Flag flag, bool set);
  void updateFlag(Cpu::Flag flag, Register reg, bool alreadyMasked = false);
  void updateFlagFromFlags(Cpu::Flag flag);
  void returnToHost(Cpu::State::Reason reason, Register pc);

};
}

#endif // AMD64_INSTRUCTIONTRANSLATOR_HPP
