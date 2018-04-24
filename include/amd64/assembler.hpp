#ifndef AMD64_ASSEMBLER_HPP
#define AMD64_ASSEMBLER_HPP

#include <string>
#include <vector>
#include <map>

namespace Amd64 {
using Stream = std::vector<uint8_t>;

/** Registers available on a AMD64 compliant system. */
enum Register {
  // 64-Bit registers
  R0 = 0, R1, R2, R3, R4, R5, R6, R7,
  R8, R9, R10, R11, R12, R13, R14, R15,

  // 32-Bit registers
  R0D, R1D, R2D, R3D, R4D, R5D, R6D, R7D,
  R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D,

  // 16-Bit registers
  R0W, R1W, R2W, R3W, R4W, R5W, R6W, R7W,
  R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W,

  // 8-Bit registers
  R0B, R1B, R2B, R3B, R4B, R5B, R6B, R7B,
  R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B,

  // Upper 8-Bit of 16-Bit registers
  AH, CH, DH, BH,

  // Not directly addressable
  EIP, RIP, // = 0b101

  // Dummy value
  NoRegister,

  // 64-Bit registers (Canonical names)
  // UNIX ABI  Arg  Save?
  RAX = R0, // Result
  RCX = R1, // 4th  Caller
  RDX = R2, // 3rd  Caller
  RBX = R3, // ---  Callee
  RDI = R7, // 1st  Caller
  RSI = R6, // 2nd  Caller
  RBP = R5, //
  RSP = R4, //
  //    R8, // 5th  Caller
  //    R9, // 6th  Caller
  //   R10, // ---  Caller
  //   R11, // ---  Caller
  //R12-15, // ---  Callee

  // 32-Bit registers (Canonical names)
  EAX = R0D,
  ECX = R1D,
  EDX = R2D,
  EBX = R3D,
  EDI = R7D,
  ESI = R6D,
  EBP = R5D,
  ESP = R4D,

  // 16-Bit registers (Canonical names)
  AX = R0W,
  CX = R1W,
  DX = R2W,
  BX = R3W,
  DI = R7W,
  SI = R6W,
  BP = R5W,
  SP = R4W,

  // 8-Bit registers (Canonical names)
  AL = R0B,
  CL = R1B,
  DL = R2B,
  BL = R3B,
  DIL = R7B,
  SIL = R6B,
  BPL = R5B,
  SPL = R4B,

  // Range values
  Bit64Start = R0,
  Bit64Last = R15,
  Bit32Start = R0D,
  Bit32Last = R15D,
  Bit16Start = R0W,
  Bit16Last = R15W,
  Bit8Start = R0B,
  Bit8Last = BH,
};

/** Flag bit-index into RFLAGS. */
enum Flag {
  CF = 0,
  PF = 2,
  AF = 4,
  ZF = 6,
  SF = 7,
  DF = 10,
  OF = 11,
};

/** Building blocks of the `REX` size prefix. */
enum class RexField : uint8_t {
  Prefix = 0x40,

  /** Operand Width: If set the operand is 64-Bit */
  W = 1 << 3,

  /* Important notice for the enlargement bits:
   * These bits are documented to "enlarge" various bit-fields.
   *
   * In reality, they don't enlarge these bit-fields at all.  What actually
   * happens is that these bits act as the high-bit, the bit-field is thus
   * always enlarged.
   *
   * So they don't really change the form of the `ModRM` byte.
   */

  /** Register: The ModRM `reg` field gets 1-bit longer. */
  R = 1 << 2,

  /** Index: The SIB `index` field gets 1-bit longer. */
  X = 1 << 1,

  /** Base: ModRMs `r/m` or SIBs `base` field gets 1-bit longer. */
  B = 1 << 0,
};

enum Opcode : uint16_t {
  REX_W = static_cast<uint8_t>(RexField::Prefix) | static_cast<uint8_t>(RexField::W),
  REX_R = static_cast<uint8_t>(RexField::Prefix) | static_cast<uint8_t>(RexField::R),

