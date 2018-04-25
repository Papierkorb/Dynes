#ifndef PPU_SURFACEMANAGER_HPP
#define PPU_SURFACEMANAGER_HPP

#include <cstdint>

namespace Ppu {
/**
 * Glue class to connect the back-end PPU renderer with a display front-end.
 */
class SurfaceManager {
public:

  /**
   * Acquires a buffer to be used for the next frame.  The returned pointer must
   * be large enough in size for the frame.  The returned pointer may be one
   * that was earlier checkout out by the renderer, but only if it was checked
   * back in through \c displayFrameBuffer before.
   */
  virtual uint32_t *nextFrameBuffer() = 0;

  /**
   * Displays the data in \a buffer.
   */
  virtual void displayFrameBuffer(uint32_t *buffer) = 0;
};
}

#endif // PPU_SURFACEMANAGER_HPP
