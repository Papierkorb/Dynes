#include <amd64/memorytranslator.hpp>
#include <amd64/constants.hpp>

#include <cpu/memory.hpp>

namespace Amd64 {
static const MemReg MEMORY_PTR = MemReg::value("Memory");
static const MemReg STACK_PTR = MemReg::value("Stack");
static const MemReg RAM_PTR = MemReg::value("Ram");
static const MemReg CURRENT_STACK_PTR(ADDRR, SR);

MemoryTranslator::MemoryTranslator(Section &sec) : m_sec(sec) { }

static void indirectCall(Section &sec, std::string symbol) {
  sec.emitMov(MemReg::value(symbol), RAX);
  sec.emitCall(RAX);
}

/**
 * Returns \c true if the memory access is guaranteed to happen in RAM.  In this
 * case we can directly access the byte without going through the \c Cpu::Memory
 * mapping.
 */
static bool guaranteedToStayInRam(Core::Instruction::Addressing mode, uint16_t addr) {
  using ::Core::Instruction;

  switch(mode) {
  default:
    return false;
  case Instruction::Zp:
  case Instruction::ZpX:
  case Instruction::ZpY:
    // These are guaranteed to access [0x00, 0xFF], which is always in RAM:
    return true;
  case Instruction::Abs:
    // Absolute addresses may stay in RAM:
    return (addr < Cpu::Memory::RAM_BARRIER);
  case Instruction::AbsX:
  case Instruction::AbsY:
    // The absolute XY variants can also stay in RAM, but only if we can
    // proof that it absolutely WILL do so:
    return ((addr + 0xFF) < Cpu::Memory::RAM_BARRIER);
  case Instruction::Ind:
  case Instruction::IndX:
  case Instruction::IndY:
    // Indirect accesses could stay in RAM.  But they're not that common, and
    // have additional rules, so just use the slow-path for these.
    return false;
  }
}

void MemoryTranslator::resolve(const Core::Instruction &instr, Register destination) {
  return this->resolve(instr.addressing, instr.op16, destination);
}

void MemoryTranslator::resolve(Core::Instruction::Addressing mode, uint16_t addr, Register destination) {
  using ::Core::Instruction;
  uint8_t addr8 = static_cast<uint8_t>(addr);

  switch(mode) {
  default:
    throw std::runtime_error("Memory::resolve: Unresolvable memory access.");
  case Instruction::Zp: // return Op & 0x00FF
    this->m_sec.emitMov(addr & 0x00FF, destination);
    break;
  case Instruction::ZpX: // return (X + Op8) & 0x00FF
    this->m_sec.emitMovzx(X, UX);
    this->m_sec.emitMov(UX, destination);
    this->m_sec.emitAdd(addr8, destination);
    this->m_sec.emitAnd(uint16_t(0x00FF), destination);
    break;
  case Instruction::ZpY: // return (Y + Op8) & 0x00FF
    this->m_sec.emitMovzx(Y, destination);
    this->m_sec.emitAdd(addr8, destination);
    this->m_sec.emitAnd(uint16_t(0x00FF), destination);
    break;
  case Instruction::Abs:
    this->m_sec.emitMov(addr, destination);
    break;
  case Instruction::AbsX: // return X + Op
    this->m_sec.emitMov(addr, destination);
    this->m_sec.emitMovzx(X, UX);
    this->m_sec.emitAdd(UX, destination);
    break;
  case Instruction::AbsY: // return Y + Op
    this->m_sec.emitMov(addr, destination);
    this->m_sec.emitAdd(YX, destination);
    break;
  case Instruction::Ind: // return Memory->read16(Op)
    this->m_sec.emitMov(MEMORY_PTR, ARG_1);
    this->m_sec.emitMov(addr, ARG_2);
    indirectCall(this->m_sec, "read16");
    if (destination != RESULT16) this->m_sec.emitMov(RESULT16, destination);
    break;
  case Instruction::IndX: // return Memory->read16((Op + X) & 0x00FF)
    this->m_sec.emitMovzx(X, ARG_2);
    this->m_sec.emitAdd(addr8, ARG_2);
    this->m_sec.emitAnd(uint16_t(0x00FF), ARG_2);
    this->m_sec.emitMov(MEMORY_PTR, ARG_1);
    indirectCall(this->m_sec, "read16");
    if (destination != RESULT16) this->m_sec.emitMov(RESULT16, destination);
    break;
  case Instruction::IndY: // return Memory->read16(Op8) + Y
    this->m_sec.emitMov(MEMORY_PTR, ARG_1);
    this->m_sec.emitMov(addr8, ARG_2);
    indirectCall(this->m_sec, "read16");
    this->m_sec.emitAdd(YX, RESULT16);
    if (destination != RESULT16) this->m_sec.emitMov(RESULT16, destination);
    break;
  }
}

Register MemoryTranslator::read(const Core::Instruction &instr) {
  return this->read(instr.addressing, instr.op16);
}

Register MemoryTranslator::read(Core::Instruction::Addressing mode, uint16_t addr) {
  using Core::Instruction;

  switch(mode) {
  case Instruction::Acc: return A;
  case Instruction::X: return X;
  case Instruction::Y: return Y;
  case Instruction::S: return S;
  case Instruction::P: return P;
  case Instruction::Imm:
  case Instruction::Imp:
  case Instruction::Rel:
    this->m_sec.emitMov(addr & 0x00FF, MEML);
    return MEML;
  default: { // Resolve and read from memory.
    this->resolve(mode, addr, ARG_2);

    if (guaranteedToStayInRam(mode, addr)) {
      this->m_sec.emitMov(RAM_PTR, ARG_1);
      this->m_sec.emitAnd(Cpu::Memory::RAM_SIZE - 1, ARG_2R);
      this->m_sec.emitMov(MemReg(ARG_1, ARG_2R), MEML);
      return MEML;
    } else {
      this->m_sec.emitMov(MEMORY_PTR, ARG_1);
      indirectCall(this->m_sec, "read");
      return RESULT8;
    }
  }
  }
}

void MemoryTranslator::write(const Core::Instruction &instr, Register source) {
  this->write(instr.addressing, instr.op16, source);
}

void MemoryTranslator::write(Core::Instruction::Addressing mode, uint16_t addr, Register source) {
  using Core::Instruction;

  switch(mode) {
  case Instruction::Acc:
    if (source != A) this->m_sec.emitMov(source, A);
    break;
  case Instruction::X:
    if (source != X) this->m_sec.emitMov(source, X);
    break;
  case Instruction::Y:
    if (source != Y) this->m_sec.emitMov(source, Y);
    break;
  case Instruction::S:
    if (source != S) this->m_sec.emitMov(source, S);
    break;
  case Instruction::P:
    if (source != P) this->m_sec.emitMov(source, P);
    break;
  case Instruction::Imm:
  case Instruction::Imp:
  case Instruction::Rel:
    throw std::runtime_error("Can't write to Imm/Imp/Rel addressing instruction");
  default: // Resolve and write to memory.
    this->resolve(mode, addr, ARG_2);

    if (guaranteedToStayInRam(mode, addr)) {
      this->m_sec.emitMov(RAM_PTR, ARG_1);
      this->m_sec.emitAnd(Cpu::Memory::RAM_SIZE - 1, ARG_2R);
      this->m_sec.emitMov(source, MemReg(ARG_1, ARG_2R));
    } else {
      this->m_sec.emitMov(MEMORY_PTR, ARG_1);
      if (source != ARG_3) this->m_sec.emitMov(source, ARG_3);
      indirectCall(this->m_sec, "write");
    }

    break;
  }
}

void MemoryTranslator::rmw(const Core::Instruction &instr, std::function<Register (Register)> proc) {
  rmw(instr.addressing, instr.op16, proc);
}

void MemoryTranslator::rmw(Core::Instruction::Addressing mode, uint16_t addr, std::function<Register (Register)> proc) {
  using Core::Instruction;
  Register source, destination;

  switch(mode) {
  case Instruction::Acc:
    source = destination = A;
    break;
  case Instruction::X:
    source = destination = X;
    break;
  case Instruction::Y:
    source = destination = Y;
    break;
  case Instruction::S:
    source = destination = S;
    break;
  case Instruction::P:
    source = destination = P;
    break;
  case Instruction::Imm:
    source = MEML;
    destination = A;
    this->m_sec.emitMov(static_cast<uint8_t>(addr), MEML);
    break;
  case Instruction::Rel:
  case Instruction::Imp:
    throw std::runtime_error("Can't RMW on a Rel/Imp adressing instruction");
  default: { // Read and write to memory.
    this->resolve(mode, addr, ADDR);

    if (guaranteedToStayInRam(mode, addr)) {
      this->m_sec.emitMov(RAM_PTR, ARG_1);
      this->m_sec.emitAnd(Cpu::Memory::RAM_SIZE - 1, ADDRR);
      this->m_sec.emitMov(MemReg(ARG_1, ADDRR), MEML);

      Register result = proc(MEML);

      this->m_sec.emitMov(RAM_PTR, ARG_1);
      this->m_sec.emitMov(result, MemReg(ARG_1, ADDRR));
    } else {
      this->m_sec.emitMov(ADDR, ARG_2);
      this->m_sec.emitMov(MEMORY_PTR, ARG_1);
      indirectCall(this->m_sec, "read");

      Register result = proc(RESULT8);

      this->m_sec.emitMov(MEMORY_PTR, ARG_1);
      this->m_sec.emitMov(ADDR, ARG_2);
      if (result != ARG_3) this->m_sec.emitMov(result, ARG_3);
      indirectCall(this->m_sec, "write");
    }

    return;
  }
  }

  // Non-memory accessing RMW access.  Handle these one generically.
  Register result = proc(source);
  if (result != destination) {
    this->m_sec.emitMov(result, destination);
  }
}

void MemoryTranslator::push8(Register source) {
  this->m_sec.emitMov(STACK_PTR, ADDRR);
  this->m_sec.emitMov(source, CURRENT_STACK_PTR);
  this->m_sec.emitDec(S);
}

void MemoryTranslator::push16(Register source) {
  if (source != WX) this->m_sec.emitMov(source, WX);
  this->m_sec.emitMov(STACK_PTR, ADDRR);

  this->m_sec.emitRor(8, WX); // High-Byte first!
  this->m_sec.emitMov(WL, CURRENT_STACK_PTR);
  this->m_sec.emitDec(S);

  this->m_sec.emitShr(8, WX); // Low-Byte second.
  this->m_sec.emitMov(WL, CURRENT_STACK_PTR);
  this->m_sec.emitDec(S);
}

void MemoryTranslator::pull8(Register destination) {
  this->m_sec.emitInc(S);
  this->m_sec.emitMov(STACK_PTR, ADDRR);
  this->m_sec.emitMov(CURRENT_STACK_PTR, destination);
}

void MemoryTranslator::pull16(Register destination) {
  // Manually pull 2x8-Bit to preserve loop-around (RTS with S = 0xFE)
  this->m_sec.emitMov(STACK_PTR, ADDRR);

  this->m_sec.emitInc(S);
  this->m_sec.emitMov(CURRENT_STACK_PTR, MEML); // Lower Byte

  this->m_sec.emitInc(S);
  this->m_sec.emitMov(MEML, MEMH);
  this->m_sec.emitMov(CURRENT_STACK_PTR, MEML); // Upper Byte
  this->m_sec.emitRor(8, MEMX);

  if (MEMX != destination) this->m_sec.emitMov(MEMX, destination);
}

}
