#include <dynarec/functioncompiler.hpp>
#include <dynarec/instructiontranslator.hpp>
#include <dynarec/memorytranslator.hpp>
#include <dynarec/structtranslator.hpp>
#include <dynarec/configuration.hpp>

#include <variant>
#include <cpu.hpp>

#include <cpu/state.hpp>

namespace Dynarec {
struct Impl {
  using RmwProc = std::function<llvm::Value *(llvm::Value *)>;

  FunctionCompiler &compiler;
  MemoryTranslator memory;
  llvm::Function *function;

  FunctionFrame &frame() { return this->compiler.frame(); }

  Impl(FunctionCompiler &c, llvm::Function *f) : compiler(c), memory(c), function(f) {
  }

  void translate(Builder &b, uint16_t address, const Analysis::Branch::Instruction &instr) {
    if (const Core::Instruction *ptr = std::get_if<Core::Instruction>(&instr)) {
      if (CONFIGURATION.trace) this->traceInstruction(b, address, *ptr);
      this->translate(b, address, *ptr);
    } else {
      this->translate(b, address, std::get<Analysis::ConditionalInstruction>(instr));
    }
  }

  std::string buildTraceMessage(uint16_t address, Core::Instruction instr) {
    char buffer[64];
    int off = snprintf(buffer, sizeof(buffer), "%04x  %s %s", address, instr.commandName(), instr.addressingName());

    switch (instr.operandSize()) {
    case 2:
      snprintf(buffer + off, sizeof(buffer) - off, " %04x", instr.op16);
      break;
    case 1:
      snprintf(buffer + off, sizeof(buffer) - off, " %02x", instr.op8);
      break;
    }

    return buffer;
  }

  void traceInstruction(Builder &b, uint16_t address, Core::Instruction instr) {
    llvm::Value *message = b.CreateGlobalStringPtr(this->buildTraceMessage(address, instr));

    if (CONFIGURATION.verboseTrace) {
      this->frame().finalize(b, this->function->arg_begin());

      llvm::Value *tracer = this->compiler.compiler().builtin("trace.verbose");
      b.CreateCall(tracer, { message, this->function->arg_begin() });
    } else {
      llvm::Value *tracer = this->compiler.compiler().builtin("trace");
      b.CreateCall(tracer, { message });
    }
  }

  void reduceCycles(Builder &b, int amount) {
    this->rmw(b, this->frame().cycles, [&b, amount](llvm::Value *value) {
      return b.CreateSub(value, b.getInt32(amount));
    });
  }

