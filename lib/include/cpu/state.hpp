#ifndef CPU_STATE_HPP
#define CPU_STATE_HPP

#include <cpu.hpp>

namespace Cpu {
/**
 * Internal state of a CPU core.  This structure is mirrored in the dynamic
 * recompiler, changes here \b must be reflected over there too.
 */
struct State {
  /** Exit reasons for the Dynarec. */
  enum class Reason : uint8_t {
    /** The function called \c RTS or \c RTI. */
    Return = 0,

    /** The function called \c BRK. */
    Break = 1,

    /** The alloted cycle count was exhausted. */
    CyclesExhausted = 2,

    /** A \c JMP or \c JSR instruction was encountered. */
    Jump = 3,

    /** A \c JMP pointing to itself was encountered. */
    InfiniteLoop = 4,

    /** An unknown instruction was encountered. */
    UnknownInstruction = 5,
  };

  uint8_t a = 0; ///< Accumulator
  uint8_t x = 0; ///< X register
  uint8_t y = 0; ///< Y register
  uint8_t s = 0; ///< S register (Stack)
  uint8_t p = 0x04; ///< P register (Processor Status Word)
  int32_t cycles = 0; ///< Remaining cycles.
  uint16_t pc = 0; ///< Program Counter
  Reason reason = Reason::Return; ///< Last exit reason, unused by the interpret

  Flags flags() const { return Flags(this->p); }
  void setFlags(Flags f) { this->p = f; }

  /** Toggles \a f in the PSW according to \a active. */
  void setFlag(Flag f, bool active) {
    if (active)
      this->p |= static_cast<uint8_t>(f);
    else
      this->p &= ~static_cast<uint8_t>(f);
  }

  /** Returns \c true if \a f is set in the PSW. */
  bool hasFlag(Flag f) {
    return (this->p & static_cast<uint8_t>(f)) != 0;
  }
} __attribute__((packed));
}

#endif // CPU_STATE_HPP
