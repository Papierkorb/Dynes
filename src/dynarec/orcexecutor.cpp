#include <dynarec/orcexecutor.hpp>
#include <dynarec/function.hpp>
#include <dynarec/configuration.hpp>

#include <cpu/state.hpp>

#include <functional>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Mangler.h>

typedef llvm::orc::RTDyldObjectLinkingLayer ObjectLayer;
typedef llvm::orc::IRCompileLayer<ObjectLayer, llvm::orc::SimpleCompiler> CompileLayer;
typedef std::function<std::shared_ptr<llvm::Module>(std::shared_ptr<llvm::Module>)> OptimizeFunction;
typedef llvm::orc::IRTransformLayer<CompileLayer, OptimizeFunction> OptimizeLayer;
typedef OptimizeLayer::ModuleHandleT ModuleHandle;

typedef void(*NativeFunc)(Cpu::State *);

namespace Dynarec {
struct OrcExecutorPrivate {
  std::unique_ptr<llvm::TargetMachine> targetMachine;
  llvm::DataLayout dataLayout;
  ObjectLayer objectLayer;
  CompileLayer compileLayer;
  OptimizeLayer optimizeLayer;

  OrcExecutorPrivate()
    : targetMachine(llvm::EngineBuilder().selectTarget()),
      dataLayout(targetMachine->createDataLayout()),
      objectLayer([] { return std::make_shared<llvm::SectionMemoryManager>(); }),
      compileLayer(objectLayer, llvm::orc::SimpleCompiler(*targetMachine)),
      optimizeLayer(compileLayer, [this](std::shared_ptr<llvm::Module> mod){ return this->optimizeModule(mod); })
  {
  }

  ModuleHandle addModule(std::unique_ptr<llvm::Module> &&module) {
    // We don't do any symbol lookups from the generated code.  So we can get
    // away with not doing one.
    auto nullFinder = [](const std::string &) { return llvm::JITSymbol(nullptr); };
    auto resolver = llvm::orc::createLambdaResolver(nullFinder, nullFinder);

    //
    return llvm::cantFail(this->optimizeLayer.addModule(std::move(module), std::move(resolver)));
  }

  llvm::JITTargetAddress getSymbolAddress(const std::string &name) {
    llvm::JITSymbol symbol = this->optimizeLayer.findSymbol(name, false);
    return llvm::cantFail(symbol.getAddress());
  }

  void removeModule(ModuleHandle handle) {
    llvm::cantFail(this->optimizeLayer.removeModule(handle));
  }

  std::shared_ptr<llvm::Module> optimizeModule(std::shared_ptr<llvm::Module> module) {
    if (CONFIGURATION.optimize) {
      llvm::legacy::FunctionPassManager fpm(module.get());

      // Add optimization passes
      fpm.add(llvm::createInstructionCombiningPass());
      fpm.add(llvm::createReassociatePass());
      fpm.add(llvm::createGVNPass());
      fpm.add(llvm::createCFGSimplificationPass());
      fpm.doInitialization();

      // Run on all functions.  There should only be one in our case.
      for (auto &function : module->functions())
        fpm.run(function);
    }

    //
    return module;
  }
};

OrcExecutor::OrcExecutor() {
  // Bootstrap LLVM internals
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // Initialize late to let the internals initialize first
  this->d = new OrcExecutorPrivate;
}

OrcExecutor::~OrcExecutor() {
  delete this->d;
}

void OrcExecutor::callFunction(Function *function, Cpu::State &state) {
  void *nativeAddress = function->nativeAddress();

  if (!nativeAddress) { // JIT the function if not already done.
    ModuleHandle handle = this->d->addModule(function->stealModule());

    // When the function repository disposes of the function, also get rid of
    // the LLVM module.
    function->setFinalizer([this, handle]{ this->d->removeModule(handle); });

    // Resolve address in host memory
    llvm::JITTargetAddress address = this->d->getSymbolAddress(function->analyzed().nativeName().toStdString());
    nativeAddress = reinterpret_cast<void *>(address);
    function->setNativeAddress(nativeAddress); // Remember
  }

  // Sanity check
  if (!nativeAddress) {
    throw std::runtime_error("Native function not found!");
  }

  // Call the native function.
  NativeFunc native = reinterpret_cast<NativeFunc>(nativeAddress);
  native(&state);
}
}