  ADD_AL_imm8 = 0x04, // ib
  ADD_AX_imm16 = 0x05, // iw
  ADD_EAX_imm32 = 0x06, // id
  ADD_RAX_imm32 = 0x07, // id
  ADD_RegMem8_imm8 = 0x80, // /0 ib
  ADD_RegMem16_imm16 = 0x81, // /0 iw
  ADD_RegMem32_imm32 = 0x81, // /0 id
  ADD_RegMem64_imm32 = 0x81, // /0 id
  ADD_RegMem16_imm8 = 0x83, // /0 ib
  ADD_RegMem32_imm8 = 0x83, // /0 ib
  ADD_RegMem64_imm8 = 0x83, // /0 ib
  ADD_RegMem8_Reg8 = 0x00, // /r
  ADD_RegMem16_Reg16 = 0x01, // /r
  ADD_RegMem32_Reg32 = 0x01, // /r
  ADD_RegMem64_Reg64 = 0x01, // /r
  ADD_Reg8_RegMem8 = 0x02, // /r
  ADD_Reg16_RegMem16 = 0x03, // /r
  ADD_Reg32_RegMem32 = 0x03, // /r
  ADD_Reg64_RegMem64 = 0x03, // /r
  ADC_AL_imm8 = 0x14, // ib
  ADC_AX_imm8 = 0x15, // ib
  ADC_EAX_imm8 = 0x15, // ib
  ADC_RAX_imm8 = 0x15, // ib
  ADC_RegMem8_imm8 = 0x80, // /2 ib
  ADC_RegMem16_imm16 = 0x81, // /2 iw
  ADC_RegMem32_imm32 = 0x81, // /2 id
  ADC_RegMem64_imm32 = 0x81, // /2 id
  ADC_RegMem16_imm8 = 0x83, // /2 ib
  ADC_RegMem32_imm8 = 0x83, // /2 ib
  ADC_RegMem64_imm8 = 0x83, // /2 ib
  ADC_RegMem8_Reg8 = 0x10, // /r
  ADC_RegMem16_Reg16 = 0x11, // /r
  ADC_RegMem32_Reg32 = 0x11, // /r
  ADC_RegMem64_Reg64 = 0x11, // /r
  ADC_Reg8_RegMem8 = 0x12, // /r
  ADC_Reg16_RegMem16 = 0x13, // /r
  ADC_Reg32_RegMem32 = 0x13, // /r
  ADC_Reg64_RegMem64 = 0x13, // /r
  AND_AL_imm8 = 0x24, // ib,
  AND_AX_imm16 = 0x25, // iw,
  AND_EAX_imm32 = 0x25, // id,
  AND_RAX_imm32 = 0x25, // id,
  AND_RegMem8_imm8 = 0x80, // /4 ib
  AND_RegMem16_imm16 = 0x81, // /4 iw
  AND_RegMem32_imm32 = 0x81, // /4 id
  AND_RegMem64_imm32 = 0x81, // /4 id
  AND_RegMem16_imm8 = 0x83, // /4 ib
  AND_RegMem32_imm8 = 0x83, // /4 ib
  AND_RegMem64_imm8 = 0x83, // /4 ib
  AND_RegMem8_Reg8 = 0x20, // /r
  AND_RegMem16_Reg16 = 0x21, // /r
  AND_RegMem32_Reg32 = 0x21, // /r
  AND_RegMem64_Reg64 = 0x21, // /r
  BT_RegMem16_Reg16 = 0x0FA3, // /r
  BT_RegMem32_Reg32 = 0x0FA3, // /r
  BT_RegMem64_Reg64 = 0x0FA3, // /r
  BT_RegMem16_imm8 = 0x0FBA, // /4 ib
  BT_RegMem32_imm8 = 0x0FBA, // /4 ib
  BT_RegMem64_imm8 = 0x0FBA, // /4 ib
  CALL_Near_rel16off = 0xE8, // iw
  CALL_Near_rel32off = 0xE8, // id
  CALL_Near_regmem16 = 0xFF, // /2
  CALL_Near_regmem32 = 0xFF, // /2
  CALL_Near_regmem64 = 0xFF, // /2
  CALL_Far_pntr1616 = 0x9A, // cd
  CALL_Far_pntr1632 = 0x9A, // cp
  CALL_Far_mem1616 = 0xFF, // /3
  CALL_Far_mem1632 = 0xFF, // /3
  CLC = 0xF8,
  CLD = 0xFC,
  CMC = 0xF5,
  CMP_AL_imm8 = 0x3C, // ib
  CMP_AX_imm16 = 0x3D, // iw
  CMP_EAX_imm32 = 0x3D, // id
  CMP_RAX_imm32 = 0x3D, // id
  CMP_RegMem8_imm8 = 0x80, // /7 ib
  CMP_RegMem16_imm16 = 0x81, // /7 ib
  CMP_RegMem32_imm32 = 0x81, // /7 ib
  CMP_RegMem64_imm32 = 0x81, // /7 ib
  CMP_RegMem16_imm8 = 0x83, // /7 ib
  CMP_RegMem32_imm8 = 0x83, // /7 ib
  CMP_RegMem64_imm8 = 0x83, // /7 ib
  CMP_RegMem8_Reg8 = 0x38, // /r
  CMP_RegMem16_Reg16 = 0x39, // /r
  CMP_RegMem32_Reg32 = 0x39, // /r
  CMP_RegMem64_Reg64 = 0x39, // /r
  CMP_Reg8_RegMem8 = 0x3A, // /r
  CMP_Reg16_RegMem16 = 0x3B, // /r
  CMP_Reg32_RegMem32 = 0x3B, // /r
  CMP_Reg64_RegMem64 = 0x3B, // /r
  ENTER_imm16_imm8 = 0xC8, // iw ib
  INC_RegMem8 = 0xFE, // /0
  INC_RegMem16 = 0xFF, // /0
  INC_RegMem32 = 0xFF, // /0
  INC_RegMem64 = 0xFF, // /0
  DEC_RegMem8 = 0xFE, // /1
  DEC_RegMem16 = 0xFF, // /1
  DEC_RegMem32 = 0xFF, // /1
  DEC_RegMem64 = 0xFF, // /1
  INT_3 = 0xCC,
  JMP_Near_rel8off = 0xEB, // cb
  JMP_Near_rel16off = 0xE9, // cw
  JMP_Near_rel32off = 0xE9, // cd
  JMP_Near_RegMem16 = 0xFF, // /4
  JMP_Near_RegMem32 = 0xFF, // /4
  JMP_Near_RegMem64 = 0xFF, // /4
  LEA = 0x8D,
  LEAVE = 0xC9,
  MOV_RegMem8_Reg8 = 0x88, // /r
  MOV_RegMem16_Reg16 = 0x89, // /r
  MOV_RegMem32_Reg32 = 0x89, // /r
  MOV_RegMem64_Reg64 = 0x89, // /r
  MOV_Reg8_RegMem8 = 0x8A, // /r
  MOV_Reg16_RegMem16 = 0x8B, // /r
  MOV_Reg32_RegMem32 = 0x8B, // /r
  MOV_Reg64_RegMem64 = 0x8B, // /r
  MOV_Reg8_imm8 = 0xB0, // +rb
  MOV_Reg16_imm16 = 0xB8, // +rw
  MOV_Reg32_imm32 = 0xB8, // +rd
  MOV_Reg64_imm64 = 0xB8, // +rq
  MOV_RegMem8_imm8 = 0xC6, // +rb
  MOV_RegMem16_imm16 = 0xC7, // +rw
  MOV_RegMem32_imm32 = 0xC7, // +rd
  MOV_RegMem64_imm32 = 0xC7, // +rd
  MOVZX_Reg16_RegMem8 = 0x0FB6, // /r
  MOVZX_Reg32_RegMem8 = 0x0FB6, // /r
  MOVZX_Reg64_RegMem8 = 0x0FB6, // /r
  MOVZX_Reg32_RegMem16 = 0x0FB7, // /r
  MOVZX_Reg64_RegMem16 = 0x0FB7, // /r
  OR_AL_imm8 = 0x0C, // ib,
  OR_AX_imm16 = 0x0C, // iw,
  OR_EAX_imm32 = 0x0C, // id,
  OR_RAX_imm32 = 0x0C, // id,
  OR_RegMem8_imm8 = 0x80, // /1 ib
  OR_RegMem16_imm16 = 0x81, // /1 iw
  OR_RegMem32_imm32 = 0x81, // /1 id
  OR_RegMem64_imm32 = 0x81, // /1 id
  OR_RegMem16_imm8 = 0x83, // /1 ib
  OR_RegMem32_imm8 = 0x83, // /1 ib
  OR_RegMem64_imm8 = 0x83, // /1 ib
  OR_RegMem8_Reg8 = 0x08, // /r
  OR_RegMem16_Reg16 = 0x09, // /r
  OR_RegMem32_Reg32 = 0x09, // /r
  OR_RegMem64_Reg64 = 0x09, // /r
  POPF = 0x9D,
  PUSHF = 0x9C,
  RCL_RegMem8_1 = 0xD0, // /2
  RCL_RegMem8_CL = 0xD2, // /2
  RCL_RegMem8_imm8 = 0xC0, // /2 ib
  RCL_RegMem16_1 = 0xD1, // /2
  RCL_RegMem16_CL = 0xD3, // /2
  RCL_RegMem16_imm8 = 0xC1, // /2 ib
  RCL_RegMem32_1 = 0xD1, // /2
  RCL_RegMem32_CL = 0xD3, // /2
  RCL_RegMem32_imm8 = 0xC1, // /2 ib
  RCL_RegMem64_1 = 0xD1, // /2
  RCL_RegMem64_CL = 0xD3, // /2
  RCL_RegMem64_imm8 = 0xC1, // /2 ib
  RCR_RegMem8_1 = 0xD0, // /3
  RCR_RegMem8_CL = 0xD2, // /3
  RCR_RegMem8_imm8 = 0xC0, // /3 ib
  RCR_RegMem16_1 = 0xD1, // /3
  RCR_RegMem16_CL = 0xD3, // /3
  RCR_RegMem16_imm8 = 0xC1, // /3 ib
  RCR_RegMem32_1 = 0xD1, // /3
  RCR_RegMem32_CL = 0xD3, // /3
  RCR_RegMem32_imm8 = 0xC1, // /3 ib
  RCR_RegMem64_1 = 0xD1, // /3
  RCR_RegMem64_CL = 0xD3, // /3
  RCR_RegMem64_imm8 = 0xC1, // /3 ib
  RET_Near = 0xC3,
  RET_Near_imm16 = 0xC2,
  RET_Far = 0xCB,
  RET_Far_imm16 = 0xCA,
  ROL_RegMem8_1 = 0xD0, // /0
  ROL_RegMem8_CL = 0xD2, // /0
  ROL_RegMem8_imm8 = 0xC0, // /0 ib
  ROL_RegMem16_1 = 0xD1, // /0
  ROL_RegMem16_CL = 0xD3, // /0
  ROL_RegMem16_imm8 = 0xC1, // /0 ib
  ROL_RegMem32_1 = 0xD1, // /0
  ROL_RegMem32_CL = 0xD3, // /0
  ROL_RegMem32_imm8 = 0xC1, // /0 ib
  ROL_RegMem64_1 = 0xD1, // /0
  ROL_RegMem64_CL = 0xD3, // /0
  ROL_RegMem64_imm8 = 0xC1, // /0 ib
  ROR_RegMem8_1 = 0xD0, // /1
  ROR_RegMem8_CL = 0xD2, // /1
  ROR_RegMem8_imm8 = 0xC0, // /1 ib
  ROR_RegMem16_1 = 0xD1, // /1
  ROR_RegMem16_CL = 0xD3, // /1
  ROR_RegMem16_imm8 = 0xC1, // /1 ib
  ROR_RegMem32_1 = 0xD1, // /1
  ROR_RegMem32_CL = 0xD3, // /1
  ROR_RegMem32_imm8 = 0xC1, // /1 ib
  ROR_RegMem64_1 = 0xD1, // /1
  ROR_RegMem64_CL = 0xD3, // /1
  ROR_RegMem64_imm8 = 0xC1, // /1 ib
  SHL_RegMem8_1 = 0xD0, // /4
  SHL_RegMem8_CL = 0xD2, // /4
  SHL_RegMem8_imm8 = 0xC0, // /4 ib
  SHL_RegMem16_1 = 0xD1, // /4
  SHL_RegMem16_CL = 0xD3, // /4
  SHL_RegMem16_imm8 = 0xC1, // /4 ib
  SHL_RegMem32_1 = 0xD1, // /4
  SHL_RegMem32_CL = 0xD3, // /4
  SHL_RegMem32_imm8 = 0xC1, // /4 ib
  SHL_RegMem64_1 = 0xD1, // /4
  SHL_RegMem64_CL = 0xD3, // /4
  SHL_RegMem64_imm8 = 0xC1, // /4 ib
  SHR_RegMem8_1 = 0xD0, // /5
  SHR_RegMem8_CL = 0xD2, // /5
  SHR_RegMem8_imm8 = 0xC0, // /5 ib
  SHR_RegMem16_1 = 0xD1, // /5
  SHR_RegMem16_CL = 0xD3, // /5
  SHR_RegMem16_imm8 = 0xC1, // /5 ib
  SHR_RegMem32_1 = 0xD1, // /5
  SHR_RegMem32_CL = 0xD3, // /5
  SHR_RegMem32_imm8 = 0xC1, // /5 ib
  SHR_RegMem64_1 = 0xD1, // /5
  SHR_RegMem64_CL = 0xD3, // /5
  SHR_RegMem64_imm8 = 0xC1, // /5 ib
  STC = 0xF9,
  STD = 0xFD,
  SBB_AL_imm8 = 0x1C, // ib
  SBB_AX_imm16 = 0x1D, // iw
  SBB_EAX_imm32 = 0x1D, // id
  SBB_RAX_imm32 = 0x1D, // id
  SBB_RegMem8_imm8 = 0x80, // /3 ib
  SBB_RegMem16_imm16 = 0x81, // /3 iw
  SBB_RegMem32_imm32 = 0x81, // /3 id
  SBB_RegMem64_imm32 = 0x81, // /3 id
  SBB_RegMem16_imm8 = 0x83, // /3 ib
  SBB_RegMem32_imm8 = 0x83, // /3 ib
  SBB_RegMem64_imm8 = 0x83, // /3 ib
  SBB_RegMem8_Reg8 = 0x18, // /r
  SBB_RegMem16_Reg16 = 0x19, // /r
  SBB_RegMem32_Reg32 = 0x19, // /r
  SBB_RegMem64_Reg64 = 0x19, // /r
  SBB_Reg8_RegMem8 = 0x1A, // /r
  SBB_Reg16_RegMem16 = 0x1B, // /r
  SBB_Reg32_RegMem32 = 0x1B, // /r
  SBB_Reg64_RegMem64 = 0x1B, // /r
  SUB_AL_imm8 = 0x2C, // ib
  SUB_AX_imm16 = 0x2D, // iw
  SUB_EAX_imm32 = 0x2D, // id
  SUB_RAX_imm32 = 0x2D, // id
  SUB_RegMem8_imm8 = 0x80, // /5 ib
  SUB_RegMem16_imm16 = 0x81, // /5 iw
  SUB_RegMem32_imm32 = 0x81, // /5 id
  SUB_RegMem64_imm32 = 0x81, // /5 id
  SUB_RegMem16_imm8 = 0x83, // /5 ib
  SUB_RegMem32_imm8 = 0x83, // /5 ib
  SUB_RegMem64_imm8 = 0x83, // /5 ib
  SUB_RegMem8_Reg8 = 0x28, // /r
  SUB_RegMem16_Reg16 = 0x29, // /r
  SUB_RegMem32_Reg32 = 0x29, // /r
  SUB_RegMem64_Reg64 = 0x29, // /r
  SUB_Reg8_RegMem8 = 0x2A, // /r
  SUB_Reg16_RegMem16 = 0x2B, // /r
  SUB_Reg32_RegMem32 = 0x2B, // /r
  SUB_Reg64_RegMem64 = 0x2B, // /r
  TEST_AL_imm8 = 0xA8, // ib
  TEST_AX_imm16 = 0xA9, // iw
  TEST_EAX_imm32 = 0xA9, // id
  TEST_RAX_imm32 = 0xA9, // id
  TEST_RegMem8_imm8 = 0xF6, // /0 ib
  TEST_RegMem16_imm16 = 0xF7, // /0 iw
  TEST_RegMem32_imm32 = 0xF7, // /0 id
  TEST_RegMem64_imm32 = 0xF7, // /0 id
  TEST_RegMem8_Reg8 = 0x84, // /r
  TEST_RegMem16_Reg16 = 0x85, // /r
  TEST_RegMem32_Reg32 = 0x85, // /r
  TEST_RegMem64_Reg64 = 0x85, // /r
  XOR_AL_imm8 = 0x34, // ib
  XOR_AX_imm8 = 0x35, // ib
  XOR_EAX_imm8 = 0x35, // ib
  XOR_RAX_imm8 = 0x35, // ib
  XOR_RegMem8_imm8 = 0x80, // /6 ib
  XOR_RegMem16_imm16 = 0x81, // /6 iw
  XOR_RegMem32_imm32 = 0x81, // /6 id
  XOR_RegMem64_imm32 = 0x81, // /6 id
  XOR_RegMem16_imm8 = 0x83, // /6 ib
  XOR_RegMem32_imm8 = 0x83, // /6 ib
  XOR_RegMem64_imm8 = 0x83, // /6 ib
  XOR_RegMem8_Reg8 = 0x30, // /r
  XOR_RegMem16_Reg16 = 0x31, // /r
  XOR_RegMem32_Reg32 = 0x31, // /r
  XOR_RegMem64_Reg64 = 0x31, // /r
  XOR_Reg8_RegMem8 = 0x32, // /r
  XOR_Reg16_RegMem16 = 0x33, // /r
  XOR_Reg32_RegMem32 = 0x33, // /r
  XOR_Reg64_RegMem64 = 0x33, // /r

