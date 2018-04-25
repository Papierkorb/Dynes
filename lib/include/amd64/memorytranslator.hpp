#ifndef AMD64_MEMORYTRANSLATOR_HPP
#define AMD64_MEMORYTRANSLATOR_HPP

#include <core/instruction.hpp>
#include "assembler.hpp"
#include "constants.hpp"

#include <functional>

namespace Amd64 {
/**
 * Logic implementing 6502 memory addressing in AMD64.
 */
class MemoryTranslator {
  Section &m_sec;
public:
  MemoryTranslator(Section &sec);

  /** Resolves the \a instr to an absolute memory address. */
  void resolve(const ::Core::Instruction &instr, Register destination);

  /**
   * Resolves the (maybe relative) \a addr to an absolute address using \a mode.
   * Puts the resolved address into the \c 16-Bit register \a destination.
   */
  void resolve(::Core::Instruction::Addressing mode, uint16_t addr, Register destination);

  /** Reads the byte \a instr is pointing at, be it a memory address or a register. */
  Register read(const ::Core::Instruction &instr);

  /**
   * Reads the byte addressed by \a mode at \a addr.  Returns the \c 8-Bit
   * register the value is stored in.
   */
  Register read(::Core::Instruction::Addressing mode, uint16_t addr);

  /**
   * Writes from the \c 8-Bit \a source register into what \a instr is pointing
   * at.
   */
  void write(const ::Core::Instruction &instr, Register source);

  /**
   * Writes from the \c 8-Bit \a source register into \a addr using \a mode.
   */
  void write(::Core::Instruction::Addressing mode, uint16_t addr, Register source);

  /**
   * Reads the byte \a instr is pointing at.  This byte is then passed to
   * \a proc.  The result of \a proc is then written back into the same place.
   * The \a proc receives an \c 8-Bit register, and is expected to return a
   * \c 8-Bit register.  The returned register may be the one that was passed
   * in.
   */
  void rmw(const ::Core::Instruction &instr, std::function<Register(Register)> proc);

  /** Like the \c Core::Instruction version. */
  void rmw(::Core::Instruction::Addressing mode, uint16_t addr, std::function<Register(Register)> proc);

  /** Pushes 8-Bit onto the stack. */
  void push8(Register source);

  /** Pushes 16-Bit onto the stack. */
  void push16(Register source);

  /** Pulls 8-Bit from the stack into \a destination. */
  void pull8(Register destination);

  /** Pulls 16-Bit from the stack into \a destination. */
  void pull16(Register destination);
};
}

#endif // AMD64_MEMORYTRANSLATOR_HPP
