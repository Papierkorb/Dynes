#ifndef PPU_CORE_HPP
#define PPU_CORE_HPP

#include <core/data.hpp>
#include <ppu_new.hpp>
#include <cstdint>

namespace Ppu {
/**
 * PPU core structure.  This is the core as seen by the CPU!  As such, it
 * mainly (only) hosts variable states that can be read/written through
 * memory mapped registers.
 *
 * The \c Renderer is the class responsible for actually drawing.
 */
class Core : public ::Core::Data {
public:
  typedef std::shared_ptr<Core> Ptr;

  Core();

  /** Value of the PPUCTRL register. */
  Controls control;

  /** Value of the PPUMASK register. */
  Masks mask;

  /** Value of the PPUSTATUS register. */
  StatusFlags status;

  /** X scroll amount. */
  uint8_t scrollX = 0;

  /** Y scroll amount. */
  uint8_t scrollY = 0;

  /** Current read/write address into the OAM. */
  uint8_t oamAddr = 0;

  /** Current read/write address into the VRAM. */
  uint16_t ppuAddr = 0;

  /** Is rendering enabled? */
  bool isEnabled() const;

  /** Should an NMI be triggered upon begin of the VBlank phase? */
  bool triggerNmi() const;

  /**
   * Update the state to reflect the beginning of the next VBlank phase.
   */
  void beginVblank();

  /** Clears the VBlank flag from \c control */
  void unsetVblank();

  /** Sets the Sprite0-Hit flag in \c status. */
  void signalSprite0Hit();

  /** Sets the Sprite Overflow flag in \c status. */
  void signalSpriteOverflow();

  virtual uint8_t read(int address) override;
  virtual void write(int address, uint8_t value) override;

  /**
   * Reads PPUSTATUS.  Also resets the VBlank flag and clears the address latch.
   */
  uint8_t readStatusRegister();

  /** Reads OAMDATA, and advances the OAM address by one. */
  uint8_t readOamData();

  /** Reads PPUDATA, and advances PPUADDR by 1 or 32. */
  uint8_t readPpuData();

  /** Sets PPUCTRL to \a value. */
  void writeControlRegister(uint8_t value);

  /** Sets PPUMASK to \a value. */
  void writeMaskRegister(uint8_t value);

  /** Sets OAMADDR to \a value. */
  void writeOamAddress(uint8_t value);

  /**
   * Sets the X or Y scroll to \a value.  Y is only updated at the start of a
   * frame.
   */
  void writeScrollRegister(uint8_t value);

  /** Sets PPUADDRs low or high byte to \a value. */
  void writePpuAddress(uint8_t value);

private:
  uint8_t m_nextYScroll = 0;
  bool m_addressLatch = false;
};
}

#endif // PPU_CORE_HPP
