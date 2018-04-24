#ifndef DYNAREC_FUNCTIONCOMPILER_HPP
#define DYNAREC_FUNCTIONCOMPILER_HPP

#include "compiler.hpp"
#include "common.hpp"
#include "functionframe.hpp"

namespace Analysis { class Branch; }

namespace Dynarec {

struct FunctionCompilerImpl;

/**
 * Compiler for single \c Function's, driven by a \c Compiler.
 */
class FunctionCompiler {
public:
  FunctionCompiler(Compiler &compiler, llvm::Module *mod);
  ~FunctionCompiler();

  /**
   * Compiles \a function into the previously set LLVM module, and returns the
   * LLVM function.
   */
  llvm::Function *compile(Function *function);

  /** The main compiler. */
  Compiler &compiler();

  /** Basic blocks of the currently compiled function. */
  BlockMap &blocks();

  /** Frame of the currently compiled function. */
  FunctionFrame &frame();

  /** Compiles the given \a branch. */
  llvm::BasicBlock *compileBranch(Analysis::Branch *branch);

private:

  FunctionCompilerImpl *impl;
};
}

#endif // DYNAREC_FUNCTIONCOMPILER_HPP
