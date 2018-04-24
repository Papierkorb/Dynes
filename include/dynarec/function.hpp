#ifndef DYNAREC_FUNCTION_HPP
#define DYNAREC_FUNCTION_HPP

#include <analysis/function.hpp>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/IR/Module.h>
#include <functional>

namespace Dynarec {
/**
 * Container for data of an analyzed and compiled function.
 */
class Function {
public:
  Function(const Analysis::Function &analyzed);
  ~Function();

  /** The inner function. */
  const Analysis::Function &analyzed() const;

  /** The inner function. */
  Analysis::Function &analyzed();

  /** The module, if this function was already JITed. */
  llvm::Module *module();

  /** Sets the compiled module. */
  void setModule(std::unique_ptr<llvm::Module> &&module, llvm::Function *function);

  /** Steals the module. */
  std::unique_ptr<llvm::Module> &&stealModule();

  /** If this function was already JITed, the function. */
  llvm::Function *compiledFunction();

  void setFinalizer(const std::function<void ()> &finalizer);

  void *nativeAddress() const;
  void setNativeAddress(void *address);

private:
  Analysis::Function m_analyzed;
  std::unique_ptr<llvm::Module> m_module;
  llvm::Function *m_compiled = nullptr;
  std::function<void()> m_finalizer;
  void *m_nativeAddress;
};
}

#endif // DYNAREC_FUNCTION_HPP
