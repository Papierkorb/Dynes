#include <interpret/core_interpret.hpp>

#include <core/disassembler.hpp>
#include <functional>

class InterpretCoreImpl {
public:
  Interpret::Core *core;
  Cpu::State &state;
  Cpu::Memory::Ptr mem;
  Core::Disassembler *disasm;

  InterpretCoreImpl(Interpret::Core *parent, Cpu::State &state, const Cpu::Memory::Ptr &mem)
    : core(parent), state(state), mem(mem) {
    this->disasm = new Core::Disassembler(mem);
  }

  ~InterpretCoreImpl() {
    delete this->disasm;
  }

  int run(uint16_t address, int cycles) {
    this->disasm->setPosition(address);

    while (cycles > 0) {
      cycles -= this->step();
    }

    this->state.pc = static_cast<uint16_t>(this->disasm->position());
    return cycles;
  }

  /** Executes the next instruction. */
  int step() {
    Cpu::Hook *hook = this->core->hook();
    Core::Instruction instr = this->disasm->next();

    if (hook) hook->beforeInstruction(instr, this->state);
    this->state.pc = static_cast<uint16_t>(this->disasm->position());
    this->execute(instr);

    if (hook) hook->afterInstruction(instr, this->state);
    return instr.cycles;
  }

  /** Updates the Negative and Zero flags, and returns \a value. */
  uint8_t setNz(uint8_t value) {
    this->state.setFlag(Cpu::Flag::Negative, value >= 0x80);
    this->state.setFlag(Cpu::Flag::Zero, value == 0);
    return value;
  }

  /** Updates the Carry flag and passes on to \c setNz. */
  uint8_t setNzc(uint16_t value) {
    this->state.setFlag(Cpu::Flag::Carry, value > 0xFF);
    return this->setNz(static_cast<uint8_t>(value));
  }

  /**
   * Updates the Overflow flag from the addition or substraction of \a left
   * and \a right, with the result being \a value.  Passes on to \c setNzc.
   */
  uint8_t setNvzc(uint8_t left, uint8_t right, uint16_t value) {
    // This is how the 6502 calculates the overflow bit:
    uint8_t isOverflow = ~(left ^ right) & (left ^ value) & 0x80;
    this->state.setFlag(Cpu::Flag::Overflow, isOverflow != 0);
    return this->setNzc(value);
  }

  /** Compares the value of \a to \a op. */
  void compare(uint8_t reg, uint8_t op) {
    this->state.setFlag(Cpu::Flag::Carry, reg >= op);
    this->setNz(reg - op);
  }

  /**
   * Moves the relative position of the program counter by \a displacement if
   * \a condition is \c true.
   */
  void branchIf(uint8_t displacement, bool condition) {
    // uint8_t -> int32_t WITH sign extension.
    int32_t disp = static_cast<int32_t>(static_cast<int8_t>(displacement));
    if (condition) this->disasm->setPosition(this->disasm->position() + disp);
  }

  /** Resolves the \a instr to an absolute memory address. */
  uint16_t resolve(const Core::Instruction &instr) {
    return this->resolve(instr.addressing, instr.op16);
  }

  /**
   * Resolves the (maybe relative) \a addr to an absolute address using \a mode.
   */
  uint16_t resolve(Core::Instruction::Addressing mode, uint16_t addr) {
    using Core::Instruction;
    uint8_t addr8 = static_cast<uint8_t>(addr);

    switch(mode) {
    default: // Ignore modes that don't translate to memory.
      return 0;
    case Instruction::Rel:
      return this->state.pc + static_cast<int8_t>(addr8);
    case Instruction::Zp:
      return addr & 0x00FF;
    case Instruction::ZpX:
      return (addr8 + this->state.x) & 0x00FF;
    case Instruction::ZpY:
      return (addr8 + this->state.y) & 0x00FF;
    case Instruction::Abs:
      return addr;
    case Instruction::AbsX:
      return addr + this->state.x;
    case Instruction::AbsY:
      return addr + this->state.y;
    case Instruction::Ind:
      return this->mem->read16(addr);
    case Instruction::IndX:
      return this->mem->read16((addr8 + this->state.x) & 0x00FF);
    case Instruction::IndY:
      return this->mem->read16(addr8) + this->state.y;
    }
  }