  OperandSizeOverride = 0x66,
  AddressSizeOverride = 0x67,
};

enum Condition {
  Overflow = 0, ///< JO (OF = 1)
  NotOverflow, ///< JNO (OF = 0)
  Carry, ///< JC (CF = 1)
  NotCarry, ///< JNC (CF = 0)
  Zero, ///< JZ (ZF = 0)
  NotZero, ///< JNZ (ZF = 1)
  BelowOrEqual, ///< JBE (CF = 1 or ZF = 1)
  NotBelowOrEqual, ///< JNBE (CF = 0 and ZF = 0)
  Sign, ///< JS (SF = 1)
  NotSign, ///< JNS (SF = 0)
  Parity, ///< JP (PF = 1)
  NotParity, ///< JNP (PF = 0)
  Less, ///< JL (SF != OF)
  NotLess, ///< JNL (SF = OF)
  LessOrEqual, ///< JLE (ZF = 1 or SF != OF)
  NotLessOrEqual, ///< JNLE (ZF = 0 and SF = OF)

  // Alternate names

  Below = Carry,
  NotAboveOrEqual = Carry,
  NotBelow = NotCarry,
  AboveOrEqual = NotCarry,
  Equal = Zero,
  NotEqual = NotZero,
  NotAbove = BelowOrEqual,
  Above = NotBelowOrEqual,
  ParityEven = Parity,
  ParityOdd = NotParity,
  NotGreaterOrEqual = Less,
  GreaterOrEqual = NotLess,
  NotGreater = LessOrEqual,
  Greater = NotLessOrEqual,
};

/**
 * A reference in a \c Section to an internal or external symbol.  Basically a
 * named pointer resolved at link-time.
 */
struct Reference {
  /** Name of the referenced symbol. */
  std::string name;

