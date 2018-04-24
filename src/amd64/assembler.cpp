#include <amd64/assembler.hpp>

#include <stdexcept>

namespace Amd64 {
Assembler::Assembler() {
}

Section &Assembler::section(const std::string &name) {
  if (this->m_sections.find(name) == this->m_sections.end()) {
    this->m_sections.emplace(name, Section(name));
  }

  return this->m_sections.at(name);
}

template<typename T>
static int immediateBits(T imm, bool hasSign = true) {
  if (hasSign) {
    int64_t value = static_cast<int64_t>(imm);

    if (value == static_cast<int8_t>(value)) return 8;
    else if (value == static_cast<int16_t>(value)) return 16;
    else if (value == static_cast<int32_t>(value)) return 32;
    else return 64;
  } else {
    if (imm == static_cast<uint8_t>(imm)) return 8;
    else if (imm == static_cast<uint16_t>(imm)) return 16;
    else if (imm == static_cast<uint32_t>(imm)) return 32;
    else return 64;
  }
}

static constexpr uint8_t registerIndex(Register regi) {
  switch (regi) {
  case NoRegister: return 0;
  case AH:
  case BH:
  case CH:
  case DH: return static_cast<uint8_t>(regi - AH) + 4;
  case EIP: // Special case.
  case RIP:
    return 5;
  default: return static_cast<uint8_t>(regi) & 7;
  }
}

/* ModRM helpers */
static constexpr uint8_t RIP_RELATIVE   = 0b101;
static constexpr uint8_t HAS_SIB_BYTE   = 0b100;
static constexpr uint8_t MOD_REGISTER   = 0b11; //    Register <-/-> Register
static constexpr uint8_t MOD_MEM_DISP32 = 0b10; // Memory+Disp <-/-> Register (Displacement is 32Bit)
static constexpr uint8_t MOD_MEM_DISP8  = 0b01; // Memory+Disp <-/-> Register (Displacement is 8Bit)
static constexpr uint8_t MOD_MEM        = 0b00; //     Memotry <-/-> Register

static constexpr uint8_t modrm(uint8_t mod, uint8_t reg, uint8_t rm) {
  //  7 6  5 4 3  2 1 0  Bits
  // [mod][ reg ][ r/m ]
  return ((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7);
}

static constexpr uint8_t regreg(Register source, Register target)
{ return modrm(MOD_REGISTER, registerIndex(source), registerIndex(target)); }

static constexpr uint8_t reg(uint8_t group, Register target)
{ return modrm(MOD_REGISTER, group, registerIndex(target)); }

/* SIB helpers */
static constexpr uint8_t log2scale(uint8_t scale) {
  switch (scale) { // = log2(scale)
  case 1: return 0;
  case 2: return 1;
  case 4: return 2;
  case 8: return 3;
  default: // Redundant with MemReg::sanityCheck()
    throw std::invalid_argument("Addressing error: The scale can only be 1, 2, 4 or 8");
  }
}

static constexpr uint8_t sib(Register base, Register index, uint8_t scale) {
  uint8_t log2 = log2scale(scale);
  return ((log2 & 3) << 6) | (registerIndex(index) << 3) | registerIndex(base);
}

/** Returns the size of \a regi in bits. */
static int registerBits(Register regi) {
  if (regi >= Bit64Start && regi <= Bit64Last) return 64;
  if (regi >= Bit32Start && regi <= Bit32Last) return 32;
  if (regi >= Bit16Start && regi <= Bit16Last) return 16;
  if (regi >= Bit8Start && regi <= Bit8Last) return 8;
  if (regi == NoRegister) return 0;

  throw std::runtime_error("BUG: Assembler::registerBits is missing a case");
}

static bool registerExtended(Register regi) {
  if (regi >= R8 && regi <= R15) return true;
  if (regi >= R8D && regi <= R15D) return true;
  if (regi >= R8W && regi <= R15W) return true;
  if (regi >= R8B && regi <= R15B) return true;

  return false;
}

static int registerSameBits(Register first, Register second) {
  int bits = registerBits(first);

  if (bits != registerBits(second)) {
    throw std::invalid_argument("First and second registers have to be of same size");
  }

  return bits;
}

static void checkRexAndHigh8BitRegisters(Register first, Register second) {
  bool fe = registerExtended(first);
  bool se = registerExtended(second);

  // Special rule: Impossible to address `%xH` and an extended register at once.
  bool fe8 = fe || (first >= SPL && first <= DIL);
  bool se8 = se || (second >= SPL && second <= DIL);
  if ((fe8 && (second >= AH && second <= BH)) || (se8 && (first >= AH && first <= BH))) {
    throw std::invalid_argument("Can't address AH/BH/CH/DH and a REX-prefix register at once");
  }
}

void Section::append(const Section &other) {
  uintptr_t offset = this->bytes.size(); // New base offset of the added section body.

  this->bytes.insert(this->bytes.end(), other.bytes.begin(), other.bytes.end());

  // Append and adjust references
  this->references.reserve(this->references.size() + other.references.size());
  for (Reference ref : other.references) {
    if (ref.base > 0) { // Adjust relative addresses
      ref.base += offset;
    }

    ref.offset += offset;
    this->references.push_back(ref);
  }
}

void Section::appendImmediate(uint64_t value, int bits) {
  switch (bits) {
  case 64:
    this->append(value);
    return;
  case 32:
    this->append(static_cast<uint32_t>(value));
    return;
  case 16:
    this->append(static_cast<uint16_t>(value));
    return;
  case 8:
    this->append(static_cast<uint8_t>(value));
    return;
  default:
    throw std::invalid_argument("appendImmediate: Bits has to be one of 64, 32, 16 or 8");
  }
}

void Section::emitGeneric(Opcode opcode, Register source, Register destination) {
  this->emitPrefix(source, destination);
  this->append(opcode, regreg(source, destination));
}

void Section::emitGeneric(Opcode opcode, Register reg, const MemReg &memreg) {
  if (reg != NoRegister) {
    checkRexAndHigh8BitRegisters(reg, memreg.base);
  }

  if (memreg.dereference) { // Memory access
    this->emitPrefix(reg, memreg);
    this->append(opcode);
    this->emitSuffix(memreg, reg);
  } else { // Register access
    this->emitPrefix(reg, memreg.base);
    this->append(opcode, regreg(reg, memreg.base));
  }
}

void Section::emitGeneric(Opcode opcode, uint8_t group, const MemReg &memreg, int bits) {
  memreg.throwIfValue();

  if (registerBits(memreg.base) == 16) { this->append(AddressSizeOverride); }
  this->emitPrefix(memreg, bits);
  this->append(opcode);
  this->emitSuffix(memreg, NoRegister, group);
}

void Section::emitGeneric(Opcode opcode, uint8_t group, Register destination) {
  this->emitPrefix(destination);
  this->append(opcode, reg(group, destination));
}

uintptr_t Section::appendString(const std::string &string) {
  uintptr_t offset = this->bytes.size();
  this->bytes.insert(this->bytes.end(), string.begin(), string.end());
  this->bytes.insert(this->bytes.end(), uint8_t(0));
  return offset;
}

template<Opcode r8, Opcode r16>
static void genericRegToReg(Section *self, Register source, Register destination) {
  int bits = registerSameBits(source, destination);
  uint8_t opcode = (bits == 8) ? r8 : r16;

  self->emitPrefix(source, destination);
  self->append(opcode, regreg(source, destination));
}

template<typename Imm, Opcode r8, Opcode r16, uint8_t group>
static void genericImmToReg(Section *self, Imm immediate, Register destination) {
  int regBits = registerBits(destination);
  int valBits = (regBits > 32) ? 32 : regBits;

  uint8_t opcode = (regBits == 8) ? r8 : r16;

  self->emitPrefix(destination);
  self->append(opcode, reg(group, destination));
  self->appendImmediate(static_cast<uint64_t>(immediate), valBits);
}

void Section::emitAdd(Register source, Register destination, bool withCarry) {
  if (withCarry) {
    genericRegToReg<ADC_RegMem8_Reg8, ADC_RegMem16_Reg16>(this, source, destination);
  } else {
    genericRegToReg<ADD_RegMem8_Reg8, ADD_RegMem16_Reg16>(this, source, destination);
  }
}

void Section::emitAdd(int32_t immediate, Register destination, bool withCarry) {
  if (withCarry) {
    genericImmToReg<int32_t, ADD_RegMem8_imm8, ADD_RegMem16_imm16, 2>(this, immediate, destination);
  } else {
    genericImmToReg<int32_t, ADD_RegMem8_imm8, ADD_RegMem16_imm16, 0>(this, immediate, destination);
  }
}

void Section::emitAnd(Register source, Register destination) {
  genericRegToReg<AND_RegMem8_Reg8, AND_RegMem16_Reg16>(this, source, destination);
}

void Section::emitAnd(uint32_t immediate, Register destination) {
  genericImmToReg<uint32_t, AND_RegMem8_imm8, AND_RegMem16_imm16, 4>(this, immediate, destination);
}

void Section::emitBt(uint8_t immediate, Register source) {
  if (registerBits(source) == 8) {
    throw std::invalid_argument("emitBt: Source register can't be 8-Bit");
  }

  this->emitPrefix(source);
  this->append(BT_RegMem16_imm8, reg(4, source), immediate);
}

void Section::emitCall(Register destination) {
  this->emitPrefix(destination);
  this->append(CALL_Near_regmem64, reg(2, destination));
}

void Section::emitCall(const MemReg &symbol) {
  if (symbol.isComputed()) {
    this->emitPrefix(symbol.base);
    this->append(CALL_Near_regmem16);
    this->emitSuffix(symbol, NoRegister, 2);
  } else {
    this->append(CALL_Near_rel32off, uint32_t(0));
    this->addRipRef(symbol, -4, sizeof(uint32_t));
  }
}

static uint8_t cmpRegImmOpcode(int bits) {
  switch (bits) {
  case 64: return CMP_RegMem64_imm32;
  case 32: return CMP_RegMem32_imm32;
  case 16: return CMP_RegMem16_imm16;
  default: return CMP_RegMem8_imm8;
  }
}

void Section::emitCmp(Register left, int32_t right) {
  int regBits = registerBits(left);
  int immBits = immediateBits(right);

  if (left == AL && immBits == 8) {
    this->append(CMP_AL_imm8, static_cast<uint8_t>(right));
  } else if (left == AX && immBits >= 16) {
    this->append(CMP_AX_imm16, static_cast<uint16_t>(right));
  } else if (left == EAX && immBits >= 32) {
    this->append(CMP_EAX_imm32, static_cast<uint32_t>(right));
  } else {
    this->emitPrefix(left);
    this->append(cmpRegImmOpcode(regBits), reg(7, left));
    this->appendImmediate(static_cast<uint64_t>(right), (regBits == 64) ? 32 : regBits);
  }
}

static uint8_t cmpRegMemRegOpcode(int bits) {
  switch (bits) {
  case 64: return CMP_RegMem64_Reg64;
  case 32: return CMP_RegMem32_Reg32;
  case 16: return CMP_RegMem16_Reg16;
  default: return CMP_RegMem8_Reg8;
  }
}

void Section::emitCmp(Register left, Register right) {
  uint8_t opcode = cmpRegMemRegOpcode(registerSameBits(left, right));
  this->emitPrefix(left, right);
  this->append(opcode, regreg(left, right));
}

void Section::emitEnter(uint16_t stackSpace, bool nestedFrame) {
  uint8_t imm8 = nestedFrame ? 1 : 0;
  this->append(ENTER_imm16_imm8, stackSpace, imm8);
}

void Section::emitInc(Register regi) {
  uint8_t opcode = (registerBits(regi) == 8) ? INC_RegMem8 : INC_RegMem16;
  this->emitPrefix(regi);
  this->append(opcode, reg(0, regi));
}

void Section::emitInc(const MemReg &address, int bitSize) {
  address.throwIfValue();

  Opcode opcode = (bitSize == 8) ? INC_RegMem8 : INC_RegMem16;
  this->emitGeneric(opcode, 0, address, bitSize);
}

void Section::emitDec(Register regi) {
  uint8_t opcode = (registerBits(regi) == 8) ? DEC_RegMem8 : DEC_RegMem16;
  this->emitPrefix(regi);
  this->append(opcode, reg(1, regi));
}

void Section::emitDec(const MemReg &address, int bitSize) {
  address.throwIfValue();

  Opcode opcode = (bitSize == 8) ? DEC_RegMem8 : DEC_RegMem16;
  this->emitGeneric(opcode, 1, address, bitSize);
}

void Section::emitJccPrefix(Condition cond, int bits) {
  uint8_t base = (bits < 16) ? 0x70 : 0x80;
  uint8_t opcode = base + static_cast<uint8_t>(cond);

  if (bits == 16) this->append(OperandSizeOverride);
  if (bits >= 16) this->append(uint8_t(0x0F));
  this->append(opcode);
}

void Section::emitJcc(Condition cond, int32_t displacement) {
  int bits = immediateBits(displacement);
  this->emitJccPrefix(cond, bits);
  this->appendImmediate(static_cast<uint64_t>(displacement), bits);
}

void Section::emitJcc(Condition cond, const MemReg &destination) {
  this->emitJccPrefix(cond, 32);
  this->append(uint32_t(0));
  this->addRipRef(destination, -4, sizeof(uint32_t));
}

static uint8_t jmpNearRegMemOpcode(int bits) {
  switch (bits) {
  case 8: throw std::invalid_argument("emitJmp: Can't JMP to 8-Bit register or memory address");
  case 16: return JMP_Near_RegMem16;
  case 32: return JMP_Near_RegMem32;
  case 64: return JMP_Near_RegMem64;
  default: throw std::runtime_error("jmpNearRegMemOpcode: BUG");
  }
}

void Section::emitJmp(const MemReg &destination) {
  if (destination.isComputed()) {
    this->emitPrefix(destination.base);
    this->append(JMP_Near_RegMem16);
    this->emitSuffix(destination, NoRegister, 4);
  } else {
    this->append(JMP_Near_rel32off, uint32_t(0));
    this->addRipRef(destination, -4, sizeof(uint32_t));
  }
}

void Section::emitJmp(Register destination) {
  uint8_t opcode = jmpNearRegMemOpcode(registerBits(destination));

  if (opcode == JMP_Near_RegMem32) {
    throw std::invalid_argument("emitJmp: Can't JMP to a 32-Bit register");
  }

  this->emitPrefix(destination);
  this->append(opcode, reg(4, destination));
}

void Section::emitJmp(int32_t displacement) {
  int bits = immediateBits(displacement);
  uint8_t opcode = JMP_Near_rel8off;

  if (bits == 16) opcode = JMP_Near_rel16off;
  else if (bits == 32) opcode = JMP_Near_rel32off;

  //
  this->append(opcode);
  this->appendImmediate(static_cast<uint64_t>(displacement), bits);
}

void Section::emitLeave() {
  this->append(LEAVE);
}

void Section::emitMov(uint64_t immediate, Register destination) {
  uint8_t opcode = (registerBits(destination) == 8) ? MOV_Reg8_imm8 : MOV_Reg16_imm16;

  this->emitPrefix(destination);
  this->append(static_cast<uint8_t>(opcode + registerIndex(destination)));
  this->appendImmediate(immediate, registerBits(destination));
}

void Section::emitMov(Register source, Register destination) {
  uint8_t opcode = (registerBits(source) == 8) ? MOV_RegMem8_Reg8 : MOV_RegMem16_Reg16;
  registerSameBits(source, destination);

  this->emitPrefix(source, destination);
  this->append(opcode, regreg(source, destination));
}

void Section::emitMov(Register source, const MemReg &destination) {
  Opcode opcode = (registerBits(source) == 8) ? MOV_RegMem8_Reg8 : MOV_RegMem16_Reg16;

  if (destination.isImmediate()) {
    throw std::invalid_argument("Can't store into an immediate destination");
  }

  this->emitGeneric(opcode, source, destination);
}

void Section::emitMov(const MemReg &source, Register destination) {
  int bits = registerBits(destination);

  if (source.dereference) {
    Opcode opcode = (bits == 8) ? MOV_Reg8_RegMem8 : MOV_Reg16_RegMem16;
    this->emitGeneric(opcode, destination, source);
  } else { // Treat as immediate value.
    int bytes = bits / 8;
    uint8_t opcode = (bits == 8) ? 0xB0 : 0xB8;

    this->emitPrefix(destination);
    this->append(static_cast<uint8_t>(opcode + registerIndex(destination)));
    this->appendImmediate(uint64_t(0), bits);
    this->addRef(source, -bytes, size_t(bytes));
  }
}

void Section::emitMovzx(Register source, Register destination) {
  int srcBits = registerBits(source);
  int dstBits = registerBits(destination);

  if (srcBits >= dstBits) {
    throw std::invalid_argument("emitMovzx: Source register must be smaller than destination register");
  }

  Opcode opcode;
  if (srcBits == 8) {
    switch (dstBits) {
    case 16:
      opcode = MOVZX_Reg16_RegMem8;
      break;
    case 32:
      opcode = MOVZX_Reg32_RegMem8;
      break;
    default:
      opcode = MOVZX_Reg64_RegMem8;
      break;
    }
  } else if (srcBits == 16) {
    opcode = (dstBits == 32) ? MOVZX_Reg32_RegMem16 : MOVZX_Reg64_RegMem16;
  } else {
    throw std::invalid_argument("Source register has to be 8 or 16-bit wide");
  }

  this->emitPrefix(destination, source, false);
  this->append(opcode, regreg(destination, source));
}

void Section::emitOr(Register source, Register destination) {
  genericRegToReg<OR_RegMem8_Reg8, OR_RegMem16_Reg16>(this, source, destination);
}

void Section::emitOr(uint32_t immediate, Register destination) {
  genericImmToReg<uint32_t, OR_RegMem8_imm8, OR_RegMem16_imm16, 1>(this, immediate, destination);
}

void Section::emitRet(uint16_t popBytes) {
  if (popBytes > 0) {
    this->append(RET_Near_imm16, popBytes);
  } else {
    this->append(RET_Near);
  }
}

void Section::emitPrefix(Register first, Register second, bool sameSize) {
  bool w = false, r = false, x = false, b = false;
  int bits = registerBits(first);
  bool fe = registerExtended(first);
  bool se = registerExtended(second);

  // Special rule: Impossible to address `%xH` and an extended register at once.
  checkRexAndHigh8BitRegisters(first, second);

  if (sameSize) {
    registerSameBits(first, second);
  }

  if (bits == 16) {
    this->append(OperandSizeOverride);

    if (!fe && !se) { // The legacy word registers don't want the REX prefix though
      return;
    }
  }

  //
  w = (bits == 64);
  r |= fe;
  b |= se;

  // Only emit REX.? if required
  if ((first >= SPL && first <= DIL) || (second >= SPL && second <= DIL) || w || r || b) {
    this->emitRex(w, r, x, b);
  }
}

struct RexPrefix { bool use, w, b; };

static RexPrefix computeRexPrefix(Register toUse) {
  bool w = false, b = false;

  if (toUse >= SPL && toUse <= DIL) {
    // Requires REX (Without further flags)
  } else if (toUse >= Bit8Start && toUse <= Bit8Last) {
    // Usually not addressable with REX
    if (!registerExtended(toUse)) return { false, false, false };
  } else if (toUse >= Bit16Start && toUse <= Bit16Last) {
    // The %RxW registers also require the REX prefix!
    // The legacy word registers don't want the REX prefix though
    if (toUse >= R8W && toUse <= R15W) {
      return { true, false, true };
    } else {
      return { false, false, false };
    }
  }

  // 64-Bit registers require the Wide flag:
  w = (toUse >= Bit64Start && toUse <= Bit64Last);

  // Make sure we set the high-bit for registers R8x..R15x
  if ((toUse >= R8 && toUse <= R15) ||
      (toUse >= R8D && toUse <= R15D) ||
      (toUse >= R8W && toUse <= R15W) ||
      (toUse >= R8B && toUse <= R15B)) {
    b = true;
  }

  return { true, w, b };
}

void Section::emitPrefix(Register reg, const MemReg &ref) {
  if (ref.isImmediate()) {
    this->emitPrefix(reg);
    return;
  }

  // Disallow AH/BH/CH/DH with extended registers for addressing:
  checkRexAndHigh8BitRegisters(reg, ref.base);
  if (ref.index != NoRegister) checkRexAndHigh8BitRegisters(reg, ref.index);

  if (registerBits(ref.base) == 32) { this->append(AddressSizeOverride); }

  // Another rule: The 16-Bit registers (AX, BX, CX, DX, ...) require their
  // own prefix.
  if (reg >= Bit16Start && reg <= Bit16Last) {
    this->append(OperandSizeOverride);
  }

  //
  auto prefix = computeRexPrefix(reg);
  bool b = registerExtended(ref.base);
  bool x = registerExtended(ref.index);

  if (prefix.use || b || x) {
    this->emitRex(prefix.w, /* r = */ prefix.b, x, b);
  }
}

void Section::emitPrefix(const MemReg &ref, int bits) {
  if (ref.isImmediate()) {
    this->emitPrefix(bits);
    return;
  }

  if (registerBits(ref.base) == 32) {
    this->append(AddressSizeOverride);
  }

  bool w = false, b = false, r = false, x = false;
  if (bits == 64) {
    w = true;
  } else if (bits == 16) {
    this->append(OperandSizeOverride);
  }

  //
  b = registerExtended(ref.base);
  x = registerExtended(ref.index);

  if (r || w || b || x) {
    this->emitRex(w, r, x, b);
  }
}

void Section::emitPrefix(Register toUse) {
  auto prefix = computeRexPrefix(toUse);

  if (toUse >= Bit16Start && toUse <= Bit16Last) {
      this->append(OperandSizeOverride);
  }

  if (prefix.use)
    this->emitRex(prefix.w, false, false, prefix.b);
}

void Section::emitPrefix(int bits) {
  if (bits == 16) this->append(OperandSizeOverride);
  else if (bits == 64) this->emitRex(true, false, false, false);
}

template<Opcode r8imm1, Opcode r16imm1, Opcode r8imm8, Opcode r16imm8, uint8_t group>
static void emitShiftImm(Section *self, uint8_t immediate, Register destination) {
  int bits = registerBits(destination);

  if (immediate == 1) {
    uint8_t opcode = (bits == 8) ? r8imm1 : r16imm1;
    self->emitPrefix(destination);
    self->append(opcode, reg(group, destination));
  } else {
    uint8_t opcode = (bits == 8) ? r8imm8 : r16imm8;
    self->emitPrefix(destination);
    self->append(opcode, reg(group, destination), immediate);
  }
}

template<Opcode r8, Opcode r16, uint8_t group>
static void emitShiftCl(Section *self, Register destination) {
  int bits = registerBits(destination);
  uint8_t opcode = (bits == 8) ? r8 : r16;

  self->emitPrefix(destination);
  self->append(opcode, reg(group, destination));
}

void Section::emitRcl(uint8_t immediate, Register destination) {
  emitShiftImm<
      RCL_RegMem8_1, RCL_RegMem16_1, RCL_RegMem8_imm8, RCL_RegMem16_imm8, 2
  >(this, immediate, destination);
}

void Section::emitRclCl(Register destination) {
  emitShiftCl<RCL_RegMem8_CL, RCL_RegMem16_CL, 2>(this, destination);
}

void Section::emitRcr(uint8_t immediate, Register destination) {
  emitShiftImm<
      RCR_RegMem8_1, RCR_RegMem16_1, RCR_RegMem8_imm8, RCR_RegMem16_imm8, 3
  >(this, immediate, destination);
}

void Section::emitRcrCl(Register destination) {
  emitShiftCl<RCR_RegMem8_CL, RCR_RegMem16_CL, 3>(this, destination);
}

void Section::emitRex(bool w, bool r, bool x, bool b) {
  uint8_t byte = static_cast<uint8_t>(RexField::Prefix);
  if (w) byte |= static_cast<uint8_t>(RexField::W);
  if (r) byte |= static_cast<uint8_t>(RexField::R);
  if (x) byte |= static_cast<uint8_t>(RexField::X);
  if (b) byte |= static_cast<uint8_t>(RexField::B);

  this->append(byte);
}

void Section::emitRol(uint8_t immediate, Register destination) {
  emitShiftImm<
      ROL_RegMem8_1, ROL_RegMem16_1, ROL_RegMem8_imm8, ROL_RegMem16_imm8, 0
  >(this, immediate, destination);
}

void Section::emitRolCl(Register destination) {
  emitShiftCl<ROL_RegMem8_CL, ROL_RegMem16_CL, 0>(this, destination);
}

void Section::emitRor(uint8_t immediate, Register destination) {
  emitShiftImm<
      ROR_RegMem8_1, ROR_RegMem16_1, ROR_RegMem8_imm8, ROR_RegMem16_imm8, 1
  >(this, immediate, destination);
}

void Section::emitRorCl(Register destination) {
  emitShiftCl<ROR_RegMem8_CL, ROR_RegMem16_CL, 1>(this, destination);
}

void Section::emitShl(uint8_t immediate, Register destination) {
  emitShiftImm<
      SHL_RegMem8_1, SHL_RegMem16_1, SHL_RegMem8_imm8, SHL_RegMem16_imm8, 4
  >(this, immediate, destination);
}

void Section::emitShlCl(Register destination) {
  emitShiftCl<SHL_RegMem8_CL, SHL_RegMem16_CL, 4>(this, destination);
}

void Section::emitShr(uint8_t immediate, Register destination) {
  emitShiftImm<
      SHR_RegMem8_1, SHR_RegMem16_1, SHR_RegMem8_imm8, SHR_RegMem16_imm8, 5
  >(this, immediate, destination);
}

void Section::emitShrCl(Register destination) {
  emitShiftCl<SHR_RegMem8_CL, SHR_RegMem16_CL, 5>(this, destination);
}

void Section::emitSetcc(Condition cond, Register destination) {
  if (registerBits(destination) != 8) {
    throw std::invalid_argument("emitSetcc: Destination register must be 8-Bit in size");
  }

  uint8_t opcode = 0x90 + cond;
  this->emitPrefix(destination);
  this->append(uint8_t(0x0F), opcode, reg(0, destination));
}

void Section::emitSetcc(Condition cond, const MemReg &destination) {
  uint8_t opcode = 0x90 + cond;
  this->append(uint8_t(0x0F), opcode, modrm(MOD_MEM, 0, 0b100), uint32_t(0));
  this->addRipRef(destination, -4, sizeof(uint32_t));
}

void Section::emitSub(Register source, Register destination, bool withBorrow) {
  if (withBorrow) {
    genericRegToReg<SBB_RegMem8_Reg8, SBB_RegMem16_Reg16>(this, source, destination);
  } else {
    genericRegToReg<SUB_RegMem8_Reg8, SUB_RegMem16_Reg16>(this, source, destination);
  }
}

void Section::emitSub(int32_t immediate, Register destination, bool withBorrow) {
  if (withBorrow) {
    genericImmToReg<int32_t, SBB_RegMem8_imm8, SBB_RegMem16_imm16, 3>(this, immediate, destination);
  } else {
    genericImmToReg<int32_t, SUB_RegMem8_imm8, SUB_RegMem16_imm16, 5>(this, immediate, destination);
  }
}

void Section::emitSuffix(const MemReg &memory, Register reg, uint8_t group) {
  int dispBits = 0;
  uint8_t mod = MOD_MEM;
  uint8_t rm = (memory.index == NoRegister) ? RIP_RELATIVE : HAS_SIB_BYTE;
  uint8_t regNo = registerIndex(reg);
  bool needsReference = false;

  if (memory.displacement != 0) { // MOV 0x1234, %rax
    dispBits = immediateBits(memory.displacement);
    if (dispBits != 8) {
      dispBits = 32;
      mod = MOD_MEM_DISP32;
    } else {
      mod = MOD_MEM_DISP8;
    }
  } else if (!memory.name.empty()) { // MOV helloStr, %rax
    dispBits = 32;
    mod = MOD_MEM_DISP32;
    needsReference = true;
  } else if (memory.base == NoRegister) { // ?!
    throw std::invalid_argument("No displacement and no base register given");
  } else if (memory.index == NoRegister) {
    rm = registerIndex(memory.base);
    regNo = group;
  } else if (memory.index != NoRegister) { // Base+Index is set without a displacement
    mod = MOD_MEM_DISP8; // Use a 8-Bit zero displacement
    dispBits = 8;
  }

  // Emit ModR/M Byte
  this->append(modrm(mod, regNo, rm));

  // Do we need a SIB Byte?
  if (memory.index != NoRegister) {
    this->append(sib(memory.base, memory.index, memory.scale));
  }

  // Do we need a displacement byte sequence?
  if (dispBits > 0) {
    this->appendImmediate(static_cast<uint64_t>(memory.displacement), dispBits);

    if (needsReference) { // Need a reference too?
      this->addRipRef(memory, -(dispBits / 8), size_t(dispBits / 8));
    }
  }
}

void Section::emitTest(Register source, Register destination) {
  genericRegToReg<TEST_RegMem8_Reg8, TEST_RegMem16_Reg16>(this, source, destination);
}

void Section::emitTest(uint32_t immediate, Register destination) {
  genericImmToReg<uint32_t, TEST_RegMem8_imm8, TEST_RegMem16_imm16, 4>(this, immediate, destination);
}

void Section::emitXor(Register source, Register destination) {
  genericRegToReg<XOR_RegMem8_Reg8, XOR_RegMem16_Reg16>(this, source, destination);
}

void Section::emitXor(uint32_t immediate, Register destination) {
  genericImmToReg<uint32_t, XOR_RegMem8_imm8, XOR_RegMem16_imm16, 6>(this, immediate, destination);
}

MemReg::MemReg(const Section &disp, Register base, Register index, uint8_t scale)
  : MemReg(disp.name, true, 0, base, index, scale)
{
  //
}

MemReg MemReg::value(const Section &symbol) {
  return MemReg(symbol.name, false, 0, NoRegister, NoRegister, 1);
}

void MemReg::throwIfValue() const {
  if (!this->dereference) {
    throw std::invalid_argument("Mem/Reg reference must dereferenced for this instruction");
  }
}

void MemReg::sanityCheck() {
  if (scale != 1 && scale != 2 && scale != 4 && scale != 8) {
    throw std::invalid_argument("Addressing scale has to be one of 1, 2, 4 or 8");
  }

  if (base == NoRegister && index != NoRegister) {
    throw std::invalid_argument("A base register is required if an index register is used");
  }

  if (base != NoRegister && index != NoRegister) {
    if (registerSameBits(base, index) < 32) {
      throw std::invalid_argument("Base and index registers must be 32 or 64-bits wide");
    }
  }
}

}
