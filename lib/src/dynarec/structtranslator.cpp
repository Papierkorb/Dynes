#include <dynarec/structtranslator.hpp>

#include <llvm/IR/DerivedTypes.h>

#include <llvm/Support/raw_ostream.h>
#include <iostream>

namespace Dynarec {
static llvm::StructType *unpackStructType(llvm::Type *type) {
  if (type->isPointerTy()) // Auto resolve pointers
    return llvm::cast<llvm::StructType>(type->getPointerElementType());
  return llvm::cast<llvm::StructType>(type);
}

void StructTranslator::resolveAll(Builder &b, llvm::Value *value, std::function<bool(llvm::Value *, unsigned int)> func) {
  llvm::StructType *structType = unpackStructType(value->getType());
  llvm::Value *structPtr = b.CreateBitOrPointerCast(value, b.getInt8PtrTy());
  uint32_t offset = 0;
  uint32_t idx = 0;

  for (llvm::Type *type : structType->elements()) {
    llvm::PointerType *typePtr = type->getPointerTo();
    llvm::Value *untypedPtr = b.CreateGEP(structPtr, b.getInt32(offset));
    llvm::Value *typedPtr = b.CreateBitOrPointerCast(untypedPtr, typePtr);

    if (!func(typedPtr, idx)) break;

    offset += type->getPrimitiveSizeInBits() / 8;
    idx++;
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
