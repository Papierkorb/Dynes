#include <core/disassembler.hpp>

namespace Core {
Disassembler::Disassembler(const Data::Ptr &data, int position)
  : m_position(position), m_data(data)
{
  this->m_dataPtr = this->m_data.get();
}

Instruction Disassembler::next() {
  Instruction instr = Instruction::decode(this->nextByte());

  switch(instr.operandSize()) {
  case 1:
    instr.op8 = this->nextByte();
    break;
  case 2:
    instr.op16 = this->nextWord();
    break;
  }

  return instr;
}

void Disassembler::setPosition(int position) {
  this->m_position = position;
}

uint8_t Disassembler::nextByte() {
  uint8_t byte = this->m_dataPtr->read(this->m_position);
  this->m_position++;
  return byte;
}

uint16_t Disassembler::nextWord() {
  int pos = this->m_position;

  uint16_t lo = static_cast<uint16_t>(this->m_dataPtr->read(pos + 0));
  uint16_t hi = static_cast<uint16_t>(this->m_dataPtr->read(pos + 1));

  this->m_position = pos + 2;
  return (hi << 8) | lo;
}
}