  void translate(Builder &b, uint16_t address, const Core::Instruction &instr) {
    using Core::Instruction;

    // Address of the next instruction.
    uint16_t nextAddr = address + instr.operandSize() + 1;

    this->reduceCycles(b, instr.cycles);

    switch (instr.command) {
    case Instruction::ADC:
      this->rmw(b, this->frame().a, [this, &b, &instr](llvm::Value *acc) {
        return this->adc(b, acc, this->read(b, instr));
      });
      break;
    case Instruction::AND:
      this->rmw(b, this->frame().a, [this, &b, &instr](llvm::Value *acc) {
        return this->setNz(b, b.CreateAnd(acc, this->read(b, instr)));
      });
      break;
    case Instruction::ASL:
      this->rmw(b, instr, [this, &b](llvm::Value *value) {
        llvm::Value *result = b.CreateShl(value, 1);
        llvm::Value *hasHighbit = b.CreateICmpUGE(value, b.getInt8(0x80));

        this->rmw(b, this->frame().p, [this, &b, hasHighbit, result](llvm::Value *psw) {
          psw = this->updatePsw(b, psw, hasHighbit, Cpu::Flag::Carry);
          return this->setNz(b, psw, result);
        });

        return result;
      });
      break;
    case Instruction::BIT: {
      this->rmw(b, this->frame().p, [this, &b, &instr](llvm::Value *psw) {
        llvm::Value *value = this->read(b, instr);
        psw = b.CreateAnd(psw, ~(0x80 | 0x40 | 0x02));

        llvm::Value *anded = b.CreateAnd(value, b.CreateLoad(this->frame().a));
        llvm::Value *isZero = b.CreateICmpEQ(anded, b.getInt8(0x00));
        llvm::Value *truncated = b.CreateAnd(value, b.getInt8(0x40 | 0x80));
        llvm::Value *zeroFlag = b.CreateShl(b.CreateZExt(isZero, b.getInt8Ty()), Cpu::flagBit(Cpu::Flag::Zero));

        return b.CreateOr(psw, b.CreateOr(truncated, zeroFlag));
      });
      break;
    }
    case Instruction::BRK:
      this->returnToHost(b, b.getInt16(nextAddr), Cpu::State::Reason::Break);
      break;
    case Instruction::CLC:
      this->updatePsw(b, Cpu::Flag::Carry, false);
      break;
    case Instruction::CLD:
      this->updatePsw(b, Cpu::Flag::Decimal, false);
      break;
    case Instruction::CLI:
      this->updatePsw(b, Cpu::Flag::Interrupt, false);
      break;
    case Instruction::CLV:
      this->updatePsw(b, Cpu::Flag::Overflow, false);
      break;
    case Instruction::CMP:
      this->compare(b, b.CreateLoad(this->frame().a), this->read(b, instr));
      break;
    case Instruction::CPX:
      if (instr.addressing == Instruction::Imm) {
        asm("XCHG %bx, %bx");
      }

      this->compare(b, b.CreateLoad(this->frame().x), this->read(b, instr));
      break;
    case Instruction::CPY:
      this->compare(b, b.CreateLoad(this->frame().y), this->read(b, instr));
      break;
    case Instruction::DEC:
    case Instruction::DEX:
    case Instruction::DEY:
      this->rmw(b, instr, [this, &b](llvm::Value *value) {
        return this->setNz(b, b.CreateSub(value, b.getInt8(1), "ValueMinusOne"));
      });
      break;
    case Instruction::EOR:
      this->rmw(b, this->frame().a, [this, &b, &instr](llvm::Value *acc) {
        return this->setNz(b, b.CreateXor(acc, this->read(b, instr)));
      });
      break;
    case Instruction::INC:
    case Instruction::INX:
    case Instruction::INY:
      this->rmw(b, instr, [this, &b](llvm::Value *value) {
        return this->setNz(b, b.CreateAdd(value, b.getInt8(1), "ValuePlusOne"));
      });
      break;
    case Instruction::JMP: {
      llvm::Value *destination = b.CreateBitCast(this->resolve(b, instr), b.getInt16Ty());
      llvm::Value *isInfinite = b.CreateICmpEQ(destination, b.getInt16(address));

      // The reason "Jump" is one value lower than "InfiniteLoop".  We exploit
      // this by adding the "isInfinite" boolean result to the value of "Jump".
      // If the jump is infinite, it'll be incremented to mean "InfiniteLoop",
      // else it'll be left as "Jump".
      llvm::Value *offset = b.CreateZExt(isInfinite, b.getInt8Ty());
      llvm::Value *reason = b.CreateAdd(b.getInt8(static_cast<uint8_t>(Cpu::State::Reason::Jump)), offset);

      this->returnToHost(b, destination, reason);
      break;
    }
    case Instruction::JSR:
      this->push16(b, b.getInt16(nextAddr - 1));
      this->returnToHost(b, b.getInt16(instr.op16), Cpu::State::Reason::Jump);
      break;
    case Instruction::LDA:
      b.CreateStore(this->setNz(b, this->read(b, instr)), this->frame().a);
      break;
    case Instruction::LDX:
      b.CreateStore(this->setNz(b, this->read(b, instr)), this->frame().x);
      break;
    case Instruction::LDY:
      b.CreateStore(this->setNz(b, this->read(b, instr)), this->frame().y);
      break;
    case Instruction::LSR:
      this->rmw(b, instr, [this, &b](llvm::Value *value) {
        llvm::Value *result = b.CreateLShr(value, 1);
        llvm::Value *hasLowbit = b.CreateAnd(value, b.getInt8(0x01));

        this->rmw(b, this->frame().p, [this, &b, hasLowbit, result](llvm::Value *psw) {
          psw = this->updatePsw(b, psw, hasLowbit, Cpu::Flag::Carry);
          return this->setNz(b, psw, result);
        });

        return result;
      });

      break;
    case Instruction::NOP: /* Nothing. */ break;
    case Instruction::ORA:
      this->rmw(b, this->frame().a, [this, &b, &instr](llvm::Value *acc) {
        return this->setNz(b, b.CreateOr(acc, this->read(b, instr)));
      });
      break;
    case Instruction::PHA:
      this->push8(b, b.CreateLoad(this->frame().a));
      break;
    case Instruction::PHP: {
      llvm::Value *psw = b.CreateLoad(this->frame().p);
      psw = b.CreateOr(psw, static_cast<uint8_t>(Cpu::Flag::Break) | static_cast<uint8_t>(Cpu::Flag::AlwaysOne));
      this->push8(b, psw);
      break;
    }
    case Instruction::PLA:
      b.CreateStore(this->setNz(b, this->pull8(b)), this->frame().a);
      break;
    case Instruction::PLP:
      b.CreateStore(this->pull8(b), this->frame().p);
      break;
    case Instruction::ROL:
      this->rmw(b, instr, [this, &b](llvm::Value *value) {
        this->rmw(b, this->frame().p, [this, &b, &value](llvm::Value *psw) {
          llvm::Value *bit = b.CreateAnd(psw, 1, "CarryBit"); // Carry is Bit0
          llvm::Value *hasHighbit = b.CreateICmpUGE(value, b.getInt8(0x80), "ValuesHighBit");

          value = b.CreateOr(b.CreateShl(value, 1), bit, "RolResult");
          psw = this->updatePsw(b, psw, hasHighbit, Cpu::Flag::Carry);
          return this->setNz(b, psw, value);
        });

        return value;
      });
      break;
    case Instruction::ROR:
      this->rmw(b, instr, [this, &b](llvm::Value *value) {
        this->rmw(b, this->frame().p, [this, &b, &value](llvm::Value *psw) {
          llvm::Value *bit = b.CreateAnd(psw, 1, "CarryBit"); // Carry is Bit0
          llvm::Value *hasLowbit = b.CreateAnd(value, 0x01, "ValuesLowBit");

          value = b.CreateOr(b.CreateLShr(value, 1), b.CreateShl(bit, 7), "RorResult");
          psw = this->updatePsw(b, psw, hasLowbit, Cpu::Flag::Carry);
          return this->setNz(b, psw, value);
        });

        return value;
      });
      break;
    case Instruction::RTI:
      b.CreateStore(this->pull8(b), this->frame().p);
      this->returnToHost(b, this->pull16(b), Cpu::State::Reason::Return);
      break;
    case Instruction::RTS: {
      llvm::Value *retAddr = b.CreateAdd(this->pull16(b), b.getInt16(1));
      this->returnToHost(b, retAddr, Cpu::State::Reason::Return);
      break;
    }
    case Instruction::SBC:
      // Invert using 1s complement, the Carry will then adjust.
      this->rmw(b, this->frame().a, [this, &b, &instr](llvm::Value *acc) {
        llvm::Value *operandNeg = b.CreateXor(this->read(b, instr), 0xFF, "OperandNegated");
        return this->adc(b, acc, operandNeg);
      });
      break;
    case Instruction::SEC:
      this->updatePsw(b, Cpu::Flag::Carry, true);
      break;
    case Instruction::SED:
      this->updatePsw(b, Cpu::Flag::Decimal, true);
      break;
    case Instruction::SEI:
      this->updatePsw(b, Cpu::Flag::Interrupt, true);
      break;
    case Instruction::STA:
      this->write(b, instr, b.CreateLoad(this->frame().a));
      break;
    case Instruction::STX:
      this->write(b, instr, b.CreateLoad(this->frame().x));
      break;
    case Instruction::STY:
      this->write(b, instr, b.CreateLoad(this->frame().y));
      break;
    case Instruction::TAX:
      b.CreateStore(this->setNz(b, b.CreateLoad(this->frame().a)), this->frame().x);
      break;
    case Instruction::TAY:
      b.CreateStore(this->setNz(b, b.CreateLoad(this->frame().a)), this->frame().y);
      break;
    case Instruction::TSX:
      b.CreateStore(this->setNz(b, b.CreateLoad(this->frame().s)), this->frame().x);
      break;
    case Instruction::TXA:
      b.CreateStore(this->setNz(b, b.CreateLoad(this->frame().x)), this->frame().a);
      break;
    case Instruction::TXS:
      b.CreateStore(b.CreateLoad(this->frame().x), this->frame().s);
      break;
    case Instruction::TYA:
      b.CreateStore(this->setNz(b, b.CreateLoad(this->frame().y)), this->frame().a);
      break;
    default:
      this->returnToHost(b, b.getInt16(address), Cpu::State::Reason::UnknownInstruction);
      break;
    }
  }

