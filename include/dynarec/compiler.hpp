#ifndef DYNAREC_COMPILER_HPP
#define DYNAREC_COMPILER_HPP

#include <QString>
#include "common.hpp"

namespace llvm {
class Value;
class StructType;
class FunctionType;
}

namespace Dynarec {
struct CompilerPrivate;
class Function;

/**
 * The compiler class holds CPU-global context data.
 */
class Compiler {
public:
  Compiler();
  ~Compiler();

  /** */
  llvm::StructType *stateType();

  /** The LLVM context. */
  llvm::LLVMContext &context();

  /** Adds \a pointer as global variable \a name. */
  void addVariable(const QString &name, void *pointer);

  /** Adds \a value as global variable \a name. */
  void addVariable(const QString &name, llvm::Value *value);

  /** Adds the function at \a pointer of \a prototype as \a name. */
  void addFunction(const QString &name, void *pointer, llvm::FunctionType *prototype);

  /** Finds the built-in function \a name. */
  llvm::Value *builtin(const QString &name);

  /** Finds the global variable \a name, and casts it to \a type. */
  llvm::Value *global(Builder &b, const QString &name, llvm::Type *type = nullptr);

  /** JITs the \a function. */
  llvm::Function *compile(Function *function);
private:
  CompilerPrivate *d;
};
}

#endif // DYNAREC_COMPILER_HPP
