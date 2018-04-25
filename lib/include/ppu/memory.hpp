#ifndef PPU_MEMORY_HPP
#define PPU_MEMORY_HPP

#include <cartridge/base.hpp>

#include <ppu.hpp>

namespace Ppu {
/**
 * Manages the memory of the PPU ("VRAM").
 *
 * This includes its flags, which are publicly accessible to speed-up access
 * from within the renderer.
 */
class Memory {
public:
  typedef std::shared_ptr<Memory> Ptr;

  /** Size of the OAM. */
  static constexpr int OAM_SIZE = 256;

  /** Size of the color palette memory. */
  static constexpr int PALETTES_SIZE = 32;

  /**
   * Size of the VRAM.  Double the normal size so we can in principle support
   * four name tables.
   */
  static constexpr int MEMORY_SIZE = 2 * 2048;

  Memory(const Cartridge::Base::Ptr &cartridge);

  /** Value of the PPUCTRL register. */
  Controls control = 0;

  /** Value of the PPUMASK register. */
  Masks mask = 0;

  /** Value of the PPUSTATUS register. */
  StatusFlags status = 0;

  /** X/Y scroll amount. */
  Scroll scrollX = { .value = 0 };
  Scroll nextScrollX = { .value = 0 };
  Scroll scrollY = { .value = 0 };
  Scroll nextScrollY = { .value = 0 };

  /** Current read/write address into the OAM. */
  uint8_t oamAddr = 0;

  /** Current read/write address into the VRAM. */
  uint16_t ppuAddr = 0;

  /** Object Attribute Memory, stores information of Sprites. */
  uint8_t oam[OAM_SIZE];

  /** Color palettes memory. */
  uint8_t palettes[PALETTES_SIZE];

  /** Memory for name tables. */
  uint8_t ram[MEMORY_SIZE];

  /** Is rendering enabled? */
  bool isEnabled() const;

  /** Should an NMI be triggered upon begin of the VBlank phase? */
  bool triggerNmi() const;

  /** Resets the internal state. */
  void reset();

  /** Reads a byte at \a address.  This is only accessed from the CPU! */
  uint8_t cpuRead(int address);

  /** Writes \a value into \a address.  This is only accessed from the CPU! */
  void cpuWrite(int address, uint8_t value);

  /** Reads a byte at \a address. */
  uint8_t read(int address);

  /** Reads \a index'th palette. */
  Palette palette(int index);

  /** Reads the \a index'th sprite. */
  OamSprite sprite(int index);

  const OamSprite *sprites() const;

  /** Writes \a value into \a address. */
  void write(int address, uint8_t value);

  /** Returns the cartridge mapper. */
  Cartridge::Base::Ptr cartridge() const
  { return this->m_cartridge; }

private:
  Cartridge::Base::Ptr m_cartridge;
  Cartridge::Base *m_cartridgePtr;
  bool m_latch = false;
  uint8_t m_buffer;
};
}

#endif // PPU_MEMORY_HPP
