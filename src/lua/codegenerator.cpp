#include <lua/codegenerator.hpp>

#include <core/instruction.hpp>

#include <analysis/branch.hpp>
#include <analysis/function.hpp>

#include <cpu/state.hpp>

#include <sstream>
#include <functional>
#include <variant>
#include <set>
/************************* DEBUG FUNCTIONALITY FLAGS **************************/

// Print out instructions before they ran.
//#define LOG_INSTRUCTIONS

/*************************                           **************************/


namespace Lua {

enum Register { A, X, Y, P, S };

using Stream = std::ostringstream; // More convenient to write

struct Ref {
  std::string name;

  // Registers:
  static const Ref a;
  static const Ref x;
  static const Ref y;
  static const Ref s;
  static const Ref p;

  // Unpacked P register:
  static const Ref c;
  static const Ref z;
  static const Ref i;
  static const Ref b;
  static const Ref d;
  static const Ref v;
  static const Ref n;

  // Temporary:
  static const Ref t;
  static const Ref u;
  static const Ref w;
  static const Ref addr; // Temporary resolved address register

  static const Ref &reg(Register name) {
    switch (name) {
    case A: return a;
    case X: return x;
    case Y: return y;
    case S: return s;
    case P: return p;
    }
  }

  static Ref imm(uint8_t v) { return Ref{ std::to_string(v) }; }
  static Ref imm(uint16_t v) { return Ref{ std::to_string(v) }; }
};

const Ref Ref::a{ "a" };
const Ref Ref::x{ "x" };
const Ref Ref::y{ "y" };
const Ref Ref::s{ "s" };
const Ref Ref::p{ "p" };
const Ref Ref::t{ "t" };
const Ref Ref::u{ "u" };
const Ref Ref::w{ "w" };
const Ref Ref::addr{ "addr" };
const Ref Ref::c{ "C" };
const Ref Ref::z{ "Z" };
const Ref Ref::i{ "I" };
const Ref Ref::b{ "B" };
const Ref Ref::d{ "D" };
const Ref Ref::v{ "V" };
const Ref Ref::n{ "N" };

using Callback = std::function<Ref(const Ref &)>;

/** Generation of machine specific code parts. */
struct MachineSpecifics {
  virtual ~MachineSpecifics() { }

  Ref bitTest(const Ref &v, int mask) {
    Ref m{ std::to_string(mask) }; // `((VALUE & MASK) == MASK)`
    return Ref{ "(" + this->bAnd(v, m).name + " == " + m.name + ")" };
  }

  Ref setBit(const Ref &value, const Ref &bit, int shift) {
    std::string mask = std::to_string(1 << shift);
    return this->bOr(value, Ref{ "(" + bit.name + " and " + mask + " or 0)" });
  }

  virtual void prologue(Stream &stream) { (void)stream; }
  virtual Ref bNot(const Ref &v) = 0;
  virtual Ref bAnd(const Ref &l, const Ref &r) = 0;
  virtual Ref bOr(const Ref &l, const Ref &r) = 0;
  virtual Ref bXor(const Ref &l, const Ref &r) = 0;
  virtual Ref bShl(const Ref &l, const Ref &r) = 0;
  virtual Ref bShr(const Ref &l, const Ref &r) = 0;
};

struct Lua53Machine : public MachineSpecifics {
  Ref bAnd(const Ref &l, const Ref &r) override {
    return Ref{ "(" + l.name + " & " + r.name + ")" };
  }

  Ref bOr(const Ref &l, const Ref &r) override {
    return Ref{ "(" + l.name + " | " + r.name + ")" };
  }

  Ref bXor(const Ref &l, const Ref &r) override {
    return Ref{ "(" + l.name + " ~ " + r.name + ")" };
  }

  Ref bNot(const Ref &v) override {
    return Ref{ "(~" + v.name + ")" };
  }

  Ref bShl(const Ref &l, const Ref &r) override {
    return Ref{ "(" + l.name + " << " + r.name + ")" };
  }

