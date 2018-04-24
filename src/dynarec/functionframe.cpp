#include <dynarec/functionframe.hpp>
#include <dynarec/structtranslator.hpp>

namespace Dynarec {
FunctionFrame::FunctionFrame(Builder &b) {
  llvm::LLVMContext &ctx = b.getContext();

  this->a = b.CreateAlloca(llvm::Type::getInt8Ty(ctx), nullptr, "A");
  this->x = b.CreateAlloca(llvm::Type::getInt8Ty(ctx), nullptr, "X");
  this->y = b.CreateAlloca(llvm::Type::getInt8Ty(ctx), nullptr, "Y");
  this->s = b.CreateAlloca(llvm::Type::getInt8Ty(ctx), nullptr, "S");
  this->p = b.CreateAlloca(llvm::Type::getInt8Ty(ctx), nullptr, "P");
  this->cycles = b.CreateAlloca(llvm::Type::getInt32Ty(ctx), nullptr, "Cycles");

}

void FunctionFrame::initialize(Builder &b, llvm::Value *stateValue) {
  StructTranslator translator;
  std::vector<llvm::Value *> fields{ a, x, y, s, p, cycles };
  translator.copyTo(b, stateValue, fields);
}

void FunctionFrame::finalize(Builder &b, llvm::Value *stateValue) {
  StructTranslator translator;
  std::vector<llvm::Value *> fields{ a, x, y, s, p, cycles };
  translator.copyFrom(b, stateValue, fields,
                      [&b](llvm::Value *ptr){ return b.CreateLoad(ptr); });
}
}