  /** Reads the byte \a instr is pointing at, be it a memory address or a register. */
  uint8_t read(const Core::Instruction &instr) {
    return this->read(instr.addressing, instr.op16);
  }

  /** Reads the byte addressed by \a mode at \a addr. */
  uint8_t read(Core::Instruction::Addressing mode, uint16_t addr) {
    using Core::Instruction;

    switch(mode) {
    case Instruction::Acc: return this->state.a;
    case Instruction::X: return this->state.x;
    case Instruction::Y: return this->state.y;
    case Instruction::S: return this->state.s;
    case Instruction::P: return this->state.p;
    case Instruction::Imm:
    case Instruction::Imp:
    case Instruction::Rel:
      return static_cast<uint8_t>(addr);
    default: // Resolve and read from memory.
      return this->mem->read(this->resolve(mode, addr));
    }
  }

  /** Writes the \a value into what \a instr is pointing at. */
  void write(const Core::Instruction &instr, uint8_t value) {
    this->write(instr.addressing, instr.op16, value);
  }

  /** Writes \a value into the byte addressed by \a mode at \a addr. */
  void write(Core::Instruction::Addressing mode, uint16_t addr, uint8_t value) {
    using Core::Instruction;

    switch(mode) {
    case Instruction::Acc:
      this->state.a = value;
      break;
    case Instruction::X:
      this->state.x = value;
      break;
    case Instruction::Y:
      this->state.y = value;
      break;
    case Instruction::S:
      this->state.s = value;
      break;
    case Instruction::P:
      this->state.p = value;
      break;
    case Instruction::Imm:
    case Instruction::Imp:
    case Instruction::Rel:
      throw std::runtime_error("Can't write to Imm/Imp/Rel addressing instruction");
      break; // Ignore.
    default: // Resolve and write to memory.
      this->mem->write(this->resolve(mode, addr), value);
      break;
    }
  }

  /**
   * Reads the byte \a instr is pointing at.  This byte is then passed to \a proc.
   * The result of \a proc is then written back into the same place.
   */
  void rmw(const Core::Instruction &instr, std::function<uint8_t(uint8_t)> proc) {
    rmw(instr.addressing, instr.op16, proc);
  }

  /** Like the \c Core::Instruction version. */
  void rmw(Core::Instruction::Addressing mode, uint16_t addr, std::function<uint8_t(uint8_t)> proc) {
    using Core::Instruction;

    switch(mode) {
    case Instruction::Acc:
      this->state.a = proc(this->state.a);
      break;
    case Instruction::X:
      this->state.x = proc(this->state.x);
      break;
    case Instruction::Y:
      this->state.y = proc(this->state.y);
      break;
    case Instruction::S:
      this->state.s = proc(this->state.s);
      break;
    case Instruction::P:
      this->state.p = proc(this->state.p);
      break;
    case Instruction::Imm:
      this->state.a = proc(static_cast<uint8_t>(addr));
      break;
    case Instruction::Rel:
    case Instruction::Imp:
      throw std::runtime_error("Can't RMW on a Rel/Imp adressing instruction");
      break;
    default: { // Read and write to memory.
      uint16_t resolved = this->resolve(mode, addr);
      uint8_t result = proc(this->mem->read(resolved));
      this->mem->write(resolved, result);
      break;
    }
    }
  }

  void jump(uint16_t addr) {
    this->state.pc = addr;
    this->disasm->setPosition(static_cast<uint32_t>(addr));
  }