  /** Offset in the section byte stream */
  uintptr_t offset;

  /** Size of the reference field */
  size_t size;

  /**
   * Base address to assume for relative addressing.  If this is zero this
   * reference is assumed to use absolute addressing instead for a pointer, or
   * a plain value for a non-pointer symbol.
   *
   * Note that this base address is relative to the start of the section it is
   * used in.
   */
  uintptr_t base;
};

class Section; // For `struct Name`

/**
 * Reference to a memory and/or register.  Allows to also encode a displacement
 * (Which can be a named reference), an index register and a scale.
 *
 * The \a scale has to be one of 1, 2, 4, or 8.
 *
 * The \a index register, if used, \b must be of same size as the \a base
 * register.
 */
class MemReg {
  MemReg(std::string n, bool deref, int32_t disp, Register b, Register i, uint8_t s)
    : name(n), displacement(disp), base(b), index(i), scale(s), dereference(deref)
  { this->sanityCheck(); }

public:
  MemReg(const Section &disp, Register base = NoRegister, Register index = NoRegister, uint8_t scale = 1);
  MemReg(std::string disp, Register base = NoRegister, Register index = NoRegister, uint8_t scale = 1)
    : MemReg(disp, true, 0, base, index, scale) { }
  MemReg(const char *disp, Register base = NoRegister, Register index = NoRegister, uint8_t scale = 1)
    : MemReg(disp, true, 0, base, index, scale) { }
  MemReg(int32_t disp, Register base = NoRegister, Register index = NoRegister, uint8_t scale = 1)
    : MemReg(std::string(), true, disp, base, index, scale) { }
  MemReg(Register base, Register index = NoRegister, uint8_t scale = 1)
    : MemReg(std::string(), true, 0, base, index, scale) { }

