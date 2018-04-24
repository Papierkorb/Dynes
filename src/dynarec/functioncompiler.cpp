#include <dynarec/function.hpp>
#include <dynarec/functioncompiler.hpp>
#include <dynarec/functionframe.hpp>
#include <dynarec/common.hpp>
#include <analysis/branch.hpp>
#include <dynarec/instructiontranslator.hpp>

#include <llvm/IR/Verifier.h>

namespace Dynarec {

struct FunctionCompilerImpl {
  FunctionCompiler *parent;
  Compiler &compiler;
  llvm::Module *module;
  BlockMap blocks;
  FunctionFrame *frame = nullptr;
  llvm::Function *function;

  FunctionCompilerImpl(FunctionCompiler *p, Compiler &c, llvm::Module *m)
    : parent(p), compiler(c), module(m)
  {
  }

  ~FunctionCompilerImpl() {
    delete this->frame;
  }

  llvm::Function *buildLlvmFunction(llvm::LLVMContext &ctx, Function *function) {
    llvm::ArrayRef<llvm::Type *> args{ llvm::PointerType::get(this->compiler.stateType(), 0) };
    llvm::FunctionType *prototype = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), args, false);

    std::string name = function->analyzed().nativeName().toStdString();

    return llvm::Function::Create(prototype, llvm::Function::ExternalLinkage, name, this->module);
  }

  bool isBranching(Analysis::Branch::Instruction &instr) {
    if (const Core::Instruction *ptr = std::get_if<Core::Instruction>(&instr)) {
      return ptr->isBranching();
    } else {
      // In this case it's a ConditionalInstruction, which is always branching.
      return true;
    }
  }

  llvm::BasicBlock *compileBranch(Analysis::Branch *branch) {
    llvm::BasicBlock *start = nullptr;
    llvm::BasicBlock *previous = nullptr;
    Builder b(this->module->getContext());

    for(const Analysis::Branch::Element &el : branch->elements()) {
      uint16_t address = el.first;
      InstructionBlock instrBlock = this->blocks.value(address);
      Analysis::Branch::Instruction instr = el.second;

      if (!instrBlock.out) { // Lazy compile
        instrBlock = this->compileInstruction(address, instr);
      }

      // Branch from previous instruction to here, but only if the last
      // instruction in that basic block isn't already a branching instruction!
      if (previous && !previous->back().isTerminator()) {
        b.SetInsertPoint(previous);
        b.CreateBr(instrBlock.in);
      }

      if (!start) start = instrBlock.in;
      previous = instrBlock.out;
    }

    return start;
  }

  QString instructionBranchName(uint16_t addr, Analysis::Branch::Instruction &instr) {
    const char *command = "?";
    const char *mode = "?";

    if (const Core::Instruction *ptr = std::get_if<Core::Instruction>(&instr)) {
      command = ptr->commandName();
      mode = ptr->addressingName();
    } else {
      Analysis::ConditionalInstruction cond = std::get<Analysis::ConditionalInstruction>(instr);
      command = cond.commandName();
      mode = cond.addressingName();
    }

    return QStringLiteral("instr_%1_%2_%3")
        .arg(addr, 4, 16, QLatin1Char('0'))
        .arg(QLatin1String(command))
        .arg(QLatin1String(mode));
  }

  InstructionBlock compileInstruction(uint16_t addr, Analysis::Branch::Instruction &instr) {
    QString branchName = this->instructionBranchName(addr, instr);
    InstructionBlock iblocks;

    // Create (at least) one branch per instruction:
    llvm::LLVMContext &ctx = this->module->getContext();

    iblocks.in = iblocks.out = llvm::BasicBlock::Create(ctx, branchName.toStdString(), this->function);

    // Store to avoid recursion
    this->blocks.insert(addr, iblocks);

    // Translate the instruction.
    Builder b(this->module->getContext());
    b.SetInsertPoint(iblocks.in);
    InstructionTranslator translator(*this->parent, this->function);
    iblocks.out = translator.translate(b, addr, instr);

    this->blocks.insert(addr, iblocks);
    return iblocks;
  }

  llvm::Function *compile(Function *function) {
    this->blocks.clear();

    llvm::LLVMContext &ctx = this->module->getContext();
    this->function = this->buildLlvmFunction(ctx, function);

    // Start building
    Builder builder(ctx);
    llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(ctx, "entry", this->function);
    builder.SetInsertPoint(entryBlock);

    // Build register-variables frame
    this->frame = new FunctionFrame(builder);
    this->frame->initialize(builder, this->function->arg_begin());

    // Compile branches
    QMap<uint16_t, Analysis::Branch *> funcBranches = function->analyzed().branches();
    for (Analysis::Branch *branch : funcBranches) {
      this->compileBranch(branch);
    }

    // Branch from the "entry" block into the first instruction block.
    builder.CreateBr(this->blocks.value(function->analyzed().begin()).in);
    return this->function; // Done!
  }

};

FunctionCompiler::FunctionCompiler(Compiler &compiler, llvm::Module *mod)
  : impl(new FunctionCompilerImpl(this, compiler, mod))
{
}

FunctionCompiler::~FunctionCompiler() {
  delete this->impl;
}

llvm::Function *FunctionCompiler::compile(Function *function) {
  return this->impl->compile(function);
}

Compiler &FunctionCompiler::compiler() {
  return this->impl->compiler;
}

BlockMap &FunctionCompiler::blocks() {
  return this->impl->blocks;
}

FunctionFrame &FunctionCompiler::frame() {
  return *this->impl->frame;
}

llvm::BasicBlock *FunctionCompiler::compileBranch(Analysis::Branch *branch) {
  return this->impl->compileBranch(branch);
}

}