  Ref bShr(const Ref &l, const Ref &r) override {
    return Ref{ "(" + l.name + " >> " + r.name + ")" };
  }
};

struct LuaJitMachine : public MachineSpecifics {
  void prologue(Stream &stream) override {
    stream << "local bit = require(\"bit\")\n"
           << "local band = bit.band, bor = bit.bor, bxor = bit.bxor, bnot = bit.bnot\n"
           << "local blshift = bit.blshift, brshift = bit.brshift\n";
  }

  Ref bAnd(const Ref &l, const Ref &r) override {
    return Ref{ "band(" + l.name + ", " + r.name + ")" };
  }

  Ref bOr(const Ref &l, const Ref &r) override {
    return Ref{ "bor(" + l.name + ", " + r.name + ")" };
  }

  Ref bXor(const Ref &l, const Ref &r) override {
    return Ref{ "bxor(" + l.name + ", " + r.name + ")" };
  }

  Ref bNot(const Ref &v) override {
    return Ref{ "bnot(" + v.name + ")" };
  }

  Ref bShl(const Ref &l, const Ref &r) override {
    return Ref{ "blshift(" + l.name + ", " + r.name + ")" };
  }

  Ref bShr(const Ref &l, const Ref &r) override {
    return Ref{ "brshift(" + l.name + ", " + r.name + ")" };
  }
};

struct Context {
  Stream &stream;
  MachineSpecifics &machine;

  Context(Stream &s, MachineSpecifics &m)
    : stream(s), machine(m)
  {
  }
};

struct Translator;

struct Line {
  Stream &stream;

  Line(Stream &s) : stream(s) { }
  Line(Translator *t);

  ~Line() {
    this->stream << '\n';
  }

  template<typename T>
  Line &operator<<(const T &t) {
    this->stream << t;
    return *this;
  }
};

struct Translator {
  Context &ctx;
  Analysis::Function &func;
  std::set<uint16_t> seen;

  Translator(Context &c, Analysis::Function &f) : ctx(c), func(f) { }

  void unpackPsw() {
    this->ctx.stream << "C = " << this->ctx.machine.bitTest(Ref::p, (int)Cpu::Flag::Carry).name << "\n"
                     << "Z = " << this->ctx.machine.bitTest(Ref::p, (int)Cpu::Flag::Zero).name << "\n"
                     << "I = " << this->ctx.machine.bitTest(Ref::p, (int)Cpu::Flag::Interrupt).name << "\n"
                     << "D = " << this->ctx.machine.bitTest(Ref::p, (int)Cpu::Flag::Decimal).name << "\n"
                     << "B = " << this->ctx.machine.bitTest(Ref::p, (int)Cpu::Flag::Break).name << "\n"
                     << "V = " << this->ctx.machine.bitTest(Ref::p, (int)Cpu::Flag::Overflow).name << "\n"
                     << "N = " << this->ctx.machine.bitTest(Ref::p, (int)Cpu::Flag::Negative).name << "\n";
  }

  void packPsw() {
    this->ctx.stream << "p = 0x20\n" // Initialize with the AlwaysOne bit already on.
                     << "p = " << this->ctx.machine.setBit(Ref::p, Ref::c, Cpu::flagBit(Cpu::Flag::Carry)).name << "\n"
                     << "p = " << this->ctx.machine.setBit(Ref::p, Ref::z, Cpu::flagBit(Cpu::Flag::Zero)).name << "\n"
                     << "p = " << this->ctx.machine.setBit(Ref::p, Ref::i, Cpu::flagBit(Cpu::Flag::Interrupt)).name << "\n"
                     << "p = " << this->ctx.machine.setBit(Ref::p, Ref::d, Cpu::flagBit(Cpu::Flag::Decimal)).name << "\n"
                     << "p = " << this->ctx.machine.setBit(Ref::p, Ref::b, Cpu::flagBit(Cpu::Flag::Break)).name << "\n"
                     << "p = " << this->ctx.machine.setBit(Ref::p, Ref::v, Cpu::flagBit(Cpu::Flag::Overflow)).name << "\n"
                     << "p = " << this->ctx.machine.setBit(Ref::p, Ref::n, Cpu::flagBit(Cpu::Flag::Negative)).name << "\n";
  }

