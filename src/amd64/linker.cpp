#include <amd64/linker.hpp>
#include <amd64/symbolregistry.hpp>
#include <amd64/memorymanager.hpp>

#include <limits>
#include <cstdio>
#include <sys/wait.h>

namespace Amd64 {
Linker::Linker(const std::string &entryPoint, SymbolRegistry &registry, MemoryManager &memory)
  : m_entryPoint(entryPoint), m_registry(registry), m_memory(memory)
{

}

void Linker::add(const std::string &name, const Section &section) {
  this->m_sections.insert({ name, section });
}

void Linker::add(const Assembler &assembler) {
  const auto &sections = assembler.sections();

  for (const auto &kv : sections) {
    this->m_sections.insert(kv);
  }
}

template<typename T>
static void replaceBytes(uint8_t *ptr, uint64_t value) {
  union { T v; uint8_t bytes[sizeof(T)]; } cast { static_cast<T>(value) };
  ::memcpy(ptr, cast.bytes, sizeof(T));
}

static void replaceBytes(uint8_t *ptr, size_t size, uint64_t value) {
  switch (size) {
  case sizeof(uint64_t):
    replaceBytes<uint64_t>(ptr, value);
    break;
  case sizeof(uint32_t):
    replaceBytes<uint32_t>(ptr, value);
    break;
  case sizeof(uint16_t):
    replaceBytes<uint16_t>(ptr, value);
    break;
  case sizeof(uint8_t):
    replaceBytes<uint8_t>(ptr, value);
    break;
  }
}

static void fixUpSectionReference(uint8_t *data, uintptr_t rip, uintptr_t destination, const Reference &ref) {
  uint8_t *ptr = data + ref.offset;

  uintptr_t relative = (ref.base > 0) ? destination - rip : uintptr_t(data);
  replaceBytes(ptr, ref.size, relative);
}

static void fixUpSymbolReference(uint8_t *data, uintptr_t rip, Symbol &symbol, const Reference &ref) {
  uint8_t *ptr = data + ref.offset;

  uint64_t value = symbol.value;
  if (!symbol.isPointer && ref.base > 0) {
    throw std::runtime_error("Symbol " + ref.name + " was referenced as pointer, but is not a pointer");
  }

  uintptr_t relative = (ref.base > 0) ? value - rip : value;
  replaceBytes(ptr, ref.size, relative);
}

/**
 * Debugging for the assembler/linker: Writes the generated code into a
 * temporary file and calls objdump(1) on it to produce a nicely readable
 * disassembly.  Enabled at compile-time by defining \c OBJDUMP
 */
static void debugDump(uint8_t *bytes, size_t length) {
  (void)bytes, (void)length;

  if (FILE *tmp = fopen("/tmp/amd64_jit.bin", "wb")) {
    fwrite(bytes, length, 1, tmp);
    fclose(tmp);

    std::string addr = std::to_string(reinterpret_cast<uintptr_t>(bytes));
    std::string command = "objdump -D -b binary -m i386:x86-64 --adjust-vma=" + addr + " -f /tmp/amd64_jit.bin";
    system(command.c_str());
  }
}

void *Linker::link(bool dumpDisassembly) {
  auto sectionOffsets = this->mergeSections();
  Section main(std::move(sectionOffsets.first));
  std::map<std::string, uintptr_t> offsets(std::move(sectionOffsets.second));

  // Load the merged section into (later) executable memory.  In the lambda
  // we'll then resolve the references through symbol lookups.
  return this->m_memory.add(main.bytes.data(), main.bytes.size(),
                            [this, &main, &offsets, dumpDisassembly](uint8_t *data, uintptr_t base) {
    for (const Reference &ref : main.references) {
      uintptr_t rip = base + ref.base; // Base address for relative addressing

      auto offsetIt = offsets.find(ref.name);
      if (offsetIt != offsets.end()) {
        // Is this a reference to another section?
        fixUpSectionReference(data, rip, base + offsetIt->second, ref);
      } else if (this->m_registry.has(ref.name)) {
        // Is this a reference to a symbol?
        Symbol sym = this->m_registry.get(ref.name);
        fixUpSymbolReference(data, rip, sym, ref);
      } else {
        // Not found!
        throw std::runtime_error("Can't resolve symbol: " + ref.name);
      }

    }

    if (dumpDisassembly) debugDump(data, main.size());
  });
}

std::pair<Section, std::map<std::string, uintptr_t>> Linker::mergeSections() {
  std::map<std::string, uintptr_t> offsets { { this->m_entryPoint, 0 } };
  Section main(this->m_entryPoint);

  // Reserve memory to speed up the process
  size_t references = 0;
  size_t totalBodySize = 0;

  for (const auto &kv : this->m_sections) {
    references += kv.second.references.size();
    totalBodySize += kv.second.bytes.size();
  }

  main.bytes.reserve(totalBodySize);
  main.references.reserve(references);

  // Add the entry section first!
  auto entryIt = this->m_sections.find(this->m_entryPoint);
  if (entryIt == this->m_sections.end()) {
    throw std::runtime_error("Couldn't find entry-point section " + this->m_entryPoint);
  }

  main.append(entryIt->second);

  // Append all other sections
  for (const auto &kv : this->m_sections) {
    if (kv.first != this->m_entryPoint) {
      offsets.insert({ kv.first, main.bytes.size() });
      main.append(kv.second);
    }
  }

  // Done!
  return { main, offsets };
}

}
