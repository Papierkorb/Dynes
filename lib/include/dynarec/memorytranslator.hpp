#ifndef DYNAREC_MEMORYTRANSLATOR_HPP
#define DYNAREC_MEMORYTRANSLATOR_HPP

#include "common.hpp"
#include <core/instruction.hpp>

namespace Dynarec {
class FunctionCompiler;

/**
 * Translates simulated memory access to LLVM IR.
 */
class MemoryTranslator {
public:
  MemoryTranslator(FunctionCompiler &funcComp);

  /** Reads directly from RAM. */
  llvm::Value *readRam(Builder &b, llvm::Value *absoluteAddress);

  /** Reads a value at \a address via \a mode. */
  llvm::Value *read(Builder &b, Core::Instruction::Addressing mode, llvm::Value *address);

  /** Writes directly into RAM. */
  void writeRam(Builder &b, llvm::Value *absoluteAddress, llvm::Value *value);

  /** Run-time pointer into the RAM at \a offset. */
  llvm::Value *ramPointer(Builder &b, llvm::Value *offset);

  /** Run-time pointer into the RAM at the stack with an 8-bit \a offset. */
  llvm::Value *stackPointer(Builder &b, llvm::Value *offset);

  /** Writes \a value at \a address via \a mode. */
  void write(Builder &b, Core::Instruction::Addressing mode, llvm::Value *address, llvm::Value *value);

  /**
   * Reads the value at \a address, calls \a proc with it, and stores the result
   * of it into the same place afterwards.
   */
  void rmw(Builder &b, Core::Instruction::Addressing mode, llvm::Value *address,
           std::function<llvm::Value *(llvm::Value*)> proc);

  /**
   * Resolves the \a address via \a mode into an absolute memory address.
   */
  llvm::Value *resolve(Builder &b, Core::Instruction::Addressing mode, llvm::Value *address);

  llvm::Value *read16(Builder &b, llvm::Value *address);

private:
  FunctionCompiler &m_funcComp;
};
}

#endif // DYNAREC_MEMORYTRANSLATOR_HPP
