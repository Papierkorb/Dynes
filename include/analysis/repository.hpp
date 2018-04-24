#ifndef ANALYSIS_REPOSITORY_HPP
#define ANALYSIS_REPOSITORY_HPP

#include <QCache>
#include <core/data.hpp>
#include "function.hpp"
#include "functiondisassembler.hpp"

namespace Analysis {
struct CacheKey {
  uint64_t tag;
  uint16_t addr;

  // Why C++ (According to Qt) can't generate this one itself is beyond me.
  constexpr bool operator==(CacheKey other) const {
    return this->tag == other.tag && this->addr == other.addr;
  }
};

constexpr uint qHash(CacheKey key, uint seed = 0) {
  return ::qHash(key.tag, seed) ^ ::qHash(key.addr, seed);
}

/**
 * Repository of analyzed functions.  Caches up to a configurable amount of
 * functions, after which it starts to remove the oldest function LRU style.
 *
 * Uses the address, and CPU memory configuration tag, as caching key.
 *
 * \sa Cpu::Memory::tag()
 */
template<typename FuncT>
class Repository {
public:
  static constexpr int DEFAULT_CACHE_SIZE = 1000;
  typedef std::function<FuncT*(Analysis::Function&)> Packer;

  explicit Repository(const Core::Data::Ptr &mem, Packer packer, int cacheSize = DEFAULT_CACHE_SIZE)
    : m_memory(mem), m_packer(packer), m_cache(cacheSize)
  { }

  /**
   * Evicts the function at \a address from the cache.
   */
  void evict(uint16_t address) {
    CacheKey key{ .tag = this->m_memory->tag(), .addr = address };
    this->m_cache.remove(key);
  }

  /**
   * Gets the function at \a address.  It'll automatically be added to the
   * cache.
   */
  FuncT *get(uint16_t address) {
    CacheKey key{ .tag = this->m_memory->tag(), .addr = address };
    FuncT *compiled = this->m_cache.object(key);

    if (!compiled) {
      FunctionDisassembler disasm(this->m_memory);
      Function base = disasm.disassemble(address);
      compiled = this->m_packer(base);

      if (base.cacheable()) this->m_cache.insert(key, compiled);
    }

    return compiled;
  }

  /** Removes all functions from the cache. */
  void clear() { this->m_cache.clear(); }

private:
  Core::Data::Ptr m_memory;
  Packer m_packer;
  QCache<CacheKey, FuncT> m_cache;
};
}

#endif // ANALYSIS_REPOSITORY_HPP
