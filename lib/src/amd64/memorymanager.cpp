#include <amd64/memorymanager.hpp>

namespace Amd64 {
MemoryManager::MemoryManager() {

}

MemoryManager::~MemoryManager() {
  for (ExecutableMemory *mem : this->m_blocks)
    delete mem;
}

static void *tryAddBlock(ExecutableMemory *mem, const void *buffer, size_t count,
                        std::function<void(uint8_t *, uintptr_t)> &callback) {
  // Make the region writable
  mem->makeWritable();

  // Copy the buffer
  intptr_t offset = mem->allocate(buffer, count);
  if (offset < 0) {
    mem->makeExecutable();
    return nullptr;
  }

  //
  void *entryPoint = static_cast<char *>(mem->executable()) + offset;

  if (callback) {
    uint8_t *destination = mem->writable() + offset;
    callback(destination, reinterpret_cast<uintptr_t>(entryPoint));
  }

  //
  mem->makeExecutable();
  return entryPoint;
}

void *MemoryManager::add(const void *buffer, size_t count, std::function<void(uint8_t *, uintptr_t)> callback) {
  ExecutableMemory *mem = nullptr;

  // Try to find block with enough space left
  for (auto block : this->m_blocks) {
    void *entryPoint = tryAddBlock(block, buffer, count, callback);
    if (entryPoint != nullptr) {
      return entryPoint;
    }
  }

  // If no block satisfied the request, allocate a new one.
  size_t pageSize = ExecutableMemory::pageSize();
  size_t defaultSize = PAGES_PER_BLOCK * pageSize;
  size_t pages = PAGES_PER_BLOCK;

  // Account for huge buffers:
  if (defaultSize < count) {
    pages = ((count / pageSize) + 1) * 4;
  }

  mem = new ExecutableMemory(pages);
  this->m_blocks.insert(this->m_blocks.begin(), mem);

  //
  void *entryPoint = tryAddBlock(mem, buffer, count, callback);
  if (entryPoint == nullptr) {
    throw std::runtime_error("Failed to insert code block");
  }

  return entryPoint;
}

void MemoryManager::removeFunction(std::vector<ExecutableMemory *>::iterator it, intptr_t offset) {
  ExecutableMemory *block = *it;

  // Decrement function count in this block and check if it's now empty.
  block->deallocate(offset);
  if (!block->isEmpty()) return;

  // If there are too many empty blocks now, deallocate this one.
  if (this->idleBlocks() > MAX_IDLE_BLOCKS) {
    this->m_blocks.erase(it);
    delete block;
  }
}

void MemoryManager::remove(void *execPtr) {
  for (auto it = this->m_blocks.begin(), end = this->m_blocks.end(); it != end; ++it) {
    auto mem = *it;

    void *executable = mem->executable();
    if (execPtr >= executable && execPtr < mem->executableEnd()) {
      intptr_t offset = static_cast<uint8_t *>(execPtr) - static_cast<uint8_t *>(executable);
      removeFunction(it, offset);
      return;
    }
  }
}

size_t MemoryManager::totalCapacity() const {
  size_t sum = 0;
  for (auto mem : this->m_blocks) sum += mem->totalBytes();
  return sum;
}

size_t MemoryManager::totalCapacityLeft() const {
  size_t sum = 0;
  for (auto mem : this->m_blocks) sum += mem->bytesLeft();
  return sum;
}

int MemoryManager::idleBlocks() const {
  int sum = 0;
  for (auto mem : this->m_blocks) {
    if (mem->isEmpty()) sum++;
  }

  return sum;
}
}
