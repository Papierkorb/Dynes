#ifndef DYNAREC_INSTRUCTIONTRANSLATOR_HPP
#define DYNAREC_INSTRUCTIONTRANSLATOR_HPP

#include "common.hpp"
#include <analysis/branch.hpp>

namespace Dynarec {
class FunctionCompiler;

/**
 * Translator for instructions.  This class holds the main translation logic.
 */
class InstructionTranslator {
public:
  InstructionTranslator(FunctionCompiler &compiler, llvm::Function *func);

  /** Translates the instruction. */
  llvm::BasicBlock *translate(Builder &b, uint16_t address, const Analysis::Branch::Instruction &instr);
private:
  FunctionCompiler &m_compiler;
  llvm::Function *m_func;
};
}

#endif // DYNAREC_INSTRUCTIONTRANSLATOR_HPP
