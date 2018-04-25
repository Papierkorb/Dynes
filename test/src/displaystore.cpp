#include <displaystore.hpp>

#include <cstdlib>

#include <ppu/renderer.hpp>

namespace Test {
static constexpr size_t FRAME_BYTE_SIZE = Ppu::Renderer::WIDTH * Ppu::Renderer::HEIGHT * sizeof(uint32_t);

DisplayStore::DisplayStore() {
  this->m_frame = reinterpret_cast<uint8_t *>(::calloc(FRAME_BYTE_SIZE, 1));
}

DisplayStore::~DisplayStore() {
  ::free(this->m_frame);
}

size_t DisplayStore::frameByteSize() const {
  return FRAME_BYTE_SIZE;
}

const uint8_t *DisplayStore::frame() const {
  return this->m_frame;
}

size_t DisplayStore::width() const {
  return Ppu::Renderer::WIDTH;
}

size_t DisplayStore::height() const {
  return Ppu::Renderer::HEIGHT;
}

void DisplayStore::displayFrameBuffer(uint32_t *buffer) {
  ::memcpy(this->m_frame, buffer, FRAME_BYTE_SIZE);
}
}
