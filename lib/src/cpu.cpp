#include <cpu.hpp>

#include <stdexcept>

namespace Cpu {

int interruptVectorAddress(Interrupt interrupt) {
  switch(interrupt) {
  case NonMaskable: return 0xFFFA;
  case Reset: return 0xFFFC;
  case Break:
  case Service: return 0xFFFE;
  default: throw std::runtime_error("BUG: Unhandeled interrupt kind");
  }
}

bool isInterruptMaskable(Interrupt interrupt) {
  return (interrupt == Service);
}

int flagBit(Flag flag) {
  switch(flag) {
  case Flag::Carry: return 0;
  case Flag::Zero: return 1;
  case Flag::Interrupt: return 2;
  case Flag::Decimal: return 3;
  case Flag::Break: return 4;
  case Flag::AlwaysOne: return 5;
  case Flag::Overflow: return 6;
  case Flag::Negative: return 7;
  default: throw std::runtime_error("BUG: Unhandeled Flag kind");
  }
}

}
