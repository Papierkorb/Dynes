#ifndef AMD64_MEMORYMANAGER_HPP
#define AMD64_MEMORYMANAGER_HPP

#include "executablememory.hpp"
#include <functional>
#include <vector>

namespace Amd64 {

/**
 * Manager for blocks of writable and executable memory on the host system.
 *
 * Used to manage the life-cycle of a function.
 */
class MemoryManager {
  MemoryManager(const MemoryManager &) = delete;
  MemoryManager(MemoryManager &&) = delete;
public:
  static constexpr int PAGES_PER_BLOCK = 4;
  static constexpr int MAX_IDLE_BLOCKS = 2;

  MemoryManager();
  ~MemoryManager();

  /**
   * Adds the function in \a buffer of \a count bytes to the executable memory,
   * returning the executable pointer to it.
   *
   * If \a callback is given, it'll be called with the \b writable pointer to
   * where the \a buffer was copied to as first argument, and the entry-point
   * address as second argument.  This can be used to modify the data further,
   * e.g. to do instruction-pointer relative address calculations.
   */
  void *add(const void *buffer, size_t count, std::function<void(uint8_t *, uintptr_t)> callback = nullptr);

  /**
   * Removes the function at \a execPtr from the memory, which was initially
   * returned by \c add().
   */
  void remove(void *execPtr);

  /** Total amount of bytes allocated. */
  size_t totalCapacity() const;

  /** Total amount of bytes left. */
  size_t totalCapacityLeft() const;

  /** Count of blocks that are completely empty. */
  int idleBlocks() const;

private:
  void removeFunction(std::vector<ExecutableMemory *>::iterator it, intptr_t offset);

  std::vector<ExecutableMemory *> m_blocks;
};
}

#endif // AMD64_MEMORYMANAGER_HPP
