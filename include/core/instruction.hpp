#ifndef CORE_INSTRUCTION_HPP
#define CORE_INSTRUCTION_HPP

#include <cstdint>

namespace Core {
/**
 * Stores a decoded 6502 instruction.
 */
struct Instruction {
  /** Memory addressing modes. */
  enum Addressing : uint8_t {
  // Short   Name            Access               Operand-Bytes
    Acc,  // Accumulator     a                    0
    X,    // X               x                    0
    Y,    // Y               y                    0
    S,    // S               s                    0
    P,    // P               p                    0
    Imm,  // Immediate       *(pc + 1)            1
    Imp,  // Implied         *(pc + 1)            1
    Rel,  // Relative        pc + op              1
    Zp,   // Zero-Page       *(op8)               1
    ZpX,  // Zero-Page,X     *((op8 + X) & 0xFF)  1
    ZpY,  // Zero-Page,Y     *((op8 + Y) & 0xFF)  1
    Abs,  // Absolute        *(op16)              2
    AbsX, // Absolute,X      *(op16 + X)          2
    AbsY, // Absolute,Y      *(op16 + Y)          2
    Ind,  // Indirect        *(*(op16))           2
    IndX, // Indirect,X      *(*(op8 + X))        1
    IndY, // Indirect,Y      *(*(op8) + Y)        1
  };

  /** List of all 6502 instructions. */
  enum Command : uint8_t {
    Unknown, // ??
    ADC, // ADd with Carry
    AND, // bitwise AND with accumulator
    ASL, // Arithmetic Shift Left
    BCC, // Branch on Carry Clear
    BCS, // Branch on Carry Set
    BEQ, // Branch on EQual
    BIT, // test BITs
    BMI, // Branch on MInus
    BNE, // Branch on Not Equal
    BPL, // Branch on PLus
    BRK, // BReaK
    BVC, // Branch on oVerflow Clear
    BVS, // Branch on oVerflow Set
    CLC, // CLear Carry
    CLD, // CLear Decimal
    CLI, // CLear Interrupt
    CLV, // CLear oVerflow
    CMP, // CoMPare accumulator
    CPX, // ComPare X register
    CPY, // ComPare Y register
    DEC, // DECrement memory
    DEX, // DEcrement X register
    DEY, // DEcrement Y register
    EOR, // bitwise Exclusive OR
    INC, // INCrement memory
    INX, // INcrement X register
    INY, // INcrement Y register
    JMP, // JuMP
    JSR, // Jump to SubRoutine
    LDA, // LoaD Accumulator
    LDX, // LoaD X register
    LDY, // LoaD Y register
    LSR, // Logical Shift Right
    NOP, // No OPeration
    ORA, // bitwise OR with Accumulator
    PHA, // PusH Accumulator
    PHP, // PusH P register
    PLA, // PuLl Accumulator
    PLP, // PuLl P register
    ROL, // ROtate Left
    ROR, // ROtate Right
    RTI, // ReTurn from Interrupt
    RTS, // ReTurn from Subroutine
    SBC, // SuBtract with Carry
    SEC, // SEt Carry
    SED, // SEt decimal
    SEI, // SEt Interrupt
    STA, // STore Accumulator
    STX, // STore X register
    STY, // STore Y register
    TAX, // Transfer A to X
    TAY, // Transfer A to Y
    TSX, // Transfer S to X
    TXA, // Transfer X to A
    TXS, // Transfer X to S
    TYA, // Transfer Y to A
  };

  Addressing addressing;
  Command command;
  int cycles; /// Cycle count to run

  union {
    uint16_t op16;
    int16_t ops16;

    union {
      uint8_t op8;
      int8_t ops8;
    };
  };

  Instruction(Command cmd, Addressing mode, int cycleCount, uint16_t op = 0)
    : addressing(mode), command(cmd), cycles(cycleCount), op16(op) { }

  Instruction(Command cmd, Addressing mode, int cycleCount, uint8_t op)
    : addressing(mode), command(cmd), cycles(cycleCount), op8(op) { }

  /** Size of the operand in Bytes. */
  int operandSize() const;

  /** Name of the command */
  const char *commandName() const;

  /** Name of the addressing */
  const char *addressingName() const;

  /** Returns \c true if this instruction accesses memory. */
  bool isMemory() const;
  static bool isMemory(Addressing mode);

  /** Returns \c true if this is a branching instruction. */
  bool isBranching() const;

  /** Is this a conditional branching instruction? */
  bool isConditionalBranching() const;

  /**
   * Returns the target address if this is a conditional branching instruction,
   * and the branch is taken.  As addressing is relative, the \a base address of
   * this instruction must be passed in.
   *
   * If this instruction doesn't contain a relative address operand, the result
   * is undefined.
   */
  uint16_t destinationAddress(uint16_t base) const;

  /** Produces an \c Instruction from the \a opcode, without an operand. */
  static Instruction decode(uint8_t opcode);
};
}

#endif // CORE_INSTRUCTION_HPP