  /**
   * Builds a value-reference:  This reference will be replaced by the \b value.
   * This may be dangerous to do.
   */
  static MemReg value(const Section &symbol);
  static MemReg value(std::string symbol)
  { return MemReg(symbol, false, 0, NoRegister, NoRegister, 1); }
  static MemReg value(const char *symbol)
  { return MemReg(symbol, false, 0, NoRegister, NoRegister, 1); }
  static MemReg value(Register reg)
  { return MemReg(std::string(), false, 0, reg, NoRegister, 1); }

  /** Will this reference produce an immediate value? */
  bool isImmediate() const
  { return (!this->dereference && this->base == NoRegister); }

  /** Is this a computed address reference? */
  bool isComputed() const
  { return (this->base != NoRegister); }

  /** Throws if this \c MemReg is not dereferenced. */
  void throwIfValue() const;

  // Displacement
  std::string name;
  int32_t displacement;

  // SIB
  Register base;
  Register index;
  uint8_t scale;

  // Should this be dereferenced (== Memory access)?
  bool dereference;

private:
  void sanityCheck();
};

/**
 * Container for different versions (opcodes) of an instruction.
 */
struct InstructionDescriptor {
  // Instruction(MemReg, Reg) or Instruction(Reg, MemReg)
  Opcode memreg8, memreg;

