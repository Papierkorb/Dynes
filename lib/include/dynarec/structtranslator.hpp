#ifndef DYNAREC_STRUCTTRANSLATOR_HPP
#define DYNAREC_STRUCTTRANSLATOR_HPP

#include "common.hpp"

namespace Dynarec {
/** Helper for interacting with structures. */
class StructTranslator {
public:
  /**
   * Adds code into \a b, which resolves all values from the structure type in
   * a value.  For each value \a func is called, with the index as second
   * argument.
   */
  static void resolveAll(Builder &b, llvm::Value *value, std::function<bool (llvm::Value *, unsigned int)> func);

  /** Copies the first N values from \a value into \a vars. */
  static void copyTo(Builder &b, llvm::Value *value, std::vector<llvm::Value *> &vars);

  /** Copies the first N values from \a vars into \a value. */
  static void copyFrom(Builder &b, llvm::Value *value, std::vector<llvm::Value *> &vars);

  /** Copies the first N values from \a vars into \a value. */
  static void copyFrom(Builder &b, llvm::Value *value, std::vector<llvm::Value *> &vars,
                       std::function<llvm::Value *(llvm::Value *)> loader);
};
}

#endif // DYNAREC_STRUCTTRANSLATOR_HPP
