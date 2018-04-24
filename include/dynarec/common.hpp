#ifndef DYNAREC_COMMON_HPP
#define DYNAREC_COMMON_HPP

#include <QMap>
#include <cstdint>
#include <functional>

#include <llvm/IR/IRBuilder.h>

namespace Dynarec {
struct InstructionBlock {
  llvm::BasicBlock *in = nullptr;
  llvm::BasicBlock *out = nullptr;
};

typedef llvm::IRBuilder<> Builder;
typedef QMap<uint16_t, InstructionBlock> BlockMap;

/** Returns a constant 32-Bit integer. */
llvm::ConstantInt *const32(llvm::LLVMContext &ctx, uint32_t value);

/** Returns a constant 64-Bit integer. */
llvm::ConstantInt *const64(llvm::LLVMContext &ctx, uint64_t value);

}

#endif // DYNAREC_COMMON_HPP
