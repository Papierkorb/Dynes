#include <core/gamepad.hpp>

namespace Core {
enum Key {
  A = 0,
  B = 1,
  Select = 2,
  Start = 3,
  Up = 4,
  Down = 5,
  Left = 6,
  Right = 7,
};

Gamepad::Gamepad(uint8_t initial) : m_state(initial), m_pos(0) {
}

static inline void toggle(uint8_t &v, int bit, bool on) {
  if (on)
    v |= (1 << bit);
  else
    v &= ~(1 << bit);
}

void Gamepad::setUp(bool on) {
  toggle(this->m_state, Up, on);
}

void Gamepad::setDown(bool on) {
  toggle(this->m_state, Down, on);
}

void Gamepad::setLeft(bool on) {
  toggle(this->m_state, Left, on);
}

void Gamepad::setRight(bool on) {
  toggle(this->m_state, Right, on);
}

void Gamepad::setA(bool on) {
  toggle(this->m_state, A, on);
}

void Gamepad::setB(bool on) {
  toggle(this->m_state, B, on);
}

void Gamepad::setStart(bool on) {
  toggle(this->m_state, Start, on);
}

void Gamepad::setSelect(bool on) {
  toggle(this->m_state, Select, on);
}

void Gamepad::write(uint8_t value) {
  if (value == 0) this->m_pos = 0;
}

uint8_t Gamepad::read() {
  uint8_t value = 0xFF;

  // A real controller returns all-ones if all buttons have been read from it.
  // If an yet unread button is read, OR with 0x40 to simulate garbage bits
  // coming from unconnected wires.  Some games actually rely on this.

  if (this->m_pos < 8) {
    value = 0x40 | ((this->m_state >> this->m_pos) & 1);
    this->m_pos++;
  }

  return value;
}

}
