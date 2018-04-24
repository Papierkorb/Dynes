#ifndef CPU_MEMORY_HPP
#define CPU_MEMORY_HPP

#include <core/data.hpp>
#include <core/gamepad.hpp>
#include <ppu/memory.hpp>
#include <cartridge/base.hpp>

namespace Cpu {
/**
 * Controller of memory as seen by the CPU.
 */
class Memory : public Core::Data {
public:
  typedef std::shared_ptr<Memory> Ptr;

  /** Size of the RAM, starting at address 0x0000. */
  static constexpr int RAM_SIZE = 2048; // 2KiB

  /** Addresses before this address are in RAM. */
  static constexpr uint16_t RAM_BARRIER = 0x2000;

  /** Size of a memory page (or "bank"). */
  static constexpr int PAGE_SIZE = 256; // 256B

  Memory(const Ppu::Memory::Ptr &vram, const Cartridge::Base::Ptr &cartridge);

  uint64_t tag() const override;
  uint8_t read(int address) override;
  void write(int address, uint8_t value) override;
  uint16_t read16(uint16_t address);

  /** Re-initializes the memory for a cold start. */
  void reset();

  /** Returns the RAM pointer. */
  uint8_t *ram() { return this->m_ram; }

  /** Input state of the first players gamepad. */
  Core::Gamepad &firstPlayer()
  { return this->m_firstPlayer; }

  /** Input state of the second players gamepad. */
  Core::Gamepad &secondPlayer()
  { return this->m_secondPlayer; }

private:
  uint8_t readIo(int offset);
  void writeIo(int offset, uint8_t value);
  void oamDma(int page);

  uint8_t m_ram[RAM_SIZE];
  Cartridge::Base::Ptr m_cartridge;
  Cartridge::Base *m_cartridgePtr;
  Ppu::Memory::Ptr m_vram;
  Ppu::Memory *m_vramPtr;

  Core::Gamepad m_firstPlayer;
  Core::Gamepad m_secondPlayer;
};
}


#endif // CPU_MEMORY_HPP
