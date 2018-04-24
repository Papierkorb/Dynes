#ifndef AMD64_FUNCTION_HPP
#define AMD64_FUNCTION_HPP

#include <analysis/function.hpp>
#include <cpu/state.hpp>

namespace Amd64 {
class MemoryManager;

/**
 * Container for a callable, fully assembled function.
 */
class Function {
public:
  Function(const Analysis::Function &analyzed, MemoryManager &manager, void *funcPtr);
  ~Function();

  /** The base function data. */
  Analysis::Function &analyzed() { return this->m_analyzed; }

  /** The base function data. */
  const Analysis::Function &analyzed() const { return this->m_analyzed; }

  /**
   * Calls the function, using the data from \a state.  Upon return, the values
   * of \a state will have been updated.
   */
  Cpu::State::Reason call(Cpu::State &state);

private:
  Analysis::Function m_analyzed;
  MemoryManager &m_manager;
  void *m_funcPtr;
};
}

#endif // AMD64_FUNCTION_HPP
