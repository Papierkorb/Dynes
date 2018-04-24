#include <cpu/memory.hpp>

#include <cstring>

// Traces all memory accesses.  This is not only verbose, it's also really slow.
// Combine with instruction tracing for more informative results.
// NOTE: If the CPU core does direct memory access to e.g. RAM, not all accesses
//       may show up!
//#define TRACE_ACCESS true
//#define TRACE_ACCESS (address < 0x100 || address > 0x1FF)
//#define TRACE_ACCESS (address == 0x0720 || address == 0x0721 || (address >= 0x2000 && address < 0x4008))

namespace Cpu {

Memory::Memory(const Ppu::Memory::Ptr &vram, const Cartridge::Base::Ptr &cartridge)
  : m_cartridge(cartridge), m_vram(vram), m_firstPlayer(0), m_secondPlayer(0)
{
  this->m_cartridgePtr = this->m_cartridge.get();
  this->m_vramPtr = this->m_vram.get();
}

uint64_t Memory::tag() const {
  return this->m_cartridgePtr->tag();
}

uint8_t Memory::read(int address) {  
  uint8_t value = 0xFF;

  if (address < 0x2000) value = this->m_ram[address & 0x7FF];
  else if (address < 0x4000) value = this->m_vramPtr->cpuRead(address & 7);
  else if (address < 0x4018) value = this->readIo(address - 0x4000);
  else if (address <= 0xFFFF) value = this->m_cartridgePtr->read(address);
  else throw std::runtime_error("Unreachable!");

#ifdef TRACE_ACCESS
  if (TRACE_ACCESS) fprintf(stderr, "mem.read[%04x] -> %02x\n", address, value);
#endif

  return value;
}

void Memory::write(int address, uint8_t value) {
#ifdef TRACE_ACCESS
  if (TRACE_ACCESS) fprintf(stderr, "mem.write[%04x] <- %02x\n", address, value);
#endif

  if (address < 0x2000) this->m_ram[address & 0x7FF] = value;
  else if (address < 0x4000) return this->m_vramPtr->cpuWrite(address & 7, value);
  else if (address < 0x4018) return this->writeIo(address - 0x4000, value);
  else if (address <= 0xFFFF) return this->m_cartridgePtr->write(address, value);
  else throw std::runtime_error("Unreachable!");
}

uint16_t Memory::read16(uint16_t address) {
  // When the high address would cross page boundaries, it doesn't go into the
  // next page.  It actually loops around in the local page.  This could be
  // regarded as a bug in the 6502.
  int highAddr = (address & 0xFF00) | ((address + 1) & 0x00FF);

  uint16_t lo = static_cast<uint16_t>(this->read(address));
  uint16_t hi = static_cast<uint16_t>(this->read(highAddr));

  return (hi << 8) | lo;
}

void Memory::reset() {
  ::memset(this->m_ram, 0x00, sizeof(this->m_ram));
}

uint8_t Memory::readIo(int offset) {
  switch (offset) {
  case 0x14: return 0;
  case 0x16: return this->m_firstPlayer.read();
  case 0x17: return this->m_secondPlayer.read();
  default: return 0;
  }
}

void Memory::writeIo(int offset, uint8_t value) {
  switch (offset) {
  case 0x14:
    this->oamDma(static_cast<uint32_t>(value));
    break;
  case 0x16:
    this->m_firstPlayer.write(value & 1);
    this->m_secondPlayer.write(value & 1);
    break;
  }
}

void Memory::oamDma(int page) {
  int base = page * PAGE_SIZE;

  for (int i = 0; i < 256; i++) {
    this->m_vramPtr->cpuWrite(4, this->read(base + i));
  }
}
}