  void function() {
    // Prologue: Open the function and create locals.
    this->ctx.stream << "return function(a, x, y, s, p, cycles)\n"
                     << "local pc, reason = 0, t, u, w, addr\n"
                     << "local C, Z, I, D, B, V, N\n";
    this->ctx.machine.prologue(this->ctx.stream);
    this->unpackPsw();

    // Make sure the root branch comes first:
    this->branch(this->func.root());

    // Then compile all other branches.
    for(Analysis::Branch *br : this->func.branches()) this->branch(br);

    // Epilogue: Return new state to the host and close the function.
    this->ctx.stream << "::eof::\n";
    this->packPsw();
    this->ctx.stream << "return a, x, y, s, p, cycles, pc, reason\n"
                     << "end\n";
  }

  void branch(Analysis::Branch *br) {
    for(const Analysis::Branch::Element &el : br->elements()) {
      uint16_t addr = el.first;

      if (!handleInstruction(addr)) continue;

      // Give each instruction a jump label
      Line(this) << "\n::instr_" << addr << "::"; // `::instr_ADDR::`

      Analysis::Branch::Instruction instr = el.second;
      if (auto normal = std::get_if<Core::Instruction>(&instr)) {
        this->putInstructionTrace(addr, *normal);
        this->instruction(addr, *normal);

        // Force sequential execution.  If multiple branches are interspersed,
        // it can happen that the sequential flow in the Lua function doesn't
        // reflect the wanted execution flow.
        if (!normal->isBranching()) {
          uint16_t nextAddr = addr + static_cast<uint16_t>(normal->operandSize()) + 1;
          Line(this) << "goto instr_" << nextAddr;
        }
      } else {
        Analysis::ConditionalInstruction cond = std::get<Analysis::ConditionalInstruction>(instr);
        this->putInstructionTrace(addr, cond);
        this->instruction(cond);
      }
    }
  }

  bool handleInstruction(uint16_t addr) {
    if (this->seen.find(addr) == this->seen.end()) {
      this->seen.insert(addr);
      return true;
    }

    return false;
  }

  void reduceCycleCount(int cycles) {
    Line(this) << "cycles = cycles - " << cycles;
  }

  void putInstructionTrace(uint16_t addr, const Core::Instruction &instr) {
    Line(this) << " -- " << instr.commandName() << " " << instr.addressingName() << " " << instr.op16;

    // Debug functionality of the Lua core: Dump, at run-time, the executed
    // instruction including current register state (BEFORE the instruction was
    // executed!).
#ifdef LOG_INSTRUCTIONS
    char buf[8];
    snprintf(buf, sizeof(buf), "%04x", addr);
    this->ctx.stream << "log('[" << buf << "] "
                     << instr.commandName() << " " << instr.addressingName() << " " << instr.op16 << "\\t "
                     << "A ' .. a .. ' "
                     << "X ' .. x .. ' "
                     << "Y ' .. y .. ' "
                     << "S ' .. s .. ' "
                     << "P ' .. "
                     << "(C and 'C' or 'c') .. "
                     << "(Z and 'Z' or 'z') .. "
                     << "(I and 'I' or 'i') .. "
                     << "(D and 'D' or 'd') .. "
                     << "(B and 'B' or 'b') .. "
                     << "(V and 'V' or 'v') .. "
                     << "(N and 'N' or 'n')"
                     << ")\n";
#endif
  }

  Ref setNz(const Ref &ref) {
    this->ctx.stream << "N = (" << ref.name << " >= 0x80)\n"
                     << "Z = (" << ref.name << " == 0x0)\n";
    return ref;
  }

