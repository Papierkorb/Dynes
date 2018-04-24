#include <core/data.hpp>

namespace Core {
int Data::read(int address, int size, uint8_t *buffer) {
  for (int i = 0; i < size; i++) {
    buffer[i] = this->read(address + i);
  }

  return size;
}
}
