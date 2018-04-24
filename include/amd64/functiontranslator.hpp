#ifndef AMD64_FUNCTIONTRANSLATOR_HPP
#define AMD64_FUNCTIONTRANSLATOR_HPP

#include "assembler.hpp"
#include "core_amd64.hpp"

#include <analysis/branch.hpp>

namespace Amd64 {
class SymbolRegistry;
class MemoryManager;

/**
 * Translator to go from (multiple) branches of a 6502 functions to an
 * assembled AMD64 function.
 */
class FunctionTranslator {
public:
  FunctionTranslator();

  /** Adds \a branch to the function. */
  void addBranch(const Analysis::Branch &branch);

  /**
   * Finalizes the translation of this function.  Upon calling, the function
   * will jump to the (translated) instruction at the \a entry address.
   *
   * @param entry The address of the 6502 entry instruction
   * @param symbols The symbol registry for (function-) global state
   * @param memory Where to link the function into
   * @return A \c CALL-able function pointer
   */
  void *link(uint16_t entry, SymbolRegistry &symbols, MemoryManager &memory);

private:
  Assembler m_asm;
  std::map<uint16_t, Section &> m_sections;
};
}

#endif // AMD64_FUNCTIONTRANSLATOR_HPP
