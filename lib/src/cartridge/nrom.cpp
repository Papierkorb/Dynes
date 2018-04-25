#include <cartridge/nrom.hpp>

namespace Cartridge {

Nrom::Nrom(const Core::InesFile &ines)
  : Base(ines)
{
  this->banks[0] = ines.romBanks().first() + ines.romBanks().last();
  this->banks[1] = ines.vromBanks().first();

  // Keep pointers for faster access.
  this->m_prgFirst = reinterpret_cast<uint8_t *>(this->banks[0].data());
  this->m_chrFirst = reinterpret_cast<uint8_t *>(this->banks[1].data());
}

Nrom::~Nrom() {
  // Nothing.
}

QString Nrom::name() const {
  return QStringLiteral("NROM");
}

uint64_t Nrom::tag() const {
  // NROM doesn't support any bank switches, nor writes to it, so we can return
  // a constant numeral.
  return 0;
}

uint8_t Nrom::read(int address) {
  if (address < 0x8000) return 0; // Bounds check
  return this->m_prgFirst[address - 0x8000];
}

void Nrom::write(int address, uint8_t value) {
  Q_UNUSED(address);
  Q_UNUSED(value);

  // N-ROM ignores write access.
}

uint8_t Nrom::readChr(int address) {
  return this->m_chrFirst[address];
}

void Nrom::writeChr(int address, uint8_t value) {
  Q_UNUSED(address);
  Q_UNUSED(value);

  // N-ROM ignores write access.
}

}
