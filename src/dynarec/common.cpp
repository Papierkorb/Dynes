#include <dynarec/common.hpp>

llvm::ConstantInt *Dynarec::const32(llvm::LLVMContext &ctx, uint32_t value) {
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), value, false);
}

llvm::ConstantInt *Dynarec::const64(llvm::LLVMContext &ctx, uint64_t value) {
  return llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), value, false);
}
