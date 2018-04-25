#ifndef PPU_RENDERER_HPP
#define PPU_RENDERER_HPP

#include "surfacemanager.hpp"
#include "memory.hpp"

namespace Cpu { class Base; }

namespace Ppu {
struct RendererPrivate;

/**
 * Scan-line based renderer for NES graphics.
 *
 * Implements the main logic for NES rendering, going from the data as set by
 * the CPU and cartridge, to a full frame image - Scanline by scanline.
 */
class Renderer {
public:

  // TODO: Don't hardcode NTSC values.

  /** Width of a frame in pixels. */
  static constexpr int WIDTH = 256;

  /** Height of a frame in pixels. */
  static constexpr int HEIGHT = 240;

  Renderer(Memory *vram, SurfaceManager *surfaces, Cpu::Base *cpu);
  ~Renderer();

  /**
   * Draws the next scanline.  Interacts with the given surface manager and the
   * CPU core by itself.  Returns \c true if the current scan line was the last
   * one in the current frame.
   */
  bool drawScanLine();

private:
  RendererPrivate *d;
};
}

#endif // PPU_RENDERER_HPP