  Ref trim(const Ref &value) {
    static const Ref byte{ "0xFF" };
    return this->ctx.machine.bAnd(value, byte);
  }

  void returnToHost(const Ref &pc, Cpu::State::Reason reason) {
    this->ctx.stream << "pc = " << pc.name << "\n"
                     << "reason = " << static_cast<int>(reason) << "\n"
                     << "goto eof\n";
  }

  void compare(const Ref &reg, const Ref &op) {
    this->ctx.stream << "t = " << op.name << "\n"
                     << "u = " << this->trim(Ref{ reg.name + " - t" }).name << "\n" // `u = reg - op`
                     << "C = (" << reg.name << " >= t)\n"; // `C = (reg >= op)`
    this->setNz(Ref::u);
  }

  void push8(const Ref &value) {
    this->ctx.stream << "write(s + 0x100, " << value.name << ")\n"
                     << "s = " << this->ctx.machine.bAnd({ "(s - 1)" }, { "0xFF" }).name << "\n";
  }

  void pull8(const Ref &into) {
    this->ctx.stream << "s = " << this->ctx.machine.bAnd({ "(s + 1)" }, { "0xFF" }).name << "\n"
                     << into.name << " = read(s + 0x100)\n";
  }

  void pull16(const Ref &into) {
    this->ctx.stream << into.name << " = read16(s + 0x101)\n"
                     << "s = " << this->ctx.machine.bAnd({ "(s + 2)" }, { "0xFF" }).name << "\n";
  }

  void adc(const Ref &op) {
    static const Ref const0x80{ "0x80" };
    // T = Operand + C
    // U = A + T  (=> Later: A = U & 0xFF)
    // V = ~(U ^ A) & (U ^ U) & 0x80 != 0
    // ~(left ^ right) & (left ^ value) & 0x80;
    Ref axt = this->ctx.machine.bNot(this->ctx.machine.bXor(Ref::a, Ref::t));
    Ref axu = this->ctx.machine.bXor(Ref::a, Ref::u);
    Ref v = this->ctx.machine.bAnd(this->ctx.machine.bAnd(axt, axu), const0x80);

    this->ctx.stream << "t = " << op.name << "\n"     // T = Op
                     << "w = a + (C and 1 or 0)\n"    // W = A + C
                     << "u = w + t\n"                 // U = W + T  (U = Op + A + C)
                     << "V = " << v.name << " ~= 0\n" // V = ~(A ^ T) & (A ^ U) & 0x80 != 0
                     << "C = (u > 0xFF)\n"            // C = U > 0xFF
                     << "a = " << this->trim(Ref::u).name << "\n";
    this->setNz(Ref::a);
  }