  // Instruction(Reg, Imm)/Group
  Opcode imm8, imm;
  uint8_t group;
};

/**
 * A section used by the \c Assembler and \c Linker.  It stores arbitrary binary
 * data, but its primary purpose is to store (and build) AMD64 instructions.
 */
class Section {
  template<typename T>
  static constexpr uint8_t u8(T t, int shift = 0){ return static_cast<uint8_t>(t >> shift); }
public:
  Section(const std::string &name) : name(name) { }

  /** The name of the section. */
  const std::string name;

  /** The body of the section. */
  Stream bytes;

  /** References, pointing into \c bytes. */
  std::vector<Reference> references;

  /** Returns the size (in Bytes) of this section. */
  size_t size() const { return this->bytes.size(); }

  /**
   * Appends the body of \a other to this section.  Also appends the references
   * from \a other, adjusting their relative addresses automatically.
   */
  void append(const Section &other);

  /** Appends \a bytes to the section. */
  void append(const std::initializer_list<uint8_t> &bytes)
  { this->bytes.insert(this->bytes.end(), bytes); }

  /** Appends \a bytes to the section. */
  void append(const Stream &bytes)
  { this->bytes.insert(this->bytes.end(), bytes.begin(), bytes.end()); }

  /** Appends \a len bytes starting at \a bytes to the section. */
  void append(const uint8_t *bytes, size_t len)
  { this->bytes.insert(this->bytes.end(), len, *bytes); }

