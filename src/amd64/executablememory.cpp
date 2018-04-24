#include <amd64/executablememory.hpp>

#include <limits>

namespace Amd64 {
ExecutableMemory::ExecutableMemory(size_t pages)
  : m_byteSize(pages * PAGE_SIZE)
{
  this->m_frames.push_back({ Free, 0, this->m_byteSize });
  this->platformConstructor(pages);
}

ExecutableMemory::~ExecutableMemory() {
  this->platformDestructor();
}

size_t ExecutableMemory::bytesLeft() const {
  size_t bytes = 0;

  for (const auto &p : this->m_frames) {
    if (p.state == Free) bytes += p.size;
  }

  return bytes;
}

bool ExecutableMemory::isEmpty() const {
  return (this->m_frames.size() == 1 && this->m_frames.at(0).state == Free);
}

intptr_t ExecutableMemory::allocate(const void *bytes, size_t len) {
  intptr_t offset = this->allocate(len);

  // Copy block if we found a fitting frame
  if (offset >= 0) {
    ::memcpy(writable() + offset, bytes, len);
  }

  return offset;
}

void ExecutableMemory::deallocate(intptr_t offset) {
  auto merger = [](auto &vec, auto l, auto r){ // Merges l+r if both are free
    if (l->state == Free && r->state == Free) {
      l->size += r->size;
      return vec.erase(r) - 1;
    }

    return r;
  };

  auto begin = this->m_frames.begin(), end = this->m_frames.end();
  auto it = begin;

  // Find the frame
  for (; it != end; ++it) {
    if (it->offset == static_cast<size_t>(offset)) break;
  }

  if (it == end) { // Not found?!
    throw std::runtime_error("ExecutableMemory::deallocate: offset not found - Corruption?");
  }

  // Look around the frame and merge it with free frames.
  it->state = Free;

  if (it != begin) { // Is the previous frame free too?
    it = merger(this->m_frames, it - 1, it);
  }

  auto next = it + 1;
  if (next != this->m_frames.end()) { // Check following frame
    merger(this->m_frames, it, next);
  }
}

intptr_t ExecutableMemory::allocate(size_t len) {
  static constexpr size_t OVERHANG_THRESHOLD = 8;

  // Naive best-fit algorithm: Finds the first free large-enough frame, and
  // a free frame that is the smallest yet still fitting one.

  auto it = this->m_frames.begin(), end = this->m_frames.end();
  auto first = end, best = end;
  size_t bestSize = std::numeric_limits<size_t>::max();

  for (; it != end; ++it) {
    if (it->state == Free && it->size >= len) {
      if (first == end) first = it;
      if (it->size < bestSize) {
        best = it;
        bestSize = it->size;

        // If this is an exact fit we're done in any case.
        if (bestSize == len) break;
      }
    }
  }

  // Check if we found a free frame
  if (best != end) it = best;        // Best fit!
  else if (first != end) it = first; // First fit!
  else return -1;                    // None matching.

  it->state = InUse; // Mark frame as used
  if (it->size > len + OVERHANG_THRESHOLD) { // Split frames?
    size_t offset = it->offset + len;
    size_t size = it->size - len;
    it = this->m_frames.insert(it + 1, { Free, offset, size }) - 1;

    it->size = len; // Shorten current frame
  }

  // Done.
  return static_cast<intptr_t>(it->offset);
}
}