  void instruction(uint16_t address, Core::Instruction instr) {
    using Core::Instruction;
    static const Ref const0x01{ "0x01" };
    static const Ref const0x80{ "0x80" };
    static const Ref const0xFF{ "0xFF" };

    // Next address is this instructions address + operand size + opcode byte.
    uint16_t nextAddr = address + static_cast<uint16_t>(instr.operandSize()) + 1;
    this->reduceCycleCount(instr.cycles);

    switch (instr.command) {
    case Instruction::ADC:
      this->adc(this->read(instr));
      break;
    case Instruction::AND:
      Line(this) << "a = " << this->ctx.machine.bAnd(Ref::a, this->read(instr)).name;
      this->setNz(Ref::a);
      break;
    case Instruction::ASL:
      this->rmw(instr, [this](const Ref &value) {
        Line(this) << "t = " << value.name;
        Line(this) << "C = (t >= 0x80)";
        return this->setNz(this->trim(this->ctx.machine.bShl(Ref::t, const0x01)));
      });
      break;
    case Instruction::BIT: {
      this->ctx.stream << "t = " << this->read(instr).name << "\n"
                       << "Z = (" << this->ctx.machine.bAnd(Ref::a, Ref::t).name << " == 0)\n"
                       << "V = " << this->ctx.machine.bitTest(Ref::t, (int)Cpu::Flag::Overflow).name << "\n"
                       << "N = " << this->ctx.machine.bitTest(Ref::t, (int)Cpu::Flag::Negative).name << "\n";
      break;
    }
    case Instruction::BRK:
      this->returnToHost(Ref::imm(nextAddr), Cpu::State::Reason::Break);
      break;
    case Instruction::CLC:
      Line(this) << "C = false";
      break;
    case Instruction::CLD:
      Line(this) << "D = false";
      break;
    case Instruction::CLI:
      Line(this) << "I = false";
      break;
    case Instruction::CLV:
      Line(this) << "V = false";
      break;
    case Instruction::CMP:
      this->compare(Ref::a, this->read(instr));
      break;
    case Instruction::CPX:
      this->compare(Ref::x, this->read(instr));
      break;
    case Instruction::CPY:
      this->compare(Ref::y, this->read(instr));
      break;
    case Instruction::DEC:
    case Instruction::DEX:
    case Instruction::DEY:
      this->rmw(instr, [this](Ref value) {
        Line(this) << "t = " << value.name << " - 1";
        return this->setNz(this->trim(Ref::t));
      });
      break;
    case Instruction::EOR:
      Line(this) << "a = " << this->ctx.machine.bXor(Ref::a, this->read(instr)).name;
      this->setNz(Ref::a);
      break;
    case Instruction::INC:
    case Instruction::INX:
    case Instruction::INY:
      this->rmw(instr, [this](Ref value) {
        Line(this) << "t = " << value.name << " + 1";
        return this->setNz(this->trim(Ref::t));
      });
      break;
    case Instruction::JMP:
      this->ctx.stream << "pc = " << this->resolve(instr).name << "\n"
                       << "if pc == " << address << " then\n"
                       << "  reason = " << static_cast<int>(Cpu::State::Reason::InfiniteLoop) << "\n"
                       << "else\n"
                       << "  reason = " << static_cast<int>(Cpu::State::Reason::Jump) << "\n"
                       << "end\n"
                       << "goto eof\n";
      break;
    case Instruction::JSR: {
      uint16_t target = nextAddr - 1;
      uint8_t hi = static_cast<uint8_t>(target >> 8);
      uint8_t lo = static_cast<uint8_t>(target);

      this->push8(Ref::imm(hi));
      this->push8(Ref::imm(lo));
      this->returnToHost(Ref::imm(instr.op16), Cpu::State::Reason::Jump);
      break;
    }
    case Instruction::LDA:
      Line(this) << "a = " << this->read(instr).name;
      this->setNz(Ref::a);
      break;
    case Instruction::LDX:
      Line(this) << "x = " << this->read(instr).name;
      this->setNz(Ref::x);
      break;
    case Instruction::LDY:
      Line(this) << "y = " << this->read(instr).name;
      this->setNz(Ref::y);
      break;
    case Instruction::LSR:
      this->rmw(instr, [this](const Ref &value) {
        Line(this) << "t = " << value.name;
        Line(this) << "C = (" + this->ctx.machine.bAnd(Ref::t, const0x01).name + " == 1)";
        return this->setNz(this->ctx.machine.bShr(Ref::t, const0x01));
      });
      break;
    case Instruction::NOP: /* Nothing. */ break;
    case Instruction::ORA:
      Line(this) << "a = " << this->ctx.machine.bOr(Ref::a, this->read(instr)).name;
      this->setNz(Ref::a);
      break;
    case Instruction::PHA:
      this->push8(Ref::a);
      break;
    case Instruction::PHP: {
      this->packPsw();

      // `push(p | Break)`
      Ref psw = this->ctx.machine.bOr(Ref::p, Ref::imm(static_cast<uint8_t>(Cpu::Flag::Break)));
      this->push8(psw);
      break;
    }
    case Instruction::PLA:
      this->pull8(Ref::a);
      this->setNz(Ref::a);
      break;
    case Instruction::PLP:
      this->pull8(Ref::p);
      this->unpackPsw();
      break;
    case Instruction::ROL:
      this->rmw(instr, [this](const Ref &value) {
        Line(this) << "t = " << value.name;
        Line(this) << "u = (C and 1 or 0)";
        Line(this) << "C = (" + this->ctx.machine.bAnd(Ref::t, const0x80).name + " == 0x80)";
        return this->setNz(this->trim(this->ctx.machine.bOr(Ref::u, this->ctx.machine.bShl(Ref::t, const0x01))));
      });
      break;
    case Instruction::ROR:
      this->rmw(instr, [this](const Ref &value) {
        Line(this) << "t = " << value.name;
        Line(this) << "u = (C and 0x80 or 0)";
        Line(this) << "C = (" + this->ctx.machine.bAnd(Ref::t, const0x01).name + " == 0x01)";
        return this->setNz(this->trim(this->ctx.machine.bOr(Ref::u, this->ctx.machine.bShr(Ref::t, const0x01))));
      });
      break;
    case Instruction::RTI:
      this->pull8(Ref::p);
      this->pull16(Ref::addr);
      this->unpackPsw();
      this->returnToHost(Ref::addr, Cpu::State::Reason::Return);
      break;
    case Instruction::RTS:
      this->pull16(Ref::addr);
      this->returnToHost(Ref{ "addr + 1" }, Cpu::State::Reason::Return);
      break;
    case Instruction::SBC:
      // Invert using 1s complement, the Carry will then adjust.
      this->adc(this->ctx.machine.bXor(this->read(instr), const0xFF));
      break;
    case Instruction::SEC:
      Line(this) << "C = true";
      break;
    case Instruction::SED:
      Line(this) << "D = true";
      break;
    case Instruction::SEI:
      Line(this) << "I = true";
      break;
    case Instruction::STA:
      this->write(instr, Ref::a);
      break;
    case Instruction::STX:
      this->write(instr, Ref::x);
      break;
    case Instruction::STY:
      this->write(instr, Ref::y);
      break;
    case Instruction::TAX:
      Line(this) << "x = a";
      this->setNz(Ref::x);
      break;
    case Instruction::TAY:
      Line(this) << "y = a";
      this->setNz(Ref::y);
      break;
    case Instruction::TSX:
      Line(this) << "x = s";
      this->setNz(Ref::x);
      break;
    case Instruction::TXA:
      Line(this) << "a = x";
      this->setNz(Ref::a);
      break;
    case Instruction::TXS:
      Line(this) << "s = x";
      break;
    case Instruction::TYA:
      Line(this) << "a = y";
      this->setNz(Ref::a);
      break;
    default:
      this->returnToHost(Ref::imm(address), Cpu::State::Reason::UnknownInstruction);
      break;
    }
  }