  void append(Opcode opcode) {
    if (opcode > 0xFF) {
      this->bytes.insert(this->bytes.end(), { uint8_t(opcode >> 8), uint8_t(opcode) });
    } else {
      this->bytes.insert(this->bytes.end(), uint8_t(opcode));
    }
  }

  // append(unsigned int) variants

  void append(uint8_t byte) { this->bytes.insert(this->bytes.end(), byte); }
  void append(uint16_t word) { this->bytes.insert(this->bytes.end(), { u8(word, 0), u8(word, 8) }); }
  void append(uint32_t dword)
  { this->bytes.insert(this->bytes.end(), { u8(dword, 0), u8(dword, 8), u8(dword, 16), u8(dword, 24) }); }
  void append(uint64_t qword) {
    this->bytes.insert(this->bytes.end(), {
                         u8(qword, 0), u8(qword, 8), u8(qword, 16), u8(qword, 24),
                         u8(qword, 32), u8(qword, 40), u8(qword, 48), u8(qword, 56),
                       });
  }

  // append(signed int) variants

  void append(int8_t sbyte) { this->append(static_cast<uint8_t>(sbyte)); }
  void append(int16_t sword) { this->append(static_cast<uint16_t>(sword)); }
  void append(int32_t sdword) { this->append(static_cast<uint32_t>(sdword)); }
  void append(int64_t sqword) { this->append(static_cast<uint64_t>(sqword)); }

  /** Appends all passed arguments to the section in the given order. */
  template<typename T, typename U, typename ... V>
  void append(T t, U u, V ... v) {
    this->append(t);
    this->append(u, v...);
  }

  /**
   * Appends \a value as immediate value of \a bits size.
   */
  void appendImmediate(uint64_t value, int bits);

  /**
   * Appends the \a opcode to the section, using the \a source and
   * \a destination.  Prefixes and suffixes are automatically added as needed.
   */
  void emitGeneric(Opcode opcode, Register source, Register destination);
  void emitGeneric(Opcode opcode, Register reg, const MemReg &memreg);
  void emitGeneric(Opcode opcode, uint8_t group, const MemReg &memreg, int bits);
  void emitGeneric(Opcode opcode, uint8_t group, Register destination);

  /**
   * Appends the \a string and returns its offset.  The string is guaranteed to
   * end with a NUL-Byte.
   */
  uintptr_t appendString(const std::string &string);

