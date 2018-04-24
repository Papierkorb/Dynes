#ifndef AMD64_SYMBOLREGISTRY_HPP
#define AMD64_SYMBOLREGISTRY_HPP

#include <map>

namespace Amd64 {

/**
 * Information about a single symbol.  A "symbol" is basically a named pointer
 * into (host) memory.  These are used by the \c Linker to resolve the
 * \c Reference's from the \c Assembler.
 *
 * A symbol doesn't have a type per-se.  Only a value (Usually the pointer), and
 * a size (to catch programming bugs at link-time).
 */
struct Symbol {
  Symbol(void *p) : isPointer(true), size(sizeof(p)), pointer(p) { }
  Symbol(uint8_t u) : size(sizeof(u)), uint8(u) { }
  Symbol(uint16_t u) : size(sizeof(u)), uint16(u) { }
  Symbol(uint32_t u) : size(sizeof(u)), uint32(u) { }
  Symbol(uint64_t u) : size(sizeof(u)), uint64(u) { }
  Symbol(int8_t i) : size(sizeof(i)), int8(i) { }
  Symbol(int16_t i) : size(sizeof(i)), int16(i) { }
  Symbol(int32_t i) : size(sizeof(i)), int32(i) { }
  Symbol(int64_t i) : size(sizeof(i)), int64(i) { }

  /**
   * Is this symbol a pointer address?  If so, it can be used in relative
   * address calculations.
   */
  bool isPointer = false;

  /** Size of the object, in bytes. */
  size_t size;

  /** The value. */
  union {
    uintptr_t value;
    void *pointer;
    uint8_t uint8;
    uint16_t uint16;
    uint32_t uint32;
    uint64_t uint64;
    int8_t int8;
    int16_t int16;
    int32_t int32;
    int64_t int64;
  };

};

/**
 * Stores symbols to be linked into a function by a \c Linker.
 */
class SymbolRegistry {
public:
  SymbolRegistry();

  /**
   * Adds a symbol to the registry.  If a symbol called \a name already exists
   * the \a symbol will replace the old one.
   */
  void add(const std::string &name, Symbol symbol);

  /** Removes the symbol \a name.  If no such symbol exists nothing happens. */
  void remove(const std::string &name);

  /**
   * Returns the symbol called \a name.  If no such symbol exists throws an
   * exception.
   */
  Symbol get(const std::string &name);

  /** Returns \c true if there's a symbol called \a name. */
  bool has(const std::string &name);

private:
  std::map<std::string, Symbol> m_symbols;
};
}

#endif // AMD64_SYMBOLREGISTRY_HPP
