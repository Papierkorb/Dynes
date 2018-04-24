#ifndef AMD64_EXECUTABLEMEMORY_HPP
#define AMD64_EXECUTABLEMEMORY_HPP

#include <stdexcept>
#include <cstring>
#include <vector>

namespace Amd64 {

/**
 * Operating system agnostic API to give access to read/write and read/execute
 * memory pages.
 */
class ExecutableMemory {
  ExecutableMemory(const ExecutableMemory &) = delete;
  ExecutableMemory(ExecutableMemory &&) = delete;
public:
  ExecutableMemory(size_t pages = 1);
  ~ExecutableMemory();

  /** Remaps the memory region to be writable. */
  void makeWritable();

  /** Remaps the memory region to be executable. */
  void makeExecutable();

  /**
   * Returns the writable pointer.  The caller \b must call \c makeWritable()
   * manually before.
   *
   * \note Don't assume that the writable and executable pointers point to the
   * same (virtual) memory addresses.
   */
  uint8_t *writable();

  /**
   * Returns the executable pointer.  The caller \b must call
   * \c makeExecutable() manually before.
   *
   * \note Don't assume that the writable and executable pointers point to the
   * same (virtual) memory addresses.
   */
  void *executable();

  /**
   * The pointer pointing just past the last byte in the executable memory
   * range.
   */
  void *executableEnd() { return reinterpret_cast<uint8_t *>(this->executable()) + this->m_byteSize; }

  /** Total size of this memory block in Bytes. */
  size_t totalBytes() const { return this->m_byteSize; }

  /** Total count of unused bytes. */
  size_t bytesLeft() const;

  /** Total count of used bytes. */
  size_t bytesUsed() const { return this->m_byteSize - this->bytesLeft(); }

  /** Is this memory block not in use? */
  bool isEmpty() const;

  /**
   * Tries to append the block beginning at \a bytes to this.  If there's not
   * enough space left returns \c -1.  Returns the offset from the beginning
   * otherwise.
   */
  intptr_t allocate(const void *bytes, size_t len);

  /** Deallocates the allocated memory at \a offset. */
  void deallocate(intptr_t offset);

  /** Size of a memory page. */
  static size_t pageSize();

private:
  enum FrameState { Free, InUse };
  struct Frame { FrameState state; size_t offset; size_t size; };

  intptr_t allocate(size_t len);
  void platformConstructor(size_t pages);
  void platformDestructor();

  size_t m_byteSize;

  std::vector<Frame> m_frames;

#if defined(__x86_64__) || defined(__x86__)
  static constexpr int PAGE_SIZE = 4096;
#endif

#ifdef __linux__
  void *m_addr;
#else
#  error Missing implementation for this platform.
#endif
};
}

#endif // AMD64_EXECUTABLEMEMORY_HPP
