#include <ppu/core.hpp>

namespace Ppu {
Core::Core()
  : control(0), mask(0), status(0)
{

}

bool Core::isEnabled() const {
  return this->mask.testFlag(Ppu::EnableBackground) || this->mask.testFlag(Ppu::EnableSprites);
}

bool Core::triggerNmi() const {
  return this->control.testFlag(Ppu::NmiEnabled);
}

void Core::beginVblank() {
  this->status = Ppu::VBlankStart;
  this->scrollY = this->m_nextYScroll;
  this->oamAddr = 0;
}

void Core::unsetVblank() {
  this->status.setFlag(Ppu::VBlankStart, false);
}

void Core::signalSprite0Hit() {
  this->status |= Ppu::SpriteHit;
}

void Core::signalSpriteOverflow() {
  this->status |= Ppu::SpriteOverflow;
}

uint8_t Core::read(int address) {
  switch(address & 7) {
  case 0: return static_cast<uint8_t>(this->control);
  case 1: return static_cast<uint8_t>(this->mask);

  case 2: return this->readStatusRegister();
  case 4: return this->readOamData();
  case 7: return this->readPpuData();
  default: return 0;
  }
}

void Core::write(int address, uint8_t value) {
  switch(address & 7) {
  case 0:
    this->writeControlRegister(value);
    break;
  case 1:
    this->writeMaskRegister(value);
    break;
  case 3:
    this->writeOamAddress(value);
    break;
  case 4:
    this->writeOamData(value);
    break;
  case 5:
    this->writeScrollRegister(value);
    break;
  case 6:
    this->writePpuAddress(value);
    break;
  case 7:
    this->writePpuData(value);
    break;
  default:
    break; // Ignore.
  }
}

uint8_t Core::readStatusRegister() {
  uint8_t value = static_cast<uint8_t>(this->status);

  this->m_addressLatch = false;
  this->status.setFlag(Ppu::VBlankStart, false);

  return value;
}

void Core::writeControlRegister(uint8_t value) {
  this->control = Ppu::Controls(value);
}

void Core::writeMaskRegister(uint8_t value) {
  this->mask = Ppu::Masks(value);
}

void Core::writeOamAddress(uint8_t value) {
  this->oamAddr = value;
}

void Core::writeScrollRegister(uint8_t value) {
  if (this->m_addressLatch)
    this->m_nextYScroll = value;
  else
    this->scrollX = value;

  this->m_addressLatch = !this->m_addressLatch;
}

void Core::writePpuAddress(uint8_t value) {
  uint16_t value16 = static_cast<uint16_t>(value);

  if (this->m_addressLatch)
    this->ppuAddr = (this->ppuAddr & 0xFF00) | value16;
  else
    this->ppuAddr = (this->ppuAddr & 0x00FF) | (value16 << 8);

  this->m_addressLatch = !this->m_addressLatch;
}
}
