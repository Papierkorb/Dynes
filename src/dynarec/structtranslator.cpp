#include <dynarec/structtranslator.hpp>

#include <llvm/IR/DerivedTypes.h>

namespace Dynarec {
void StructTranslator::resolveAll(Builder &b, llvm::Value *value, std::function<bool (llvm::Value *, unsigned int)> func) {
  llvm::Type *structType = value->getType();

  if (structType->isPointerTy()) // Auto resolve pointers
    structType = structType->getPointerElementType();

  llvm::LLVMContext &ctx = structType->getContext();
  llvm::Value *structPtr = b.CreateBitOrPointerCast(value, b.getInt8PtrTy());
  int offset = 0;

  for (unsigned int i = 0; i < structType->getStructNumElements(); i++) {
    llvm::Type *type = structType->getStructElementType(i);
    llvm::PointerType *typePtr = llvm::PointerType::get(type, 0);
    llvm::Value *untypedPtr = b.CreateGEP(structPtr, const32(ctx, offset));
    llvm::Value *typedPtr = b.CreateBitOrPointerCast(untypedPtr, typePtr);

    if (!func(typedPtr, i)) break;

    llvm::IntegerType *intType = llvm::cast<llvm::IntegerType>(type);
    offset += intType->getIntegerBitWidth() / 8;
  }
}

void StructTranslator::copyTo(Builder &b, llvm::Value *value, std::vector<llvm::Value *> &vars) {
  resolveAll(b, value, [&vars, &b](llvm::Value *ptr, unsigned int idx){
    if (idx >= vars.size()) return false;

    if (llvm::Value *destination = vars.at(idx)) {
      b.CreateStore(b.CreateLoad(ptr), destination);
    }

    return true;
  });
}

void StructTranslator::copyFrom(Builder &b, llvm::Value *value, std::vector<llvm::Value *> &vars) {
  copyFrom(b, value, vars, [](llvm::Value *v){ return v; });
}

void StructTranslator::copyFrom(Builder &b, llvm::Value *value, std::vector<llvm::Value *> &vars,
                                std::function<llvm::Value *(llvm::Value *)> loader) {
  resolveAll(b, value, [&](llvm::Value *ptr, unsigned int idx){
    if (idx >= vars.size()) return false;

    if (llvm::Value *source = vars.at(idx)) {
      b.CreateStore(loader(source), ptr);
    }

    return true;
  });
}
}
