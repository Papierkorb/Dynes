#ifndef CPU_HOOK_HPP
#define CPU_HOOK_HPP

#include <core/instruction.hpp>
#include "state.hpp"

namespace Cpu {
/**
 * Interface for an instruction-observing hook in a CPU core.
 *
 * This is an optional feature for CPU cores to support, and thus, may \b not
 * work with all implementations!
 *
 * \sa Cpu::Base::setHook()
 */
class Hook {
public:
  virtual ~Hook();

  /** Called before an instruction is executed. */
  virtual void beforeInstruction(Core::Instruction instruction, Cpu::State &state);

  /** Called after an instruction was executed. */
  virtual void afterInstruction(Core::Instruction instruction, Cpu::State &state);
};
}

#endif // CPU_HOOK_HPP