  void translate(Builder &b, uint16_t address, const Analysis::ConditionalInstruction &instr) {
    using Core::Instruction;

    this->reduceCycles(b, instr.cycles);

    // While we're here, check the remaining cycles.  If it's less than 1,
    // then store the target address and exit to the host.  If it's more
    // than zero, branch in guest code so we return less often to the host.
    this->remainingCycleCheck(b, address);

    // Trace after the remaining cycle check.  Else, we log branching conditions
    // double which happen to have an exhausted cycle count.
    if (CONFIGURATION.trace) this->traceInstruction(b, address, instr);

    switch (instr.command) {
    case Instruction::BCC:
      this->conditionalBranch(b, Cpu::Flag::Carry, false, instr);
      break;
    case Instruction::BCS:
      this->conditionalBranch(b, Cpu::Flag::Carry, true, instr);
      break;
    case Instruction::BEQ:
      this->conditionalBranch(b, Cpu::Flag::Zero, true, instr);
      break;
    case Instruction::BMI:
      this->conditionalBranch(b, Cpu::Flag::Negative, true, instr);
      break;
    case Instruction::BNE:
      this->conditionalBranch(b, Cpu::Flag::Zero, false, instr);
      break;
    case Instruction::BPL:
      this->conditionalBranch(b, Cpu::Flag::Negative, false, instr);
      break;
    case Instruction::BVC:
      this->conditionalBranch(b, Cpu::Flag::Overflow, false, instr);
      break;
    case Instruction::BVS:
      this->conditionalBranch(b, Cpu::Flag::Overflow, true, instr);
      break;
    default:
      throw std::runtime_error("Missing case for conditional instruction!");
    }
  }

