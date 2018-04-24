#include <analysis/branch.hpp>
#include <dynarec/function.hpp>

namespace Dynarec {
Function::Function(const Analysis::Function &analyzed)
  : m_analyzed(analyzed), m_nativeAddress(nullptr)
{
}

Function::~Function() {
  if (this->m_finalizer) this->m_finalizer();
}

const Analysis::Function &Function::analyzed() const {
  return this->m_analyzed;
}

Analysis::Function &Function::analyzed() {
  return this->m_analyzed;
}

llvm::Module *Function::module() {
  return this->m_module.get();
}

void Function::setModule(std::unique_ptr<llvm::Module> &&module, llvm::Function *function) {
  this->m_compiled = function;
  this->m_module = std::move(module);
}

std::unique_ptr<llvm::Module> &&Function::stealModule() {
  return std::move(this->m_module);
}

llvm::Function *Function::compiledFunction() {
  return this->m_compiled;
}

void Function::setFinalizer(const std::function<void ()> &finalizer) {
  this->m_finalizer = finalizer;
}

void *Function::nativeAddress() const {
  return this->m_nativeAddress;
}

void Function::setNativeAddress(void *address) {
  this->m_nativeAddress = address;
}
}
