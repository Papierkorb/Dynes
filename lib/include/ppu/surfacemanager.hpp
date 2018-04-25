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
   * Displays the data in \a buffer.
   */
  virtual void displayFrameBuffer(uint32_t *buffer) = 0;
};
}

#endif // PPU_SURFACEMANAGER_HPP
