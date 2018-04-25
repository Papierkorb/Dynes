#ifndef DYNAREC_ORCEXECUTOR_HPP
#define DYNAREC_ORCEXECUTOR_HPP

namespace Cpu { class State; }

namespace Dynarec {
struct OrcExecutorPrivate;
class Function;

/**
 * Executor for \c Function's using LLVM ORC JIT.
 */
class OrcExecutor {
public:
  OrcExecutor();
  ~OrcExecutor();

  void callFunction(Function *function, Cpu::State &state);

private:
  OrcExecutorPrivate *d;
};
}

#endif // DYNAREC_ORCEXECUTOR_HPP
