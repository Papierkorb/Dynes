#include <core/instruction.hpp>

namespace Core {

int Instruction::operandSize() const {
  switch (this->addressing) {
  case Acc:
  case X:
  case Y:
  case S:
  case P: return 0;
  case Imp: return 0;
  case Imm: return 1;
  case Rel: return 1;
  case Zp: return 1;
  case ZpX: return 1;
  case ZpY: return 1;
  case Abs: return 2;
  case AbsX: return 2;
  case AbsY: return 2;
  case Ind: return 2;
  case IndX: return 1;
  case IndY: return 1;
  default: return 0;
  }
}

const char *Instruction::commandName() const {
  switch(this->command) {
  case Unknown: return "???";
  case ADC: return "ADC";
  case AND: return "AND";
  case ASL: return "ASL";
  case BCC: return "BCC";
  case BCS: return "BCS";
  case BEQ: return "BEQ";
  case BIT: return "BIT";
  case BMI: return "BMI";
  case BNE: return "BNE";
  case BPL: return "BPL";
  case BRK: return "BRK";
  case BVC: return "BVC";
  case BVS: return "BVS";
  case CLC: return "CLC";
  case CLD: return "CLD";
  case CLI: return "CLI";
  case CLV: return "CLV";
  case CMP: return "CMP";
  case CPX: return "CPX";
  case CPY: return "CPY";
  case DEC: return "DEC";
  case DEX: return "DEX";
  case DEY: return "DEY";
  case EOR: return "EOR";
  case INC: return "INC";
  case INX: return "INX";
  case INY: return "INY";
  case JMP: return "JMP";
  case JSR: return "JSR";
  case LDA: return "LDA";
  case LDX: return "LDX";
  case LDY: return "LDY";
  case LSR: return "LSR";
  case NOP: return "NOP";
  case ORA: return "ORA";
  case PHA: return "PHA";
  case PHP: return "PHP";
  case PLA: return "PLA";
  case PLP: return "PLP";
  case ROL: return "ROL";
  case ROR: return "ROR";
  case RTI: return "RTI";
  case RTS: return "RTS";
  case SBC: return "SBC";
  case SEC: return "SEC";
  case SED: return "SED";
  case SEI: return "SEI";
  case STA: return "STA";
  case STX: return "STX";
  case STY: return "STY";
  case TAX: return "TAX";
  case TAY: return "TAY";
  case TSX: return "TSX";
  case TXA: return "TXA";
  case TXS: return "TXS";
  case TYA: return "TYA";
  default: return "!?!";
  }
}

const char *Instruction::addressingName() const {
  switch(this->addressing) {
  case Acc: return "Acc";
  case X: return "X";
  case Y: return "Y";
  case S: return "S";
  case P: return "P";
  case Imm: return "Imm";
  case Imp: return "Imp";
  case Rel: return "Rel";
  case Zp: return "Zp";
  case ZpX: return "ZpX";
  case ZpY: return "ZpY";
  case Abs: return "Abs";
  case AbsX: return "AbsX";
  case AbsY: return "AbsY";
  case Ind: return "Ind";
  case IndX: return "IndX";
  case IndY: return "IndY";
  default: return "???";
  }
}

bool Instruction::isMemory() const {
  return isMemory(this->addressing);
}

bool Instruction::isMemory(Instruction::Addressing mode) {
  switch (mode) {
  case Zp:
  case ZpX:
  case ZpY:
  case Abs:
  case AbsX:
  case AbsY:
  case Ind:
  case IndX:
  case IndY:
    return true;
  default:
    return false;
  }
}

bool Instruction::isBranching() const {
  switch (this->command) {
  case BCC:
  case BCS:
  case BEQ:
  case BMI:
  case BNE:
  case BPL:
  case BRK:
  case BVC:
  case BVS:
  case JMP:
  case RTI:
  case RTS:
  case JSR:
  case Unknown:
    return true;
  default:
    return false;
  }
}

bool Instruction::isConditionalBranching() const {
  switch (this->command) {
  case BCC:
  case BCS:
  case BEQ:
  case BMI:
  case BNE:
  case BPL:
  case BVC:
  case BVS:
    return true;
  default:
    return false;
  }
}

uint16_t Instruction::destinationAddress(uint16_t base) const {
  return static_cast<uint16_t>(base + this->ops8);
}

Instruction Instruction::decode(uint8_t opcode) {
  // No support for "illegal" instructions.  A few "illegal" opcodes that are
  // basically aliases are supported however.

  switch (opcode) {
  case 0x18: return Instruction(CLC, Imp, 2);
  case 0x38: return Instruction(SEC, Imp, 2);
  case 0x58: return Instruction(CLI, Imp, 2);
  case 0x78: return Instruction(SEI, Imp, 2);
  case 0xB8: return Instruction(CLV, Imp, 2);
  case 0xD8: return Instruction(CLD, Imp, 2);
  case 0xF8: return Instruction(SED, Imp, 2);
  case 0x10: return Instruction(BPL, Rel, 2);
  case 0x30: return Instruction(BMI, Rel, 2);
  case 0x50: return Instruction(BVC, Rel, 2);
  case 0x70: return Instruction(BVS, Rel, 2);
  case 0x90: return Instruction(BCC, Rel, 2);
  case 0xB0: return Instruction(BCS, Rel, 2);
  case 0xD0: return Instruction(BNE, Rel, 2);
  case 0xF0: return Instruction(BEQ, Rel, 2);
  case 0xAA: return Instruction(TAX, X, 2);
  case 0x8A: return Instruction(TXA, Acc, 2);
  case 0xCA: return Instruction(DEX, X, 2);
  case 0xE8: return Instruction(INX, X, 2);
  case 0xA8: return Instruction(TAY, Y, 2);
  case 0x98: return Instruction(TYA, Acc, 2);
  case 0x88: return Instruction(DEY, Y, 2);
  case 0xC8: return Instruction(INY, Y, 2);
  case 0x9A: return Instruction(TXS, S, 2);
  case 0xBA: return Instruction(TSX, X, 2);
  case 0x48: return Instruction(PHA, Acc, 3);
  case 0x68: return Instruction(PLA, Acc, 4);
  case 0x08: return Instruction(PHP, P, 3);
  case 0x28: return Instruction(PLP, P, 4);
  case 0x4C: return Instruction(JMP, Abs, 3);
  case 0x6C: return Instruction(JMP, Ind, 5);
  case 0x69: return Instruction(ADC, Imm, 2);
  case 0x65: return Instruction(ADC, Zp, 3);
  case 0x75: return Instruction(ADC, ZpX, 4);
  case 0x6D: return Instruction(ADC, Abs, 4);
  case 0x7D: return Instruction(ADC, AbsX, 4);
  case 0x79: return Instruction(ADC, AbsY, 4);
  case 0x61: return Instruction(ADC, IndX, 6);
  case 0x71: return Instruction(ADC, IndY, 5);
  case 0x29: return Instruction(AND, Imm, 2);
  case 0x25: return Instruction(AND, Zp, 3);
  case 0x35: return Instruction(AND, ZpX, 4);
  case 0x2D: return Instruction(AND, Abs, 4);
  case 0x3D: return Instruction(AND, AbsX, 4);
  case 0x39: return Instruction(AND, AbsY, 4);
  case 0x21: return Instruction(AND, IndX, 6);
  case 0x31: return Instruction(AND, IndY, 5);
  case 0x0A: return Instruction(ASL, Acc, 2);
  case 0x06: return Instruction(ASL, Zp, 5);
  case 0x16: return Instruction(ASL, ZpX, 6);
  case 0x0E: return Instruction(ASL, Abs, 6);
  case 0x1E: return Instruction(ASL, AbsX, 7);
  case 0x24: return Instruction(BIT, Zp, 3);
  case 0x2C: return Instruction(BIT, Abs, 4);
  case 0x00: return Instruction(BRK, Imm, 7);
  case 0xC9: return Instruction(CMP, Imm, 2);
  case 0xC5: return Instruction(CMP, Zp, 3);
  case 0xD5: return Instruction(CMP, ZpX, 4);
  case 0xCD: return Instruction(CMP, Abs, 4);
  case 0xDD: return Instruction(CMP, AbsX, 4);
  case 0xD9: return Instruction(CMP, AbsY, 4);
  case 0xC1: return Instruction(CMP, IndX, 6);
  case 0xD1: return Instruction(CMP, IndY, 5);
  case 0xE0: return Instruction(CPX, Imm, 2);
  case 0xE4: return Instruction(CPX, Zp, 3);
  case 0xEC: return Instruction(CPX, Abs, 4);
  case 0xC0: return Instruction(CPY, Imm, 2);
  case 0xC4: return Instruction(CPY, Zp, 3);
  case 0xCC: return Instruction(CPY, Abs, 4);
  case 0xC6: return Instruction(DEC, Zp, 5);
  case 0xD6: return Instruction(DEC, ZpX, 6);
  case 0xCE: return Instruction(DEC, Abs, 6);
  case 0xDE: return Instruction(DEC, AbsX, 7);
  case 0x49: return Instruction(EOR, Imm, 2);
  case 0x45: return Instruction(EOR, Zp, 3);
  case 0x55: return Instruction(EOR, ZpX, 4);
  case 0x4D: return Instruction(EOR, Abs, 4);
  case 0x5D: return Instruction(EOR, AbsX, 4);
  case 0x59: return Instruction(EOR, AbsY, 4);
  case 0x41: return Instruction(EOR, IndX, 6);
  case 0x51: return Instruction(EOR, IndY, 5);
  case 0xE6: return Instruction(INC, Zp, 5);
  case 0xF6: return Instruction(INC, ZpX, 6);
  case 0xEE: return Instruction(INC, Abs, 6);
  case 0xFE: return Instruction(INC, AbsX, 7);
  case 0x20: return Instruction(JSR, Abs, 6);
  case 0xA9: return Instruction(LDA, Imm, 2);
  case 0xA5: return Instruction(LDA, Zp, 3);
  case 0xB5: return Instruction(LDA, ZpX, 4);
  case 0xAD: return Instruction(LDA, Abs, 4);
  case 0xBD: return Instruction(LDA, AbsX, 4);
  case 0xB9: return Instruction(LDA, AbsY, 4);
  case 0xA1: return Instruction(LDA, IndX, 6);
  case 0xB1: return Instruction(LDA, IndY, 5);
  case 0xA2: return Instruction(LDX, Imm, 2);
  case 0xA6: return Instruction(LDX, Zp, 3);
  case 0xB6: return Instruction(LDX, ZpY, 4);
  case 0xAE: return Instruction(LDX, Abs, 4);
  case 0xBE: return Instruction(LDX, AbsY, 4);
  case 0xA0: return Instruction(LDY, Imm, 2);
  case 0xA4: return Instruction(LDY, Zp, 3);
  case 0xB4: return Instruction(LDY, ZpX, 4);
  case 0xAC: return Instruction(LDY, Abs, 4);
  case 0xBC: return Instruction(LDY, AbsX, 4);
  case 0x4A: return Instruction(LSR, Acc, 2);
  case 0x46: return Instruction(LSR, Zp, 5);
  case 0x56: return Instruction(LSR, ZpX, 6);
  case 0x4E: return Instruction(LSR, Abs, 6);
  case 0x5E: return Instruction(LSR, AbsX, 7);
  case 0x1A:
  case 0x3A:
  case 0x5A:
  case 0x7A:
  case 0xDA:
  case 0xFA:
  case 0xEA: return Instruction(NOP, Imp, 2);
  case 0x09: return Instruction(ORA, Imm, 2);
  case 0x05: return Instruction(ORA, Zp, 3);
  case 0x15: return Instruction(ORA, ZpX, 4);
  case 0x0D: return Instruction(ORA, Abs, 4);
  case 0x1D: return Instruction(ORA, AbsX, 4);
  case 0x19: return Instruction(ORA, AbsY, 4);
  case 0x01: return Instruction(ORA, IndX, 6);
  case 0x11: return Instruction(ORA, IndY, 5);
  case 0x2A: return Instruction(ROL, Acc, 2);
  case 0x26: return Instruction(ROL, Zp, 5);
  case 0x36: return Instruction(ROL, ZpX, 6);
  case 0x2E: return Instruction(ROL, Abs, 6);
  case 0x3E: return Instruction(ROL, AbsX, 7);
  case 0x6A: return Instruction(ROR, Acc, 2);
  case 0x66: return Instruction(ROR, Zp, 5);
  case 0x76: return Instruction(ROR, ZpX, 6);
  case 0x6E: return Instruction(ROR, Abs, 6);
  case 0x7E: return Instruction(ROR, AbsX, 7);
  case 0x40: return Instruction(RTI, Imp, 6);
  case 0x60: return Instruction(RTS, Imp, 6);
  case 0xEB:
  case 0xE9: return Instruction(SBC, Imm, 2);
  case 0xE5: return Instruction(SBC, Zp, 3);
  case 0xF5: return Instruction(SBC, ZpX, 4);
  case 0xED: return Instruction(SBC, Abs, 4);
  case 0xFD: return Instruction(SBC, AbsX, 4);
  case 0xF9: return Instruction(SBC, AbsY, 4);
  case 0xE1: return Instruction(SBC, IndX, 6);
  case 0xF1: return Instruction(SBC, IndY, 5);
  case 0x85: return Instruction(STA, Zp, 3);
  case 0x95: return Instruction(STA, ZpX, 4);
  case 0x8D: return Instruction(STA, Abs, 4);
  case 0x9D: return Instruction(STA, AbsX, 5);
  case 0x99: return Instruction(STA, AbsY, 5);
  case 0x81: return Instruction(STA, IndX, 6);
  case 0x91: return Instruction(STA, IndY, 6);
  case 0x86: return Instruction(STX, Zp, 3);
  case 0x96: return Instruction(STX, ZpY, 4);
  case 0x8E: return Instruction(STX, Abs, 4);
  case 0x84: return Instruction(STY, Zp, 3);
  case 0x94: return Instruction(STY, ZpX, 4);
  case 0x8C: return Instruction(STY, Abs, 4);
  default: return Instruction(Unknown, Imp, 1);
  }
}

}