  /**** Reusable complex instruction implementations. ****/

  void conditionalBranch(Builder &b, Cpu::Flag flag, bool expect,
                         const Analysis::ConditionalInstruction &instr) {
    llvm::BasicBlock *truthy = this->compiler.compileBranch(instr.trueBranch());
    llvm::BasicBlock *falsy = this->compiler.compileBranch(instr.falseBranch());

    this->conditionalBranch(b, flag, expect, truthy, falsy);
  }

  void conditionalBranch(Builder &b, Cpu::Flag flag, bool expectSet,
                         llvm::BasicBlock *truthy, llvm::BasicBlock *falsy) {
    int mask = 1 << Cpu::flagBit(flag);

    llvm::Value *psw = b.CreateLoad(this->frame().p, "PSW");
    llvm::Value *cleaned = b.CreateAnd(psw, mask);

    llvm::Value *expect = b.getInt8(expectSet ? mask : 0);
    llvm::Value *test = b.CreateICmpEQ(cleaned, expect);

    b.CreateCondBr(test, truthy, falsy);
  }

  void remainingCycleCheck(Builder &b, uint16_t address) {
    llvm::Value *cycles = b.CreateLoad(this->frame().cycles);
    llvm::Value *test = b.CreateICmpSLE(cycles, b.getInt32(0)); // `cycles <= 0`

    llvm::Function *func = b.GetInsertBlock()->getParent();
    llvm::BasicBlock *exhausted = llvm::BasicBlock::Create(b.getContext(), "CyclesExhausted", func);
    llvm::BasicBlock *cont = llvm::BasicBlock::Create(b.getContext(), "CyclesNotExhausted", func);

    // if (cycles <= 0) { exhausted } else { cont }
    b.CreateCondBr(test, exhausted, cont);

    b.SetInsertPoint(exhausted); // Return to host
    this->returnToHost(b, b.getInt16(address), Cpu::State::Reason::CyclesExhausted);

    // Resume operation as if nothing happened for the caller.
    b.SetInsertPoint(cont);
  }