  const Ref &conditionTest(Analysis::ConditionalInstruction instr) {
    using Core::Instruction;

    static const Ref bcc{ "(C == false)" };
    static const Ref bcs{ "(C == true)" };
    static const Ref beq{ "(Z == true)" };
    static const Ref bne{ "(Z == false)" };
    static const Ref bmi{ "(N == true)" };
    static const Ref bpl{ "(N == false)" };
    static const Ref bvs{ "(V == true)" };
    static const Ref bvc{ "(V == false)" };

    // Figure out the type of the test:
    switch (instr.command) {
    case Instruction::BCC: return bcc;
    case Instruction::BCS: return bcs;
    case Instruction::BEQ: return beq;
    case Instruction::BMI: return bmi;
    case Instruction::BNE: return bne;
    case Instruction::BPL: return bpl;
    case Instruction::BVC: return bvc;
    case Instruction::BVS: return bvs;
    default:
      throw std::runtime_error("Missing case for conditional instruction!");
    }
  }

  void instruction(Analysis::ConditionalInstruction instr) {
    this->reduceCycleCount(instr.cycles);

    uint16_t truthy = instr.trueBranch()->start();
    uint16_t falsy = instr.falseBranch()->start();
    Ref condition = this->conditionTest(instr);

    // First check for cycle exhaustion.  If no cycles are left compute the
    // re-entry branch and return to the host.
    this->ctx.stream << "if cycles <= 0 then\n"
                     << "  if " << condition.name << " then\n"
                     << "    pc = " << truthy << "\n"
                     << "  else\n"
                     << "    pc = " << falsy << "\n"
                     << "  end\n"
                     << "  reason = " << (int)Cpu::State::Reason::CyclesExhausted << "\n"
                     << "  goto eof\n"
                     << "end\n";

    // There are still cycles left, carry on and jump directly.
    this->ctx.stream << "if " << condition.name << " then\n"
                     << "  goto instr_" << truthy << "\n"
                     << "else\n"
                     << "  goto instr_" << falsy << "\n"
                     << "end\n";
  }

