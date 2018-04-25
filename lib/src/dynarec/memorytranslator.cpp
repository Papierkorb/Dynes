#include <dynarec/functioncompiler.hpp>
#include <dynarec/memorytranslator.hpp>

#include <cpu.hpp>

namespace Dynarec {
MemoryTranslator::MemoryTranslator(FunctionCompiler &funcComp)
  : m_funcComp(funcComp)
{

}

static bool useFastPath(Core::Instruction::Addressing mode) {
  using Core::Instruction;

  switch (mode) {
  // Stack and Zero-Page accesses are guaranteed to end up in RAM.
  case Instruction::S:
  case Instruction::Zp:
  case Instruction::ZpX:
  case Instruction::ZpY:
    return true;
  default:
    return false;
  }
}

llvm::Value *MemoryTranslator::readRam(Builder &b, llvm::Value *absoluteAddress) {
  return b.CreateLoad(this->ramPointer(b, absoluteAddress), "RamValue");
}

llvm::Value *MemoryTranslator::read(Builder &b, Core::Instruction::Addressing mode, llvm::Value *address) {
  llvm::Value *resolved = this->resolve(b, mode, address);

  if (useFastPath(mode)) {
    return this->readRam(b, resolved);
  } else {
    llvm::Value *memory = this->m_funcComp.compiler().global(b, "memory");
    llvm::Value *reader = this->m_funcComp.compiler().builtin("mem.read");
    return b.CreateCall(reader, { memory, resolved });
  }
}

void MemoryTranslator::writeRam(Builder &b, llvm::Value *absoluteAddress, llvm::Value *value) {
  b.CreateStore(value, this->ramPointer(b, absoluteAddress));
}

llvm::Value *MemoryTranslator::ramPointer(Builder &b, llvm::Value *offset) {
  llvm::Value *ram = this->m_funcComp.compiler().global(b, "ram", b.getInt8PtrTy());
  return b.CreateGEP(ram, offset, "DirectRamPtr");
}

llvm::Value *MemoryTranslator::stackPointer(Builder &b, llvm::Value *offset) {
  llvm::Value *offset16 = b.CreateZExt(offset, b.getInt16Ty(), "Offset16Bit");
  llvm::Value *displaced = b.CreateOr(offset16, Cpu::STACK_BASE, "StackOffset");
  return this->ramPointer(b, displaced);
}

void MemoryTranslator::write(Builder &b, Core::Instruction::Addressing mode, llvm::Value *address,
                             llvm::Value *value) {
  llvm::Value *resolved = this->resolve(b, mode, address);

  if (useFastPath(mode)) {
    this->writeRam(b, resolved, value);
  } else {
    llvm::Value *memory = this->m_funcComp.compiler().global(b, "memory");
    llvm::Value *writer = this->m_funcComp.compiler().builtin("mem.write");
    b.CreateCall(writer, { memory, resolved, value });
  }
}

void MemoryTranslator::rmw(Builder &b, Core::Instruction::Addressing mode, llvm::Value *address,
                           std::function<llvm::Value *(llvm::Value *)> proc) {
  llvm::Value *resolved = this->resolve(b, mode, address);

  if (useFastPath(mode)) {
    llvm::Value *ram = this->ramPointer(b, resolved);
    llvm::Value *value = b.CreateLoad(ram);
    llvm::Value *result = proc(value);
    b.CreateStore(result, ram);
  } else {
    llvm::Value *memory = this->m_funcComp.compiler().global(b, "memory");
    llvm::Value *reader = this->m_funcComp.compiler().builtin("mem.read");
    llvm::Value *writer = this->m_funcComp.compiler().builtin("mem.write");
    llvm::Value *value = b.CreateCall(reader, { memory, resolved });

    llvm::Value *result = proc(value);
    b.CreateCall(writer, { memory, resolved, result });
  }
}

static llvm::Value *resolveAbs(Builder &b, llvm::Value *base, llvm::Value *offset) {
  llvm::Value *offset8 = b.CreateLoad(offset);
  llvm::Value *offset16 = b.CreateZExt(offset8, b.getInt16Ty(), "MemOffset");
  return b.CreateAdd(base, offset16, "MemAddr");
}

static llvm::Value *resolveIndX(MemoryTranslator *t, FunctionFrame &frame, Builder &b, llvm::Value *address) {
  llvm::Value *x8 = b.CreateLoad(frame.x, "X");
  llvm::Value *x16 = b.CreateZExt(x8, b.getInt16Ty(), "X16Bit");
  llvm::Value *base = b.CreateAnd(b.CreateAdd(address, x16), 0x00FF, "Address+X");

  return t->read16(b, base);
}

static llvm::Value *resolveIndY(MemoryTranslator *t, FunctionFrame &frame, Builder &b, llvm::Value *address) {
  llvm::Value *resolved = t->read16(b, address);

  llvm::Value *y8 = b.CreateLoad(frame.y, "Y");
  llvm::Value *y16 = b.CreateZExt(y8, b.getInt16Ty(), "Y16Bit");
  return b.CreateAdd(resolved, y16);
}

llvm::Value *MemoryTranslator::resolve(Builder &b, Core::Instruction::Addressing mode, llvm::Value *address) {
  using Core::Instruction;

  FunctionFrame &frame = this->m_funcComp.frame();

  switch (mode) {
  case Instruction::Zp:
    return address;
  case Instruction::ZpX:
    return b.CreateAnd(resolveAbs(b, address, frame.x), 0x00FF, "MemAddrZpX");
  case Instruction::ZpY:
    return b.CreateAnd(resolveAbs(b, address, frame.y), 0x00FF, "MemAddrZpY");
  case Instruction::S:
    return b.CreateOr(address, Cpu::STACK_BASE, "StackAddr");
  case Instruction::Abs:
    return address;
  case Instruction::AbsX:
    return resolveAbs(b, address, frame.x);
  case Instruction::AbsY:
    return resolveAbs(b, address, frame.y);
  case Instruction::Ind:
    return this->read16(b, address);
  case Instruction::IndX:
    return resolveIndX(this, frame, b, address);
  case Instruction::IndY:
    return resolveIndY(this, frame, b, address);

  default:
    throw std::runtime_error("Non-memory addressing mode");
  }
}

llvm::Value *MemoryTranslator::read16(Builder &b, llvm::Value *address) {
  llvm::Value *memory = this->m_funcComp.compiler().global(b, "memory");
  llvm::Value *reader = this->m_funcComp.compiler().builtin("mem.read16");
  return b.CreateCall(reader, { memory, address });
}
}