  llvm::Value *adc(Builder &b, llvm::Value *left, llvm::Value *right) {
    llvm::Value *result = nullptr;

    this->rmw(b, this->frame().p, [this, &b, left, right, &result](llvm::Value *psw) {
      llvm::Value *left16 = b.CreateZExt(left, b.getInt16Ty(), "Left16Bit");
      llvm::Value *right16 = b.CreateZExt(right, b.getInt16Ty(), "Right16Bit");

      // Carry is the lowest bit in P.
      llvm::Value *carry8 = b.CreateAnd(psw, 1, "Carry8Bit");
      llvm::Value *carry16 = b.CreateZExt(carry8, b.getInt16Ty(), "Carry16Bit");

      llvm::Value *immediate = b.CreateAdd(left16, carry16, "Left+Carry");
      result = b.CreateAdd(immediate, right16, "L+R+C"); // Also add carry

      // Update PSW flags and write them back.
      return this->setNzvc(b, psw, left16, right16, result);
    });

    // Result is 16-Bit, truncate to 8-Bit
    return b.CreateTrunc(result, b.getInt8Ty());
  }

  void compare(Builder &b, llvm::Value *reg, llvm::Value *operand) {
    this->rmw(b, this->frame().p, [this, &b, reg, operand](llvm::Value *psw) {
      llvm::Value *greaterEqual = b.CreateICmpUGE(reg, operand, "Register>=Operand");
      llvm::Value *difference = b.CreateSub(reg, operand, "RegisterMinusOperand");

      psw = this->updatePsw(b, psw, greaterEqual, Cpu::Flag::Carry);
      psw = this->setNz(b, psw, difference);
      return psw;
    });
  }

  void returnToHost(Builder &b, llvm::Value *pc, Cpu::State::Reason reason) {
    this->returnToHost(b, pc, b.getInt8(static_cast<uint8_t>(reason)));
  }

  void returnToHost(Builder &b, llvm::Value *pc, llvm::Value *reason) {
    // Rescue registers back into the state structure.
    this->frame().finalize(b, this->function->arg_begin());

    // Update pc and reason values in the state struct too
    StructTranslator translator;
    std::vector<llvm::Value *> fields{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, pc, reason };
    translator.copyFrom(b, this->function->arg_begin(), fields);

    // Return back to host code.
    b.CreateRetVoid();
  }

  void writeToLog(Builder &b, const char *message, llvm::Value *value) {
    llvm::Value *logger = this->compiler.compiler().builtin("debug");
    llvm::Value *msg = b.CreateGlobalStringPtr(message);
    b.CreateCall(logger, { msg, value });
  }

  /**** Stack operations. ****/

  void push8(Builder &b, llvm::Value *value) {
    this->rmw(b, this->frame().s, [this, &b, value](llvm::Value *s) {
      b.CreateStore(value, this->memory.stackPointer(b, s));
      return b.CreateSub(s, b.getInt8(1), "SMinusOne");
    });
  }

