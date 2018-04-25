#ifndef CPU_HPP
#define CPU_HPP

#include <QFlags>

namespace Cpu {
  /** Base address of the stack. */
  static constexpr int STACK_BASE = 0x100;

  /** Interrupt vectors offered by the 6502 */
  enum Interrupt {
    NonMaskable,
    Reset,
    Break,
    Service,
  };

  /** Process Status Word flags. */
  enum class Flag : uint8_t {
    Carry = (1 << 0),
    Zero = (1 << 1),
    Interrupt = (1 << 2),
    Decimal = (1 << 3),
    Break = (1 << 4),
    AlwaysOne = (1 << 5),
    Overflow = (1 << 6),
    Negative = (1 << 7),
  };

  Q_DECLARE_FLAGS(Flags, Flag)

  /** Returns the memory address of the \a interrupt vector. */
  int interruptVectorAddress(Interrupt interrupt);

  /** Can the \a interrupt be masked? */
  bool isInterruptMaskable(Interrupt interrupt);

  /** Returns the Bit-position of \a flag */
  int flagBit(Flag flag);

  /** Size of a memory page. */
  static constexpr int PAGE_SIZE = 256;
}

#endif // CPU_HPP