  /** Shared implementation for ADC and SBC instructions. */
  void adc(uint8_t right8) {
    uint16_t left = static_cast<uint16_t>(this->state.a);
    uint16_t right = static_cast<uint16_t>(right8);

    // Adjust by carry
    uint16_t c = this->state.hasFlag(Cpu::Flag::Carry) ? 1 : 0;
    this->state.a = this->setNvzc(left, right, left + right + c);
  }

  /** Executes the \a instr in the context of the guest CPU. */
  void execute(Core::Instruction instr) {
    using Core::Instruction;
    using Cpu::Flag;

    switch (instr.command) {
    case Instruction::ADC:
      this->adc(this->read(instr));
      break;
    case Instruction::AND:
      this->state.a = this->setNz(this->state.a & this->read(instr));
      break;
    case Instruction::ASL:
      this->rmw(instr, [this](uint8_t v) {
        this->state.setFlag(Flag::Carry, v >= 0x80);
        return this->setNz(v << 1);
      });
      break;
    case Instruction::BCC:
      this->branchIf(instr.op8, !this->state.hasFlag(Flag::Carry));
      break;
    case Instruction::BCS:
      this->branchIf(instr.op8, this->state.hasFlag(Flag::Carry));
      break;
    case Instruction::BEQ:
      this->branchIf(instr.op8, this->state.hasFlag(Flag::Zero));
      break;
    case Instruction::BIT: {
      uint8_t value = this->read(instr);
      this->state.setFlag(Flag::Zero, (this->state.a & value) == 0);
      this->state.setFlag(Flag::Overflow, (value & (1 << 6)) != 0);
      this->state.setFlag(Flag::Negative, (value & (1 << 7)) != 0);
      break;
    }
    case Instruction::BMI:
      this->branchIf(instr.op8, this->state.hasFlag(Flag::Negative));
      break;
    case Instruction::BNE:
      this->branchIf(instr.op8, !this->state.hasFlag(Flag::Zero));
      break;
    case Instruction::BPL:
      this->branchIf(instr.op8, !this->state.hasFlag(Flag::Negative));
      break;
    case Instruction::BRK:
      this->core->interrupt(Cpu::Break, true);
      break;
    case Instruction::BVC:
      this->branchIf(instr.op8, !this->state.hasFlag(Flag::Overflow));
      break;
    case Instruction::BVS:
      this->branchIf(instr.op8, this->state.hasFlag(Flag::Overflow));
      break;
    case Instruction::CLC:
      this->state.setFlag(Flag::Carry, false);
      break;
    case Instruction::CLD:
      this->state.setFlag(Flag::Decimal, false);
      break;
    case Instruction::CLI:
      this->state.setFlag(Flag::Interrupt, false);
      break;
    case Instruction::CLV:
      this->state.setFlag(Flag::Overflow, false);
      break;
    case Instruction::CMP:
      this->compare(this->state.a, this->read(instr));
      break;
    case Instruction::CPX:
      this->compare(this->state.x, this->read(instr));
      break;
    case Instruction::CPY:
      this->compare(this->state.y, this->read(instr));
      break;
    case Instruction::DEC:
    case Instruction::DEX:
    case Instruction::DEY:
      this->rmw(instr, [this](uint8_t v) { return this->setNz(v - 1); });
      break;
    case Instruction::EOR:
      this->state.a = this->setNz(this->state.a ^ this->read(instr));
      break;
    case Instruction::INC:
    case Instruction::INX:
    case Instruction::INY:
      this->rmw(instr, [this](uint8_t v) { return this->setNz(v + 1); });
      break;
    case Instruction::JMP:
      this->jump(this->resolve(instr));
      break;
    case Instruction::JSR:
      this->core->push(static_cast<uint16_t>(this->state.pc - 1));
      this->jump(instr.op16);
      break;
    case Instruction::LDA:
      this->state.a = this->setNz(this->read(instr));
      break;
    case Instruction::LDX:
      this->state.x = this->setNz(this->read(instr));
      break;
    case Instruction::LDY:
      this->state.y = this->setNz(this->read(instr));
      break;
    case Instruction::LSR:
      this->rmw(instr, [this](uint8_t v) {
        this->state.setFlag(Flag::Carry, (v & 1) == 1);
        return this->setNz(v >> 1);
      });
      break;
    case Instruction::NOP: /* Nothing. */ break;
    case Instruction::ORA:
      this->state.a = this->setNz(this->state.a | this->read(instr));
      break;
    case Instruction::PHA:
      this->core->push(this->state.a);
      break;
    case Instruction::PHP: {
      uint8_t psw = this->state.p | static_cast<uint8_t>(Flag::Break) | static_cast<uint8_t>(Flag::AlwaysOne);
      this->core->push(psw);
      break;
    }
    case Instruction::PLA:
      this->state.a = this->setNz(this->core->pull());
      break;
    case Instruction::PLP:
      this->state.p = this->core->pull();
      break;
    case Instruction::ROL:
      this->rmw(instr, [this](uint8_t v) {
        uint8_t c = this->state.hasFlag(Flag::Carry) ? 1 : 0;
        this->state.setFlag(Flag::Carry, v >= 0x80);
        return this->setNz((v << 1) | c);
      });
      break;
    case Instruction::ROR:
      this->rmw(instr, [this](uint8_t v) {
        uint8_t c = this->state.hasFlag(Flag::Carry) ? 0x80 : 0;
        this->state.setFlag(Flag::Carry, (v & 1) == 1);
        return this->setNz((v >> 1) | c);
      });
      break;
    case Instruction::RTI:
      this->state.p = this->core->pull();
      this->jump(this->core->pull16());
      break;
    case Instruction::RTS:
      this->jump(this->core->pull16() + 1);
      break;
    case Instruction::SBC:
      // Invert using 1s complement, the Carry will then adjust.
      this->adc(this->read(instr) ^ 0xFF);
      break;
    case Instruction::SEC:
      this->state.setFlag(Flag::Carry, true);
      break;
    case Instruction::SED:
      this->state.setFlag(Flag::Decimal, true);
      break;
    case Instruction::SEI:
      this->state.setFlag(Flag::Interrupt, true);
      break;
    case Instruction::STA:
      this->write(instr, this->state.a);
      break;
    case Instruction::STX:
      this->write(instr, this->state.x);
      break;
    case Instruction::STY:
      this->write(instr, this->state.y);
      break;
    case Instruction::TAX:
      this->state.x = this->setNz(this->state.a);
      break;
    case Instruction::TAY:
      this->state.y = this->setNz(this->state.a);
      break;
    case Instruction::TSX:
      this->state.x = this->setNz(this->state.s);
      break;
    case Instruction::TXA:
      this->state.a = this->setNz(this->state.x);
      break;
    case Instruction::TXS:
      this->state.s = this->state.x;
      break;
    case Instruction::TYA:
      this->state.a = this->setNz(this->state.y);
      break;
    default:
      throw std::runtime_error("Unknown instruction encountered");
    }
  }
};

namespace Interpret {
Core::Core(const Cpu::Memory::Ptr &mem, Cpu::State state, QObject *parent)
  : Base(mem, state, parent)
{
  this->impl = new InterpretCoreImpl(this, this->m_state, mem);
}

Core::~Core() {
  delete this->impl;
}

void Core::step() {
  this->impl->step();
  this->impl->state.pc = static_cast<uint16_t>(this->impl->disasm->position());
}

int Core::run(int cycles) {
  return this->impl->run(this->m_state.pc, cycles);
}

void Core::jump(uint16_t address) {
  this->m_state.pc = address;
  this->impl->disasm->setPosition(address);
}

void Core::execute(const ::Core::Instruction &instruction) {
  this->impl->execute(instruction);
}

}