  void push16(Builder &b, llvm::Value *value) {
    this->rmw(b, this->frame().s, [this, &b, value](llvm::Value *s) {
      s = b.CreateSub(s, b.getInt8(2), "SMinusTwo");
      llvm::Value *ptr = this->memory.stackPointer(b, s);
      llvm::Value *lo = b.CreateTrunc(value, b.getInt8Ty());
      llvm::Value *hi = b.CreateTrunc(b.CreateLShr(value, 8), b.getInt8Ty());

      b.CreateStore(hi, b.CreateGEP(ptr, b.getInt8(2)));
      b.CreateStore(lo, b.CreateGEP(ptr, b.getInt8(1)));

      return s;
    });
  }

  llvm::Value *pull8(Builder &b) {
    llvm::Value *result = nullptr;

    this->rmw(b, this->frame().s, [this, &b, &result](llvm::Value *s) {
      s = b.CreateAdd(s, b.getInt8(1), "SPlusOne");
      result = b.CreateLoad(this->memory.stackPointer(b, s), "Stack8Pull");
      return s;
    });

    return result;
  }

  llvm::Value *pull16(Builder &b) {
    llvm::Value *result = nullptr;

    this->rmw(b, this->frame().s, [this, &b, &result](llvm::Value *s) {
      llvm::Value *ptr = this->memory.stackPointer(b, s);
      llvm::Value *hi = b.CreateLoad(b.CreateGEP(ptr, b.getInt8(2)), "Stack16PullHi");
      llvm::Value *lo = b.CreateLoad(b.CreateGEP(ptr, b.getInt8(1)), "Stack16PullLo");

      lo = b.CreateZExt(lo, b.getInt16Ty());
      hi = b.CreateShl(b.CreateZExt(hi, b.getInt16Ty()), 8);

      result = b.CreateOr(lo, hi, "Stack16Pull");
      return b.CreateAdd(s, b.getInt8(2), "SPlusTwo");
    });

    return result;
  }

  /**** Processor status word manipulation. ****/

  void updatePsw(Builder &b, Cpu::Flag flag, bool state) {
    this->rmw(b, this->frame().p, [&b, flag, state](llvm::Value *psw) {
      if (state)
        return b.CreateOr(psw, (1 << Cpu::flagBit(flag)));
      else
        return b.CreateAnd(psw, ~(1 << Cpu::flagBit(flag)));
    });
  }

  llvm::Value *updatePsw(Builder &b, llvm::Value *psw, llvm::Value *condition, Cpu::Flag flag) {
    int bit = Cpu::flagBit(flag);
    llvm::Value *cond8 = b.CreateZExt(condition, b.getInt8Ty(), "Cond8Bit");
    llvm::Value *shifted = b.CreateShl(cond8, bit, "ShiftedCond");
    llvm::Value *cleaned = b.CreateAnd(psw, ~(1 << bit), "PswCleaned");
    return b.CreateOr(cleaned, shifted, "PswUpdated");
  }

  llvm::Value *setNz(Builder &b, llvm::Value *value) {
    this->rmw(b, this->frame().p, [this, &b, value](llvm::Value *psw) {
      return this->setNz(b, psw, value);
    });

    return value;
  }

  llvm::Value *setNz(Builder &b, llvm::Value *psw, llvm::Value *value) {
    llvm::Value *isNegative = b.CreateICmpUGE(value, b.getInt8(0x80));
    llvm::Value *isZero = b.CreateICmpEQ(value, b.getInt8(0x00));

    psw = this->updatePsw(b, psw, isNegative, Cpu::Flag::Negative);
    psw = this->updatePsw(b, psw, isZero, Cpu::Flag::Zero);

    return psw;
  }

