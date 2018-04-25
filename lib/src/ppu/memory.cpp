#include <ppu/memory.hpp>

namespace Ppu {
Memory::Memory(const Cartridge::Base::Ptr &cartridge)
  : m_cartridge(cartridge)
{
  this->m_cartridgePtr = this->m_cartridge.get();
  this->reset();
}

bool Memory::isEnabled() const {
  return (this->mask.testFlag(Ppu::EnableBackground) || this->mask.testFlag(Ppu::EnableSprites));
}

bool Memory::triggerNmi() const {
  return this->control.testFlag(Ppu::NmiEnabled);
}

void Memory::reset() {
  this->control = 0;
  this->mask = 0;
  this->status = 0;
  this->scrollX.value = 0;
  this->scrollY.value = 0;
  this->nextScrollY.value = 0;
  this->nextScrollX.value = 0;
  this->oamAddr = 0;
  this->ppuAddr = 0;

  ::memset(this->ram, 0x0, sizeof(this->ram));
  ::memset(this->oam, 0xFF, sizeof(this->oam));
  ::memset(this->palettes, 0x0, sizeof(this->palettes));
}

static constexpr int ppuAddressIncrement(Controls control) {
  return control.testFlag(Ppu::BigIncrement) ? 32 : 1;
}

static constexpr int paletteOffset(int address) {
  return address & (((address & 0x13) == 0x10) ? 0x0F : 0x1F);
}

uint8_t Memory::cpuRead(int address) {
  uint8_t value;

  switch (address) {
  case 2: // PPUSTATUS
    value = static_cast<uint8_t>(this->status);
    this->status.setFlag(Ppu::VBlankStart, false);
    this->m_latch = false;
    return value;
  case 4: // OAMDATA
    value = this->oam[this->oamAddr];
    this->oamAddr++;
    return value;
  case 7: // PPUDATA
    // Surprising behaviour: Read accesses from the CPU below the color palettes
    // are buffered!  Meaning: A read from PPUDATA fetches the next byte (which
    // PPUADDR is pointing at), increments PPUADDR, and then returns the byte
    // from the last fetch.
    if (this->ppuAddr < 0x3F00) { // Buffered read
      value = this->m_buffer;
      this->m_buffer = this->read(this->ppuAddr);
    } else { // Unbuffered read otherwise.
      value = this->m_buffer = this->read(this->ppuAddr);
    }

    this->ppuAddr += ppuAddressIncrement(this->control);
    return value;
  default: // Ignore anything else.
    return 0;
  }
}

void Memory::cpuWrite(int address, uint8_t value) {
  switch (address) {
  case 0: // PPUCTRL
    this->control = Controls(value);
    this->scrollY.nameTable = value & 3;
    break;
  case 1: // PPUMASK
    this->mask = Masks(value);
    break;
  case 3: // OAMADDR
    this->oamAddr = value;
    break;
  case 4: // OAMDATA
    this->oam[this->oamAddr] = value;
    this->oamAddr++;
    break;
  case 5: // PPUSCROLL
    if (!this->m_latch) {
      this->nextScrollX.low = value;
    } else {
      this->nextScrollY.low = value;
    }

    this->m_latch = !this->m_latch;
    break;
  case 6: // PPUADDR
    // Write high byte first, low byte second.
    if (!this->m_latch) {
      this->ppuAddr = (this->ppuAddr & 0x00FF) | ((static_cast<uint16_t>(value) << 8) & 0x3F00);
    } else {
      this->ppuAddr = (this->ppuAddr & 0xFF00) | static_cast<uint16_t>(value);

      // The PPUADDR and PPUSCROLL registers are the same in hardware, it's just
      // that PPUSCROLL does some calculations beforehand.  Some games rely on
      // this behaviour to scroll instead of using PPUSCROLL.
      this->scrollX.low = this->ppuAddr >> 8;
      this->nextScrollX.low = this->ppuAddr >> 8;
      this->nextScrollY.low = this->ppuAddr;
    }

    this->m_latch = !this->m_latch;
    break;
  case 7: // PPUDATA
    this->write(this->ppuAddr, value);
    this->ppuAddr += ppuAddressIncrement(this->control);
    break;
  }
}

static int nameTableAddress(int offset, Mirroring mode) {
  switch (mode) {
  case Single:
    return offset % NAME_TABLE_SIZE;
  case Horizontal:
    return ((offset / 2) & NAME_TABLE_SIZE) | (offset % NAME_TABLE_SIZE);
  case Vertical:
    return offset % (2 * NAME_TABLE_SIZE);
  case Four:
    return offset - NAME_TABLE_BASE;
  default:
    throw std::runtime_error("Unreachable");
  }
}

uint8_t Memory::read(int address) {
  address &= (TOTAL_SIZE - 1); // Ignore higher address bits completely.
  if (address < 0x2000) return this->m_cartridgePtr->readChr(address);
  if (address < 0x3F00) return this->ram[nameTableAddress(address, this->m_cartridgePtr->nameTableMirroring())];
  if (address < 0x4000) return this->palettes[paletteOffset(address)];

  return 0; // Ignore otherwise.
}

void Memory::write(int address, uint8_t value) {
  address &= (TOTAL_SIZE - 1);

  if (address < 0x2000) {
    this->m_cartridgePtr->writeChr(address, value);
  } else if (address < 0x3F00) {
    this->ram[nameTableAddress(address, this->m_cartridgePtr->nameTableMirroring())] = value;
  } else if (address < 0x4000) {
    this->palettes[paletteOffset(address)] = value;
  }
}

Palette Memory::palette(int index) {
  int base = index * 4;

  return Palette(
    this->palettes[0], // Backdrop color is fixed!
    this->palettes[base + 1], // `base+0` is skipped
    this->palettes[base + 2],
    this->palettes[base + 3]
  );
}

OamSprite Memory::sprite(int index) {
  int base = index * 4;
  return *reinterpret_cast<OamSprite *>(this->oam + base);
}

const OamSprite *Memory::sprites() const {
  return reinterpret_cast<const OamSprite *>(this->oam);
}
}
