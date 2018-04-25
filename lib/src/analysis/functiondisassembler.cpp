#include <analysis/branch.hpp>
#include <analysis/conditionalinstruction.hpp>
#include <analysis/function.hpp>
#include <analysis/functiondisassembler.hpp>

#include <core/data.hpp>
#include <core/disassembler.hpp>

namespace Analysis {
struct FunctionDisassemblerImpl {
  Core::Data::Ptr data;

  FunctionDisassemblerImpl(const Core::Data::Ptr &d) : data(d) { }

  Branch *getOrBuildBranch(Function &f, uint16_t address) {
    Branch *br = f.branch(address);

    if (!br) {
      br = new Branch(address);

      // To avoid recursive loops, add the branch now, and fill it later.
      f.add(br);
      this->buildBranch(f, br, address);
    }

    return br;
  }

#define TRACE(...)
//#define TRACE(...) fprintf(stderr, ";"); for (int i = 0; i < depth; i++) fprintf(stderr, "  "); fprintf(stderr, __VA_ARGS__);

  void buildBranch(Function &f, Branch *br, uint16_t address) {
    Core::Disassembler disasm(this->data, static_cast<int>(static_cast<uint32_t>(address)));
    Core::Instruction instr(Core::Instruction::Unknown, Core::Instruction::Imp, 0);
    Branch::List &elements = br->elements();

#ifdef TRACE
    static int depth = 0;
    depth++;

    TRACE("-> Branch at %04x\n", address)
#endif

    do {
      uint16_t addr = static_cast<uint16_t>(disasm.position());
      instr = disasm.next();
      TRACE(" %04x %s\n", addr, instr.commandName())

      // Discover sub branches for conditionally branching instructions.
      // Explore both the true and false branches then.
      if (instr.isConditionalBranching()) {
        // Start address of the next instruction.
        uint16_t nextAddr = static_cast<uint16_t>(disasm.position());
        Branch *falsy = this->getOrBuildBranch(f, nextAddr);
        Branch *truthy = this->getOrBuildBranch(f, instr.destinationAddress(nextAddr));

        ConditionalInstruction cond(instr, truthy, falsy);
        elements.push_back({ addr, cond });
      } else {
        // This is not a conditional instruction.
        elements.push_back({ addr, instr });
      }

      // Break once we hit any branching instruction.
    } while(!instr.isBranching());

#ifdef TRACE
    TRACE("<-- Branch end\n")
    depth--;
#endif
  }

#undef TRACE
};

FunctionDisassembler::FunctionDisassembler(Core::Data::Ptr &data)
  : impl(new FunctionDisassemblerImpl(data))
{

}

FunctionDisassembler::~FunctionDisassembler() {
  delete this->impl;
}

static bool isAddressCacheable(uint16_t address) {
  // Accesses to the cartridge are usually cacheable.
  // Others would need tracking of RAM changes, which would be quite expensive.
  return (address >= 0x4018);
}

Function FunctionDisassembler::disassemble(uint16_t address) {
  Function func(this->impl->data->tag(), address, isAddressCacheable(address));

  // Discover branches going from the start address of the function.
  this->impl->getOrBuildBranch(func, address);
  return func;
}
}