  llvm::Value *setNzvc(Builder &b, llvm::Value *psw, llvm::Value *left, llvm::Value *right, llvm::Value *result) {
    // Check for unsigned overflow: result > 0xFF ("result" is still 16-Bit)
    llvm::Value *isCarry = b.CreateICmpUGT(result, b.getInt16(0x00FF));
    psw = this->updatePsw(b, psw, isCarry, Cpu::Flag::Carry);

    // Check for signed overflow: Int8(~(left ^ right) & (left ^ result)) >= 0x80;
    llvm::Value *lxri = b.CreateNot(b.CreateXor(left, right), "NotLeftXorRight");
    llvm::Value *lxre = b.CreateXor(left, result, "LeftXorResult");
    llvm::Value *anded = b.CreateTrunc(b.CreateAnd(lxri, lxre), b.getInt8Ty());
    llvm::Value *isOverflow = b.CreateICmpUGE(anded, b.getInt8(0x80));
    psw = this->updatePsw(b, psw, isOverflow, Cpu::Flag::Overflow);

    return this->setNz(b, psw, b.CreateTrunc(result, b.getInt8Ty()));
  }

  /**** Memory and Register read/write functionality. ****/

  static uint16_t operand(const Core::Instruction &instr) {
    if (instr.operandSize() == 1)
      return instr.op16 & 0x00FF;
    return instr.op16;
  }

  llvm::Value *resolve(Builder &b, const Core::Instruction &instr)
  { return this->resolve(b, instr.addressing, operand(instr)); }

  llvm::Value *read(Builder &b, const Core::Instruction &instr)
  { return this->read(b, instr.addressing, operand(instr)); }

  void write(Builder &b, const Core::Instruction &instr, llvm::Value *value)
  { this->write(b, instr.addressing, operand(instr), value); }

  void rmw(Builder &b, const Core::Instruction &instr, RmwProc proc)
  { this->rmw(b, instr.addressing, operand(instr), proc); }

  llvm::Value *resolve(Builder &b, Core::Instruction::Addressing mode, uint16_t addr) {
    using Core::Instruction;

    uint8_t addr8 = static_cast<uint8_t>(addr);
    FunctionFrame &frame = this->compiler.frame();

    switch (mode) {
    case Instruction::Acc: return frame.a;
    case Instruction::X: return frame.x;
    case Instruction::Y: return frame.y;
    case Instruction::S: return frame.s;
    case Instruction::P: return frame.p;
    case Instruction::Imm:
    case Instruction::Imp: return b.getInt8(addr8);
    default: return this->memory.resolve(b, mode, b.getInt16(addr));
    }
  }

  llvm::Value *read(Builder &b, Core::Instruction::Addressing mode, uint16_t addr) {
    if (Core::Instruction::isMemory(mode)) {
      return this->memory.read(b, mode, b.getInt16(addr));
    } else if (mode == Core::Instruction::Imm) {
      return b.getInt8(static_cast<uint8_t>(addr));
    } else {
      return b.CreateLoad(this->resolve(b, mode, addr));
    }
  }

  void write(Builder &b, Core::Instruction::Addressing mode, uint16_t addr, llvm::Value *value) {
    if (Core::Instruction::isMemory(mode)) {
      this->memory.write(b, mode, b.getInt16(addr), value);
    } else {
      b.CreateStore(value, this->resolve(b, mode, addr));
    }
  }

  void rmw(Builder &b, llvm::Value *pointer, RmwProc proc) {
    llvm::Value *value = b.CreateLoad(pointer);
    llvm::Value *result = proc(value);
    b.CreateStore(result, pointer);
  }

  void rmw(Builder &b, Core::Instruction::Addressing mode, uint16_t addr, RmwProc proc) {
    if (Core::Instruction::isMemory(mode)) {
      this->memory.rmw(b, mode, b.getInt16(addr), proc);
    } else {
      llvm::Value *resolved = this->resolve(b, mode, addr);
      llvm::Value *value = b.CreateLoad(resolved);
      llvm::Value *result = proc(value);
      b.CreateStore(result, resolved);
    }
  }
};

InstructionTranslator::InstructionTranslator(FunctionCompiler &compiler, llvm::Function *func)
  : m_compiler(compiler), m_func(func)
{
}

llvm::BasicBlock *InstructionTranslator::translate(Builder &b, uint16_t address,
                                                   const Analysis::Branch::Instruction &instr) {
  Impl(this->m_compiler, this->m_func).translate(b, address, instr);
  return b.GetInsertBlock();
}

}
