#ifndef CORE_DISASSEMBLER_HPP
#define CORE_DISASSEMBLER_HPP

#include "data.hpp"
#include "instruction.hpp"

namespace Core {
/** 6502 instruction disassembler. */
class Disassembler {
public:
  Disassembler(const Data::Ptr &data, int position = 0);

  /**
   * Disassembles the current instruction, and moves the position on to point at
   * the beginning of the following instruction.
   */
  Instruction next();

  /** Current position, in Bytes. */
  int position() const { return this->m_position; }

  /** Moves the current position to \a position. */
  void setPosition(int position);

private:
  uint8_t nextByte();
  uint16_t nextWord();

  int m_position;
  Data::Ptr m_data;
  Data *m_dataPtr;
};
}

#endif // CORE_DISASSEMBLER_HPP
