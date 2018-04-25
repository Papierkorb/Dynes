#ifndef AMD64_LINKER_HPP
#define AMD64_LINKER_HPP

#include "assembler.hpp"

namespace Amd64 {
class SymbolRegistry;
class MemoryManager;

/**
 * Linker for output of an \c Assembler.  Combines the different sections of an
 * assembler into a coherent stream of bytes, replacing bytes where necessary.
 *
 * A single \c Linker instance produces a \b single function.
 * It can't be used multiple times.
 */
class Linker {
public:
  Linker(const std::string &entryPoint, SymbolRegistry &registry, MemoryManager &memory);

  /** Adds the \a section called \a name to the linker. */
  void add(const std::string &name, const Section &section);

  /** Adds all sections of the \a assembler to the linker. */
  void add(const Assembler &assembler);

  /**
   * Links the sections and symbols into the memory manager.
   *
   * If \a dumpDisassembly is \c true, will send the generated code through
   * `objdump(1)` to show a nice disassembly.
   */
  void *link(bool dumpDisassembly = false);

private:
  std::string m_entryPoint;
  SymbolRegistry &m_registry;
  MemoryManager &m_memory;
  std::map<std::string, const Section&> m_sections;

  std::pair<Section, std::map<std::string, uintptr_t>> mergeSections();
};
}

#endif // AMD64_LINKER_HPP