  /** Resolves the \a instr to an absolute memory address. */
  Ref resolve(const Core::Instruction &instr) {
    return this->resolve(instr.addressing, instr.op16);
  }

  /**
   * Resolves the (maybe relative) \a addr to an absolute address using \a mode.
   */
  Ref resolve(Core::Instruction::Addressing mode, uint16_t addr) {
    using Core::Instruction;
    static const Ref const0xFF{ "0xFF" };
    static const Ref const0xFFFF{ "0xFFFF" };
    uint8_t addr8 = static_cast<uint8_t>(addr);

    switch(mode) {
    default: // Ignore other modes.
      return Ref::imm(static_cast<uint8_t>(0));
    case Instruction::Zp:
      return Ref::imm(addr8);
    case Instruction::ZpX:
      return this->ctx.machine.bAnd(Ref{ "(" + std::to_string(addr) + " + x)" }, const0xFF);
    case Instruction::ZpY:
      return this->ctx.machine.bAnd(Ref{ "(" + std::to_string(addr) + " + y)" }, const0xFF);
    case Instruction::Abs:
      return Ref::imm(addr);
    case Instruction::AbsX:
      return this->ctx.machine.bAnd(Ref{ "(" + std::to_string(addr) + " + x)" }, const0xFFFF);
    case Instruction::AbsY:
      return this->ctx.machine.bAnd(Ref{ "(" + std::to_string(addr) + " + y)" }, const0xFFFF);
    case Instruction::Ind:
      return Ref{ "read16(" + std::to_string(addr) + ")" };
    case Instruction::IndX: {
      Ref zpx = this->ctx.machine.bAnd(Ref{ "(" + std::to_string(addr8) + " + x)"}, const0xFF);
      return Ref{ "read16(" + zpx.name + ")" };
    }
    case Instruction::IndY: {
      Ref aby = Ref{ "(read16(" + std::to_string(addr8) + ") + y)" };
      return this->ctx.machine.bAnd(aby, const0xFFFF);
    }
    }
  }

  /** Reads the byte \a instr is pointing at, be it a memory address or a register. */
  Ref read(const Core::Instruction &instr) {
    return this->read(instr.addressing, instr.op16);
  }

  /** Reads the byte addressed by \a mode at \a addr. */
  Ref read(Core::Instruction::Addressing mode, uint16_t addr) {
    using Core::Instruction;

    switch(mode) {
    case Instruction::Acc: return Ref::a;
    case Instruction::X: return Ref::x;
    case Instruction::Y: return Ref::y;
    case Instruction::S: return Ref::s;
    case Instruction::P: return Ref::p;
    case Instruction::Imm:
    case Instruction::Imp:
    case Instruction::Rel:
      return Ref::imm(static_cast<uint8_t>(addr));
    default: // Resolve and read from memory.
      return Ref{ "read(" + this->resolve(mode, addr).name + ")" };
    }
  }

