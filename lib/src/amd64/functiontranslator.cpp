#include <amd64/functiontranslator.hpp>
#include <amd64/instructiontranslator.hpp>
#include <amd64/linker.hpp>

/************************* DEBUG FUNCTIONALITY FLAGS **************************/

// If set, will use the `objdump` tool to disassemble the generated code.
// Used to verify the assemblers functionality.
static constexpr bool DUMP_DISASSEMBLY = false;

/*************************                           **************************/


namespace Amd64 {
FunctionTranslator::FunctionTranslator() {
}

static std::string instructionSectionName(uint16_t address) {
  return "instr_" + std::to_string(address);
}

void FunctionTranslator::addBranch(const Analysis::Branch &branch) {
  for (const Analysis::Branch::Element &el : branch.elements()) {
    uint16_t address = el.first;
    Analysis::Branch::Instruction instr = el.second;

    if (this->m_sections.find(address) == this->m_sections.end()) {
      Section &section = this->m_asm.section(instructionSectionName(address));
      InstructionTranslator t(section);

      this->m_sections.insert({ address, section });
      auto jump = t.translate(address, instr);

      if (jump.first) { // Need to add a JMP?
        section.emitJmp(instructionSectionName(jump.second));
      }
    }
  }
}

void *FunctionTranslator::link(uint16_t entry, SymbolRegistry &symbols, MemoryManager &memory) {
  Linker linker(instructionSectionName(entry), symbols, memory);
  linker.add(this->m_asm);
  return linker.link(DUMP_DISASSEMBLY);
}

}
