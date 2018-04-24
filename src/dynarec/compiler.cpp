#include <dynarec/compiler.hpp>
#include <dynarec/common.hpp>
#include <dynarec/function.hpp>
#include <dynarec/functioncompiler.hpp>
#include <dynarec/configuration.hpp>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Verifier.h>

#include <QMap>
#include <tuple>

namespace Dynarec {
struct CompilerPrivate {
  llvm::LLVMContext ctx;
  QMap<QString, llvm::Value *> variables;
  QMap<QString, llvm::Value *> functions;

  llvm::StructType *stateType = nullptr; // struct State { ... };
};

Compiler::Compiler() : d(new CompilerPrivate) {

  // Build the LLVM representation of `struct Cpu::State`.
  // For this reason that struct is marked as packed.
  llvm::ArrayRef<llvm::Type *> elements{
    llvm::Type::getInt8Ty(this->d->ctx), // u8, A
    llvm::Type::getInt8Ty(this->d->ctx), // u8, X
    llvm::Type::getInt8Ty(this->d->ctx), // u8, Y
    llvm::Type::getInt8Ty(this->d->ctx), // u8, S
    llvm::Type::getInt8Ty(this->d->ctx), // u8, P
    llvm::Type::getInt32Ty(this->d->ctx), // i32, Cycles
    llvm::Type::getInt16Ty(this->d->ctx), // u16, PC
    llvm::Type::getInt8Ty(this->d->ctx), // u8, State::Reason
  };

  this->d->stateType = llvm::StructType::create(this->d->ctx, elements, "CpuContext");
}

Compiler::~Compiler() {
  delete this->d;
}

llvm::StructType *Compiler::stateType() {
  return this->d->stateType;
}

llvm::LLVMContext &Compiler::context() {
  return this->d->ctx;
}

void Compiler::addVariable(const QString &name, void *pointer) {
  // Store as actual `void*` constant to ease usage later.
  llvm::ConstantInt *address = const64(this->d->ctx, reinterpret_cast<uint64_t>(pointer));
  llvm::Type *voidPtrTy = llvm::PointerType::get(llvm::Type::getVoidTy(this->d->ctx), 0);
  llvm::Value *value = llvm::ConstantExpr::getIntToPtr(address, voidPtrTy);
  this->d->variables.insert(name, value);
}

void Compiler::addVariable(const QString &name, llvm::Value *value) {
  this->d->variables.insert(name, value);
}

void Compiler::addFunction(const QString &name, void *pointer, llvm::FunctionType *prototype) {
  llvm::Constant *address = const64(this->d->ctx, reinterpret_cast<uint64_t>(pointer));
  llvm::PointerType *protoPtrTy = llvm::PointerType::get(prototype, 0);
  llvm::Value *value = llvm::ConstantExpr::getIntToPtr(address, protoPtrTy);

  this->d->functions.insert(name, value);
}

llvm::Value *Compiler::builtin(const QString &name) {
  llvm::Value *pointer = this->d->functions.value(name);
  if (!pointer) throw std::runtime_error("Unknown builtin");
  return pointer;
}

llvm::Value *Compiler::global(Builder &b, const QString &name, llvm::Type *type) {
  llvm::Value *value = this->d->variables.value(name);

  if (!value) throw std::runtime_error("Unknown global");

  if (type)
    return b.CreatePointerCast(value, type);
  return value;
}

llvm::Function *Compiler::compile(Function *function) {
  QString name = function->analyzed().nativeName();
  std::unique_ptr<llvm::Module> mod(std::make_unique<llvm::Module>(name.toStdString(), this->d->ctx));

  FunctionCompiler compiler(*this, mod.get());
  llvm::Function *func = compiler.compile(function);

  llvm::raw_ostream &errorStream = llvm::errs();

  // Verify module.  If it's no good, always dump the module.
  bool verifyError = llvm::verifyModule(*mod, &errorStream);
  bool dump = verifyError || CONFIGURATION.dump;

  if (dump) mod->print(errorStream, nullptr);
  if (verifyError)
    throw std::runtime_error("Broken code generation");

  function->setModule(std::move(mod), func);

  return func;
}

}