  /**
   * Adds a named reference to a byte-range in the section.  The \a name is
   * later looked-up at link-time, and the byte-range is replaced by that
   * symbols value.
   *
   * The \a offset can be positive (in which case it points to an offset
   * relative to the beginning of the \b current start of the section).  If it
   * is negative it points to an offset relative to the end of the \b current
   * end of the section.
   *
   * If \a base is non-zero, the reference will use a relative address to
   * \a base.
   */
  void addRef(const MemReg &name, intptr_t offset, size_t size, uintptr_t base = 0) {
    intptr_t s = static_cast<intptr_t>(this->bytes.size());
    uintptr_t off = static_cast<uintptr_t>((s + offset) % s);
    this->references.push_back(Reference{ name.name, off, size, base });
  }

  /**
   * Like \c addRef(), but assumes an address relative to the instruction
   * pointer.
   */
  void addRipRef(const MemReg &name, intptr_t offset, size_t size)
  { this->addRef(name, offset, size, this->bytes.size()); }

  /* emitX() methods to append instructions to the section. */

  void emitAdd(Register source, Register destination, bool withCarry = false);
  void emitAdd(int32_t immediate, Register destination, bool withCarry = false);
  void emitAnd(Register source, Register destination);
  void emitAnd(uint32_t immediate, Register destination);
  void emitBt(uint8_t immediate, Register source);
  void emitCall(Register destination);
  void emitCall(const MemReg &symbol);
  void emitCmp(Register left, int32_t right); /* CMP(left, right) = `if (LEFT <=> RIGHT) { .. }` */
  void emitCmp(Register left, Register right);
  void emitEnter(uint16_t stackSpace = 0, bool nestedFrame = false);
  void emitInc(Register regi);
  void emitInc(const MemReg &address, int bitSize);
  void emitDec(Register regi);
  void emitDec(const MemReg &address, int bitSize);
  void emitJcc(Condition cond, int32_t displacement);
  void emitJcc(Condition cond, const MemReg &destination);
  void emitJmp(const MemReg &destination);
  void emitJmp(Register destination);
  void emitJmp(int32_t displacement);
  void emitLeave();
  void emitMov(uint64_t immediate, Register destination);
  void emitMov(Register source, Register destination);
  void emitMov(Register source, const MemReg &destination);
  void emitMov(const MemReg &source, Register destination);
  void emitMovzx(Register source, Register destination);
  void emitOr(Register source, Register destination);
  void emitOr(uint32_t immediate, Register destination);
  void emitPopf() { this->append(POPF); }
  void emitPushf() { this->append(PUSHF); }
  void emitPrefix(Register first, Register second, bool sameSize = true);
  void emitPrefix(Register reg, const MemReg &ref);
  void emitPrefix(const MemReg &ref, int bits);
  void emitPrefix(Register toUse);
  void emitPrefix(int bits);
  void emitRcl(uint8_t immediate, Register destination);
  void emitRclCl(Register destination);
  void emitRcr(uint8_t immediate, Register destination);
  void emitRcrCl(Register destination);
  void emitRet(uint16_t popBytes = 0);
  void emitRex(bool w = false, bool r = false, bool x = false, bool b = false);
  void emitRex(RexField bits)
  { this->append(static_cast<uint8_t>(RexField::Prefix) | static_cast<uint8_t>(bits)); }
  void emitRol(uint8_t immediate, Register destination);
  void emitRolCl(Register destination);
  void emitRor(uint8_t immediate, Register destination);
  void emitRorCl(Register destination);
  void emitShl(uint8_t immediate, Register destination);
  void emitShlCl(Register destination);
  void emitShr(uint8_t immediate, Register destination);
  void emitShrCl(Register destination);
  void emitSetcc(Condition cond, Register destination);
  void emitSetcc(Condition cond, const MemReg &destination);
  void emitSub(Register source, Register destination, bool withBorrow = false);
  void emitSub(int32_t immediate, Register destination, bool withBorrow = false);
  void emitSuffix(const MemReg &memory, Register reg = NoRegister, uint8_t group = 0);
  void emitTest(Register source, Register destination);
  void emitTest(uint32_t immediate, Register destination);
  void emitXor(Register source, Register destination);
  void emitXor(uint32_t immediate, Register destination);

private:
  void emitJccPrefix(Condition cond, int bits);
};

/**
 * The assembler class mainly manages named sections, which then hold
 * information of their respective instructions.
 */
class Assembler {
public:
  Assembler();

  /** The sections int this assembler. */
  const std::map<std::string, Section> &sections() const
  { return this->m_sections; }

  /**
   * Returns the section \a name.  If no section called \a name exists, returns
   * a new section instead.
   */
  Section &section(const std::string &name);

private:
  std::map<std::string, Section> m_sections;
};
}

#endif // AMD64_ASSEMBLER_HPP