  /** Writes the \a value into what \a instr is pointing at. */
  void write(const Core::Instruction &instr, const Ref &ref) {
    this->write(instr.addressing, instr.op16, ref);
  }

  /** Writes \a value into the byte addressed by \a mode at \a addr. */
  void write(Core::Instruction::Addressing mode, uint16_t addr, const Ref &ref) {
    using Core::Instruction;

    switch(mode) {
    case Instruction::Acc:
      Line(this) << Ref::a.name << " = " << ref.name;
      break;
    case Instruction::X:
      Line(this) << Ref::x.name << " = " << ref.name;
      break;
    case Instruction::Y:
      Line(this) << Ref::y.name << " = " << ref.name;
      break;
    case Instruction::S:
      Line(this) << Ref::s.name << " = " << ref.name;
      break;
    case Instruction::P:
      Line(this) << Ref::p.name << " = " << ref.name;
      break;
    case Instruction::Imm:
    case Instruction::Imp:
    case Instruction::Rel:
      throw std::runtime_error("Can't write to Imm/Imp/Rel addressing instruction");
      break; // Ignore.
    default: // Resolve and write to memory.
      Line(this) << "write(" << this->resolve(mode, addr).name << ", " << ref.name << ")";
      break;
    }
  }

  /**
   * Reads the byte \a instr is pointing at.  This byte is then passed to \a proc.
   * The result of \a proc is then written back into the same place.
   */
  void rmw(const Core::Instruction &instr, Callback proc) {
    rmw(instr.addressing, instr.op16, proc);
  }

  /** Like the \c Core::Instruction version. */
  void rmw(Core::Instruction::Addressing mode, uint16_t addr, Callback proc) {
    using Core::Instruction;
    static const Ref readAddr{ "read(addr)" };
    std::string r;

    // Call proc first, and only afterwards emit the write access.  This is to
    // support multi-Lua-line instructions.
    switch(mode) {
    case Instruction::Acc:
      r = proc(Ref::a).name;
      Line(this) << Ref::a.name << " = " << r;
      break;
    case Instruction::X:
      r = proc(Ref::x).name;
      Line(this) << Ref::x.name << " = " << r;
      break;
    case Instruction::Y:
      r = proc(Ref::y).name;
      Line(this) << Ref::y.name << " = " << r;
      break;
    case Instruction::S:
      r = proc(Ref::s).name;
      Line(this) << Ref::s.name << " = " << r;
      break;
    case Instruction::P:
      r = proc(Ref::p).name;
      Line(this) << Ref::p.name << " = " << r;
      break;
    case Instruction::Imm:
      r = proc(Ref::imm(static_cast<uint8_t>(addr))).name;
      Line(this) << Ref::a.name << " = " << r;
      break;
    case Instruction::Rel:
    case Instruction::Imp:
      throw std::runtime_error("Can't RMW on a Rel/Imp adressing instruction");
      break;
    default: { // Read and write to memory.
      Line(this) << "addr = " << this->resolve(mode, addr).name;
      r = proc(readAddr).name;
      Line(this) << "write(addr, " << r << ") \n";
      break;
    }
    }
  }
};

Line::Line(Translator *t) : stream(t->ctx.stream) { }

CodeGenerator::CodeGenerator(Machine machine) {
  switch (machine) {
  case CodeGenerator::Lua53:
    this->m_machine = new Lua53Machine;
    break;
  case CodeGenerator::LuaJit:
    throw std::runtime_error("Not implemented.");
//    this->m_machine = new LuaJitMachine;
    break;
  }
}

CodeGenerator::~CodeGenerator() {
  delete this->m_machine;
}

std::string CodeGenerator::translate(Analysis::Function &func) {
  Stream b;
  Context ctx(b, *this->m_machine);
  Translator translator(ctx, func);

  translator.function();

  return b.str();
}
}
