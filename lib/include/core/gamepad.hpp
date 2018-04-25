#ifndef CORE_GAMEPAD_HPP
#define CORE_GAMEPAD_HPP

#include <cstdint>

namespace Core {
/** Simulates a standard game pad for input. */
class Gamepad {
public:
  Gamepad(uint8_t initial = 0);

  void setUp(bool on);
  void setDown(bool on);
  void setLeft(bool on);
  void setRight(bool on);
  void setA(bool on);
  void setB(bool on);
  void setStart(bool on);
  void setSelect(bool on);

  /** Fetches the next serial state byte. */
  uint8_t read();

  /** Sends \a value to the pad. */
  void write(uint8_t value);

private:
  uint8_t m_state;
  uint8_t m_pos;
};
}

#endif // CORE_GAMEPAD_HPP
