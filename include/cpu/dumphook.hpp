#ifndef CPU_DUMPHOOK_HPP
#define CPU_DUMPHOOK_HPP

#include "hook.hpp"

namespace Cpu {
/**
 * Debug \c Hook dumping all ran instructions, including full state, onto
 * standard error.
 */
class DumpHook : public Hook {
public:
  DumpHook();
  ~DumpHook() override;

  virtual void beforeInstruction(Core::Instruction instruction, State &state) override;
  virtual void afterInstruction(Core::Instruction instruction, State &state) override;
private:
  uint16_t m_currentPc;
};
}

#endif // DUMPHOOK_HPP
