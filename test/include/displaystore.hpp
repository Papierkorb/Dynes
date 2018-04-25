#ifndef TEST_DISPLAYSTORE_HPP
#define TEST_DISPLAYSTORE_HPP

#include <ppu/surfacemanager.hpp>

#include <cstddef>

namespace Test {

/**
 * Surface manager for the NES PPU storing the most current frame.  Used so it
 * can be compared to an image a casette provides.
 */
class DisplayStore : public Ppu::SurfaceManager {
public:
  DisplayStore();
  virtual ~DisplayStore();

  /** Returns the size of a frame in bytes. */
  size_t frameByteSize() const;

  /** Returns the current frame. */
  const uint8_t *frame() const;

  /** Width of the frame in pixel. */
  size_t width() const;

  /** Height of the frame in pixel. */
  size_t height() const;

  void displayFrameBuffer(uint32_t *buffer) override;
private:
  uint8_t *m_frame;
};
}

#endif // TEST_DISPLAYSTORE_HPP
