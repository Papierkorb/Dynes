#ifndef CARTRIDGE_NROM_HPP
#define CARTRIDGE_NROM_HPP

#include "base.hpp"

namespace Cartridge {
class Nrom : public Base {
public:
  Nrom(const Core::InesFile &ines);
  ~Nrom() override;

  QString name() const override;
  uint64_t tag() const override;
  uint8_t read(int address) override;
  void write(int address, uint8_t value) override;
  uint8_t readChr(int address) override;
  void writeChr(int address, uint8_t value) override;

private:
  QByteArray banks[2];
  uint8_t *m_prgFirst;
  uint8_t *m_prgSecond;
  uint8_t *m_chrFirst;
};
}

#endif // CARTRIDGE_NROM_HPP
