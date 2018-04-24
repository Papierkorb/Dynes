#include <amd64/executablememory.hpp>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>

namespace Amd64 {
void ExecutableMemory::platformConstructor(size_t) {
  this->m_addr = ::mmap(nullptr, this->m_byteSize, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (this->m_addr == MAP_FAILED) {
    throw std::runtime_error("Failed to acquire memory");
  }
}

void ExecutableMemory::platformDestructor() {
  ::munmap(this->m_addr, this->m_byteSize);
}

void ExecutableMemory::makeWritable() {
  ::mprotect(this->m_addr, this->m_byteSize, PROT_READ | PROT_WRITE);
}

void ExecutableMemory::makeExecutable() {
  ::mprotect(this->m_addr, this->m_byteSize, PROT_READ | PROT_EXEC);
}

uint8_t *ExecutableMemory::writable() {
  return reinterpret_cast<uint8_t *>(this->m_addr);
}

void *ExecutableMemory::executable() {
  return this->m_addr;
}

size_t ExecutableMemory::pageSize() {
  return PAGE_SIZE;
}
}
