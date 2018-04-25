#include <amd64/instructiontranslator.hpp>
#include <amd64/memorytranslator.hpp>
#include <amd64/constants.hpp>

#include <cpu.hpp>
#include <cpu/state.hpp>

#include <functional>


/************************* DEBUG FUNCTIONALITY FLAGS **************************/

// Print out instructions while they're being translated
//#define TRACE_INSTRUCTIONS

// Add a dummy `MOV %0xABCD, %AX` at the start of a translated instruction.
// This allows to tell the original address from a disassembly.
//#define MARK_INSTRUCTIONS

// Print out instructions after they ran.  Branching instructions (JSR, RET, ..)
// will print BEFORE they ran.  This generates a huge log.
//#define LOG_INSTRUCTIONS

/*************************                           **************************/

namespace Amd64 {
InstructionTranslator::InstructionTranslator(Section &section)
  : m_sec(section)
{

}

void InstructionTranslator::traceInstruction(uint16_t address, ::Core::Instruction instr) {
  (void)address, (void)instr;
#ifdef TRACE_INSTRUCTIONS
  fprintf(stderr, "[%04x] %s %s", address, instr.commandName(), instr.addressingName());

  switch (instr.operandSize()) {
  case 1:
    fprintf(stderr, " %02x\n", instr.op8);
    break;
  case 2:
    fprintf(stderr, " %02x %02x\n", (instr.op16 >> 8) & 0xFF, instr.op16 & 0xFF);
    break;
  default:
    fprintf(stderr, "\n");
  }
#endif

#ifdef MARK_INSTRUCTIONS
  // Create useless MOV so one can match the 6502 instruction to the AMD64
  // sequence in a disassembly dump.
  this->m_sec.emitMov(address, RESULT16);
#endif
}

void InstructionTranslator::logInstruction(uint16_t address, Core::Instruction instr) {
#ifdef LOG_INSTRUCTIONS
  this->m_sec.emitMov(MemReg::value("log"), RAX);
  this->m_sec.emitMov(MemReg::value("State"), ARG_1);
  this->m_sec.emitMov(static_cast<uint8_t>(instr.command), ARG_2L);
  this->m_sec.emitMov(static_cast<uint8_t>(instr.addressing), ARG_3);
  this->m_sec.emitMov(instr.op16, VX);
  this->m_sec.emitMov(address, PC);
  this->m_sec.emitCall(RAX);
#else
  (void)address, (void)instr;
#endif
}

std::pair<bool, uint16_t> InstructionTranslator::translate(uint16_t address, const Analysis::Branch::Instruction &instr) {
  if (const Core::Instruction *ptr = std::get_if<Core::Instruction>(&instr)) {
    auto result = this->translate(address, *ptr);
    if (result.first) this->logInstruction(address, *ptr);
    return result;
  } else {
    this->translate(address, std::get<Analysis::ConditionalInstruction>(instr));
    return { false, 0 };
  }
}

static std::string branchSectionName(const Analysis::Branch *branch) {
  return "instr_" + std::to_string(branch->start());
}

static std::pair<Cpu::Flag, bool> branchCommandToFlag(Core::Instruction::Command command) {
  using Core::Instruction;

  switch (command) {
  case Instruction::BCC: return { Cpu::Flag::Carry, false };
  case Instruction::BCS: return { Cpu::Flag::Carry, true };
  case Instruction::BEQ: return { Cpu::Flag::Zero, true };
  case Instruction::BNE: return { Cpu::Flag::Zero, false };
  case Instruction::BMI: return { Cpu::Flag::Negative, true };
  case Instruction::BPL: return { Cpu::Flag::Negative, false };
  case Instruction::BVS: return { Cpu::Flag::Overflow, true };
  case Instruction::BVC: return { Cpu::Flag::Overflow, false };
  default:
    throw std::runtime_error("Missing case for conditional instruction!");
  }
}

void InstructionTranslator::translate(uint16_t address, Analysis::ConditionalInstruction instr) {
  using Core::Instruction;

  auto branchFlag = branchCommandToFlag(instr.command);
  std::string truthy = branchSectionName(instr.trueBranch());
  std::string falsy = branchSectionName(instr.falseBranch());

  this->traceInstruction(address, instr);
  Condition cond = (branchFlag.second) ? Carry : NotCarry;

  // Cycle-exhaustion check:
  this->m_sec.emitMov(static_cast<uint8_t>(Cpu::State::Reason::CyclesExhausted), REASON);
  this->m_sec.emitMov(address, PC);
  this->m_sec.emitCmp(CYCLES, 0);
  this->m_sec.emitJcc(GreaterOrEqual, 1);
  this->m_sec.emitRet(); //           ^ Skipped by this

  // Late-count cycles, so the check above doesn't take the cycles of this
  // instruction into account.  Else when we resume after exhaustion this
  // instruction would be counted twice.
  this->countCycles(instr.cycles);
  this->logInstruction(address, instr);

  // Perform the actual conditional branch:
  this->m_sec.emitBt(static_cast<uint8_t>(Cpu::flagBit(branchFlag.first)), PX);
  this->m_sec.emitJcc(cond, truthy);
  this->m_sec.emitJmp(falsy);
}

std::pair<bool, uint16_t> InstructionTranslator::translate(uint16_t address, ::Core::Instruction instr) {
  using Core::Instruction;
  using Cpu::Flag;
  using Cpu::State;

  MemoryTranslator memory(this->m_sec);
  uint16_t nextAddr = static_cast<uint16_t>(address + instr.operandSize() + 1);

  this->traceInstruction(address, instr);
  this->countCycles(instr.cycles);

  switch (instr.command) {
  case Instruction::ADC:
    this->adc(memory.read(instr));
    break;
  case Instruction::AND:
    this->m_sec.emitAnd(memory.read(instr), A);
    this->setNz();
    break;
  case Instruction::ASL:
    memory.rmw(instr, [this](Register reg) {
      this->m_sec.emitShl(1, reg);                     // SHL $1, %Reg
      this->m_sec.emitSetcc(Carry, VL);                // Rescue Carry flag
      this->m_sec.emitOr(reg, reg);
      this->setNz(static_cast<uint8_t>(Flag::Carry));  // %P <- NZ
      this->m_sec.emitOr(VL, P);
      return reg;
    });
    break;
  case Instruction::BIT: {
    uint8_t z = static_cast<uint8_t>(Cpu::flagBit(Flag::Zero));
    uint8_t v = static_cast<uint8_t>(Cpu::flagBit(Flag::Overflow));
    uint8_t n = static_cast<uint8_t>(Cpu::flagBit(Flag::Negative));
    uint8_t nv = static_cast<uint8_t>((1 << v) | (1 << n));
    uint8_t mask = static_cast<uint8_t>((1 << n) | (1 << v) | (1 << z));

    Register reg = memory.read(instr);
    this->m_sec.emitAnd(uint8_t(~mask), P);        // %P &= ~NVZ
    this->m_sec.emitTest(reg, A);                  // Is (%Value & %A) ..
    this->m_sec.emitSetcc(Zero, UL);               // .. equals to 0?
    this->m_sec.emitShl(z, UL);                    // Adjust for %P
    this->m_sec.emitOr(UL, P);                     // %P |= Z
    this->m_sec.emitAnd(nv, reg);                  // %Reg &= NV
    this->m_sec.emitOr(reg, P);                    // %P |= NV
    // The above works as conveniently the top-two bits are tested, which are
    // exactly the NV flags in the right order.  Thus we don't have to manually
    // check for the values, we can just copy them over into P.

    break;
  }
  case Instruction::BRK:
    this->logInstruction(address, instr);
    this->m_sec.emitMov(nextAddr, PC);
    this->returnToHost(State::Reason::Break, PC);
    return { false, nextAddr };
  case Instruction::CLC:
    this->updateFlag(Flag::Carry, false);
    break;
  case Instruction::CLD:
    this->updateFlag(Flag::Decimal, false);
    break;
  case Instruction::CLI:
    this->updateFlag(Flag::Interrupt, false);
    break;
  case Instruction::CLV:
    this->updateFlag(Flag::Overflow, false);
    break;
  case Instruction::CMP:
    this->compare(A, memory.read(instr));
    break;
  case Instruction::CPX:
    this->compare(X, memory.read(instr));
    break;
  case Instruction::CPY:
    this->compare(Y, memory.read(instr));
    break;
  case Instruction::DEC:
  case Instruction::DEX:
  case Instruction::DEY:
    memory.rmw(instr, [this](Register source) {
      this->m_sec.emitDec(source);
      this->setNz();
      return source;
    });
    break;
  case Instruction::EOR:
    this->m_sec.emitXor(memory.read(instr), A);
    this->setNz();
    break;
  case Instruction::INC:
  case Instruction::INX:
  case Instruction::INY:
    memory.rmw(instr, [this](Register source) {
      this->m_sec.emitInc(source);
      this->setNz();
      return source;
    });
    break;
  case Instruction::JMP:
    this->logInstruction(address, instr);

    // Prepare PC and reason
    memory.resolve(instr, PC);
    this->m_sec.emitMov(static_cast<uint8_t>(State::Reason::Jump), REASON);

    // Infinite loop detection:
    this->m_sec.emitCmp(PC, address); // Is PC == address?
    this->m_sec.emitSetcc(Equal, VL); // If so, then `%VL = 1`, else `%VL = 0`
    this->m_sec.emitAdd(VL, REASON);  // %Reason = %Reason + %VL
    // This saves us from using a branching instruction.

    // Return to host.
    this->m_sec.emitRet();

    return { false, nextAddr };
  case Instruction::JSR:
    this->logInstruction(address, instr);
    this->m_sec.emitMov(static_cast<uint16_t>(nextAddr - 1), WX);
    memory.push16(WX);
    this->m_sec.emitMov(instr.op16, PC);
    this->returnToHost(State::Reason::Jump, PC);
    return { false, nextAddr };
  case Instruction::LDA:
    this->m_sec.emitMov(memory.read(instr), A);
    this->m_sec.emitOr(A, A);
    this->setNz();
    break;
  case Instruction::LDX:
    this->m_sec.emitMov(memory.read(instr), X);
    this->m_sec.emitOr(X, X);
    this->setNz();
    break;
  case Instruction::LDY:
    this->m_sec.emitMov(memory.read(instr), Y);
    this->m_sec.emitOr(Y, Y);
    this->setNz();
    break;
  case Instruction::LSR:
    memory.rmw(instr, [this](Register reg) {
      this->m_sec.emitShr(1, reg);                     // SHR $1, %Reg
      this->m_sec.emitSetcc(Carry, VL);                // Rescue Carry flag
      this->setNz(static_cast<uint8_t>(Flag::Carry));  // %P <- NZ
      this->updateFlag(Flag::Carry, VL, true);         // %P <- %VL as Carry
      return reg;
    });
    break;
  case Instruction::NOP: /* Nothing. */ break;
  case Instruction::ORA:
    this->m_sec.emitOr(memory.read(instr), A);
    this->setNz();
    break;
  case Instruction::PHA:
    memory.push8(A);
    break;
  case Instruction::PHP: {
    uint8_t mask = static_cast<uint8_t>(Flag::Break) | static_cast<uint8_t>(Flag::AlwaysOne);
    this->m_sec.emitMov(P, UL);   // Push(P | Break | AlwaysOne)
    this->m_sec.emitOr(mask, UL);
    memory.push8(UL);
    break;
  }
  case Instruction::PLA:
    memory.pull8(A);

    // pull8() can't guarantee that the RFLAGS reflect what we pulled.  So
    // throw it through what's basically a no-op.
    this->m_sec.emitOr(A, A);
    this->setNz();
    break;
  case Instruction::PLP:
    memory.pull8(P);
    break;
  case Instruction::ROL:
    // AMD64 Surprise: RCL (and RCR) do NOT affect the Sign/Zero flags, but the
    // SHL/SHR instructions do!  So, we have to do these checks manually.
    memory.rmw(instr, [this](Register reg) {
      uint8_t c = static_cast<uint8_t>(Cpu::flagBit(Flag::Carry));
      uint8_t cMask = static_cast<uint8_t>(Flag::Carry);

      this->m_sec.emitBt(c, PX);                // Carry <- %P's carry
      this->m_sec.emitRcl(1, reg);              // %reg = (%reg << 1) | Carry
      this->m_sec.emitSetcc(Carry, VL);         // Rescue Carry flag
      this->m_sec.emitOr(reg, reg);             // Analyse %reg for NZ flags
      this->setNz(cMask);                       // %P <- NZ
      this->updateFlag(Flag::Carry, VL, true);  // %P <- %VL as Carry

      return reg;
    });
    break;
  case Instruction::ROR:
    memory.rmw(instr, [this](Register reg) {
      uint8_t c = static_cast<uint8_t>(Cpu::flagBit(Flag::Carry));
      uint8_t cMask = static_cast<uint8_t>(Flag::Carry);

      this->m_sec.emitBt(c, PX);                // Carry <- %P's carry
      this->m_sec.emitRcr(1, reg);              // %reg = Carry | (%reg >> 1)
      this->m_sec.emitSetcc(Carry, VL);         // Rescue Carry flag
      this->m_sec.emitOr(reg, reg);             // Analyse %reg for NZ flags
      this->setNz(cMask);                       // %P <- NZ
      this->updateFlag(Flag::Carry, VL, true);  // %P <- %VL as Carry

      return reg;
    });
    break;
  case Instruction::RTI:
    this->logInstruction(address, instr);
    memory.pull8(P);
    memory.pull16(PC);
    this->returnToHost(State::Reason::Jump, PC);
    return { false, nextAddr };
  case Instruction::RTS:
    this->logInstruction(address, instr);
    memory.pull16(PC);
    this->m_sec.emitInc(PC); // `JSR` stores the return PC off-by-one.
    this->returnToHost(State::Reason::Jump, PC);
    return { false, nextAddr };
  case Instruction::SBC: {
    // Invert using 1s complement, the Carry will then adjust.
    Register value = memory.read(instr);
    this->m_sec.emitXor(0xFF, value);
    this->adc(value);
    break;
  }
  case Instruction::SEC:
    this->updateFlag(Flag::Carry, true);
    break;
  case Instruction::SED:
    this->updateFlag(Flag::Decimal, true);
    break;
  case Instruction::SEI:
    this->updateFlag(Flag::Interrupt, true);
    break;
  case Instruction::STA:
    memory.write(instr, A);
    break;
  case Instruction::STX:
    memory.write(instr, X);
    break;
  case Instruction::STY:
    memory.write(instr, Y);
    break;
  case Instruction::TAX:
    // Note on the transfer instructions: AMD64s MOV does NOT modify any flags.
    // But the 6502 instructions do.  So we add a dummy OR which does modify
    // the flags, so we can update NZ in P like normal.

    this->m_sec.emitMov(A, X);
    this->m_sec.emitOr(X, X);
    this->setNz();
    break;
  case Instruction::TAY:
    this->m_sec.emitMov(A, Y);
    this->m_sec.emitOr(Y, Y);
    this->setNz();
    break;
  case Instruction::TSX:
    this->m_sec.emitMov(S, UL); // S in R13B (or so), X is in BH ..
    this->m_sec.emitMov(UL, X); // .. which can't be encoded at once.
    this->m_sec.emitOr(X, X);
    this->setNz();
    break;
  case Instruction::TXA:
    this->m_sec.emitMov(X, A);
    this->m_sec.emitOr(A, A);
    this->setNz();
    break;
  case Instruction::TXS:
    // This variant does NOT update the NZ flags!  (But `TSX` does)
    this->m_sec.emitMov(X, UL); // Same reason as for `TSX`
    this->m_sec.emitMov(UL, S);
    break;
  case Instruction::TYA:
    this->m_sec.emitMov(Y, A);
    this->m_sec.emitOr(A, A);
    this->setNz();
    break;
  case Instruction::Unknown:
    this->m_sec.emitMov(address, PC);
    this->returnToHost(State::Reason::UnknownInstruction, PC);
    return { false, nextAddr };
  default:
    throw std::runtime_error("Unknown instruction encountered");
  }

  // Usually we do require a JMP added at the end of a section to get to the
  // next instruction section.
  return { true, nextAddr };
}

void InstructionTranslator::adc(Register value) {
  uint8_t vc = static_cast<uint8_t>(Cpu::Flag::Carry) | static_cast<uint8_t>(Cpu::Flag::Overflow);
  uint8_t v = static_cast<uint8_t>(Cpu::flagBit(Cpu::Flag::Overflow));
  uint8_t c = static_cast<uint8_t>(Cpu::flagBit(Cpu::Flag::Carry));

  this->m_sec.emitBt(c, PX);           // Pull the 6502 Carry into AMD64s Carry
  this->m_sec.emitAdd(value, A, true); // %A = %A + %value + Carry
  this->m_sec.emitSetcc(Overflow, VL); // Rescue flags before setNz()
  this->m_sec.emitSetcc(Carry, WL);    // ...
  this->setNz(vc);                     // Set NZ
  this->m_sec.emitShl(v, VL);          // Adjust the V flag.  C is already adjusted.
  this->m_sec.emitOr(WL, P);           // %P |= Carry
  this->m_sec.emitOr(VL, P);           // %P |= Overflow
}

void InstructionTranslator::countCycles(int cycles) {
  // SUB $Count, %Cycles   ; Simply substract the cycle count
  this->m_sec.emitSub(cycles, CYCLES);
}

void InstructionTranslator::compare(Register reg, Register mem) {
  this->m_sec.emitCmp(mem, reg);

  // The 6502s Carry is set the other way around than what AMD64 does.
  // So just sell the Not-Carry as Carry.
  this->m_sec.emitSetcc(NotCarry, VL);

  // Also clears the Carry-bit for us:
  this->setNz(static_cast<uint8_t>(Cpu::Flag::Carry));
  this->m_sec.emitOr(VL, P);
  // No SHL necessary, as the Carry-bit is bit0 already.
}

void InstructionTranslator::setNz(uint8_t addMask) {
  // This method MUST be called right after the to-be observed instruction aas
  // executed!!

  uint8_t n = static_cast<uint8_t>(Cpu::flagBit(Cpu::Flag::Negative));
  uint8_t z = static_cast<uint8_t>(Cpu::flagBit(Cpu::Flag::Zero));
  uint8_t notNz = static_cast<uint8_t>(~((1 << n) | (1 << z) | addMask));

  this->m_sec.emitSetcc(Sign, UL); // SETS %UL      ; Copy Sign and ..
  this->m_sec.emitSetcc(Zero, UH); // SETZ %UH      ; Zero flags from %RFLAGS
  this->m_sec.emitShl(n, UL);      // SHL  $7, %UL  ; Adjust both to their 6502 position in %P
  this->m_sec.emitShl(z, UH);      // SHL  $1, %UH
  this->m_sec.emitOr(UH, UL);      // OR  %UH, %UL  ; %UL = %UL | %UH
  this->m_sec.emitAnd(notNz, P);   // AND ~NZ, %P   ; Clear NZ bits in %P
  this->m_sec.emitOr(UL, P);       // OR %UL, %P    ; And apply them to %P
}

void InstructionTranslator::updateFlag(Cpu::Flag flag, bool set) {
  uint8_t mask = static_cast<uint8_t>(flag);

  if (set) {
    this->m_sec.emitOr(mask, P);
  } else {
    this->m_sec.emitAnd(uint8_t(~mask), P);
  }
}

void InstructionTranslator::updateFlag(Cpu::Flag flag, Register reg, bool alreadyMasked) {
  uint8_t mask = static_cast<uint8_t>(flag);
  uint8_t bit = static_cast<uint8_t>(Cpu::flagBit(flag));

  this->m_sec.emitMov(reg, WL);
  if (bit > 0) this->m_sec.emitShl(bit, WL);
  if (!alreadyMasked) this->m_sec.emitAnd(uint8_t(~mask), P);
  this->m_sec.emitOr(WL, P);
}


void InstructionTranslator::returnToHost(Cpu::State::Reason reason, Register pc) {
  if (pc != PC) this->m_sec.emitMov(pc, PC);
  this->m_sec.emitMov(static_cast<uint8_t>(reason), REASON);
  this->m_sec.emitRet();
}
}
