#include <amd64/function.hpp>
#include <amd64/memorymanager.hpp>

namespace Amd64 {
Function::Function(const Analysis::Function &analyzed, MemoryManager &manager, void *funcPtr)
  : m_analyzed(analyzed), m_manager(manager), m_funcPtr(funcPtr)
{

}

Function::~Function() {
  this->m_manager.remove(this->m_funcPtr);
}

// Defined in src/amd64/guest_call.s
extern "C" void amd64_core_call_guest(void *funcPtr, Cpu::State *state);

Cpu::State::Reason Function::call(Cpu::State &state) {
  // The guest function doesn't use the host ABI, instead it expects:
  //       A in %BL
  //       X in %BH
  //       Y in %R12B
  //       S in %R13B
  //       P in %R14B
  //  Cycles in %R15D
  // These are also the "return" registers.  Additionally it'll return:
  //  Reason in %AL
  //      PC in %CX
  // This convention is basically defined in amd64/constants.hpp, where these
  // register are defined.

  amd64_core_call_guest(this->m_funcPtr, &state);
  return state.reason;
}
}
