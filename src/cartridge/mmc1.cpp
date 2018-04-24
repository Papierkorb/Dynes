#include <cartridge/mmc1.hpp>

namespace Cartridge {
enum Register0 {
  MirrorHorizontally = (1 << 0),
  EnableMirroring = (1 << 1),
  SwitchLowProgramBank = (1 << 2),
  SmallProgramBanks = (1 << 3), // 16KiB if set, 32KiB if clear
  SmallCharBanks = (1 << 4), // 4KiB if set, 8KiB if clear
};

/** Written (by guest code) to reset a register. */
static constexpr uint8_t RESET_SIGNAL = 0x80;

/** Base address of the additional RAM. */
static constexpr uint16_t RAM_BASE = 0x6000;

/** Size of additional RAM. */
static constexpr int RAM_SIZE = 0x2000;

/** Base address of register "pages". */
static constexpr uint16_t REGISTER_BASE = 0x8000;

static constexpr uint16_t PRG_BANK0 = 0x8000;
static constexpr uint16_t PRG_BANK1 = 0xC000;

static constexpr uint16_t CHR_BANK0 = 0x0000;
static constexpr uint16_t CHR_BANK1 = 0x1000;

Mmc1::Mmc1(const Core::InesFile &ines)
  : Base(ines),
    m_ines(ines), m_control(SmallProgramBanks), m_charLow(0), m_charHigh(0), m_prg(0),
    m_serial(0), m_serialPos(0), m_ramBank(QByteArray(RAM_SIZE, 0))
{
  // Initially, the first and the last PRG banks are mapped.
  this->m_control = SmallProgramBanks | SmallCharBanks;
  this->m_prg = ines.romBanks().size() - 1;
  this->m_charLow = 0;
  this->m_charHigh = ines.vromBanks().size() - 1;
  this->m_charIsRam = ines.vromBanks().isEmpty();

  if (this->m_charIsRam) {
    this->m_charLowBank = QByteArray(Core::InesFile::VROM_BANK_SIZE, 0);
    this->m_charHighBank = QByteArray(Core::InesFile::VROM_BANK_SIZE, 0);
  }

  this->updateProgramMapping();
  this->updateCharMapping();
}

Mmc1::~Mmc1() {
  //
}

QString Mmc1::name() const {
  return QStringLiteral("MMC1");
}

uint64_t Mmc1::tag() const {
  uint64_t tag = static_cast<uint64_t>(this->m_prg);
  // 0xC = the two PRG Bank control bits
  tag |= static_cast<uint64_t>(this->m_control & 0xC) << 5;
  return tag;
}

uint8_t Mmc1::read(int address) {
  if (address < PRG_BANK0) return this->m_ramBank.ptr[address - RAM_BASE];
  else if (address < PRG_BANK1) return this->m_programLowBank.ptr[address - PRG_BANK0];
  else return this->m_programHighBank.ptr[address - PRG_BANK1];
}

void Mmc1::write(int address, uint8_t value) {
  if (address >= REGISTER_BASE) {
    this->writeRegister(address, value);
  } else {
    this->m_ramBank.mutablePtr[address - RAM_BASE] = value;
  }
}

uint8_t Mmc1::readChr(int address) {
  if (address < CHR_BANK1)
    return this->m_charLowBank.ptr[address - CHR_BANK0];
  else
    return this->m_charHighBank.ptr[address - CHR_BANK1];
}

void Mmc1::writeChr(int address, uint8_t value) {
  if (this->m_charIsRam) {
    if (address < CHR_BANK1)
      this->m_charLowBank.mutablePtr[address - CHR_BANK0] = value;
    else
      this->m_charHighBank.mutablePtr[address - CHR_BANK1] = value;
  }
}

void Mmc1::writeRegister(int address, uint8_t value) {
  if (value & RESET_SIGNAL) {
    this->m_serial = 0; // Reset shift register
    this->m_serialPos = 0;
    this->m_control |= (3 << 2); // Set bits 2 and 3
    return; // Ignore other bits.
  }

  // Write into the shift register.  It's filled from high bit to low bit!
  this->m_serial = (this->m_serial >> 1) | ((value & 1) << 4);
  this->m_serialPos++;

  // Is the shift register "full"?  If so, the write cycle is complete.
  if (this->m_serialPos >= 5) {
    this->updateRegister(address, this->m_serial);

    this->m_serial = 0;
    this->m_serialPos = 0;
  }
}

void Mmc1::updateRegister(int address, uint8_t value) {
  // Register 0 is [0x8000, 0xA000), Register 1 [0xA000, 0xC000), etc.
  int regNum = (address - REGISTER_BASE) >> 13;

  switch(regNum) {
  case 0:
    this->m_control = value;
    this->updateCharMapping();
    this->updateProgramMapping();
    break;
  case 1:
    this->m_charLow = value;
    this->updateCharMapping();
    break;
  case 2:
    this->m_charHigh = value;
    this->updateCharMapping();
    break;
  case 3:
    this->m_prg = value;
    this->updateProgramMapping();
    break;
  default:
    throw std::runtime_error("Unreachable");
  }
}

void Mmc1::updateCharMapping() {
  const QVector<QByteArray> banks = this->m_ines.vromBanks();
  if (this->m_charIsRam) return;

  if (this->m_control & SmallCharBanks) {
    this->m_charLowBank = banks.at(this->m_charLow % banks.size());
    this->m_charHighBank = banks.at(this->m_charHigh % banks.size());
  } else {
    int bankIdx = this->m_charLow >> 1;
    this->m_charLowBank = banks.at(bankIdx % banks.size());
    this->m_charHighBank = banks.at((bankIdx + 1) % banks.size());
  }

  // Update name table mirroring mode.
  if (this->m_control & EnableMirroring) {
    if (this->m_charHigh & MirrorHorizontally) {
      this->m_nameTableMirroring = Ppu::Horizontal;
    } else {
      this->m_nameTableMirroring = Ppu::Vertical;
    }
  } else {
    this->m_nameTableMirroring = Ppu::Single;
  }
}

void Mmc1::updateProgramMapping() {
  const QVector<QByteArray> banks = this->m_ines.romBanks();
  if (this->m_control & SmallProgramBanks) {
    if (this->m_control & SwitchLowProgramBank) {
      this->m_programLowBank = banks.at(this->m_prg % banks.size());
      this->m_programHighBank = banks.last();
    } else {
      this->m_programLowBank = banks.first();
      this->m_programHighBank = banks.at(this->m_prg % banks.size());
    }
  } else {
    // Big bank ignore the lowest bit for addressing.
    int bankIdx = this->m_prg >> 1;
    this->m_programLowBank = banks.at(bankIdx % banks.size());
    this->m_programHighBank = banks.at((bankIdx + 1) % banks.size());
  }
}
}
