#ifndef DYNAREC_FUNCTIONFRAME_HPP
#define DYNAREC_FUNCTIONFRAME_HPP

#include "common.hpp"

namespace llvm { class Value; }

namespace Dynarec {
/** Function frame variables in the guest code. */
struct FunctionFrame {
  llvm::Value *a;
  llvm::Value *x;
  llvm::Value *y;
  llvm::Value *s;
  llvm::Value *p;
  llvm::Value *cycles;

  FunctionFrame(Builder &b);

  /**
   * Initializes the values to values from \a structValue.
   */
  void initialize(Builder &b, llvm::Value *stateValue);

  /**
   * Copies values of this frame back.
   */
  void finalize(Builder &b, llvm::Value *stateValue);
};
}

#endif // DYNAREC_FUNCTIONFRAME_HPP
