#ifndef CARTRIDGE_MMC1_HPP
#define CARTRIDGE_MMC1_HPP

#include "base.hpp"

namespace Cartridge {

/**
 * Mapper class for the `MMC1` controller chip.
 *
 * Based on http://nesdev.com/mmc1.txt
 */
class Mmc1 : public Base {
public:
  Mmc1(const Core::InesFile &ines);
  ~Mmc1() override;

  QString name() const override;
  uint64_t tag() const override;
  uint8_t read(int address) override;
  void write(int address, uint8_t value) override;
  uint8_t readChr(int address) override;
  void writeChr(int address, uint8_t value) override;

private:
  struct Bank {
    QByteArray data;
    union {
      const uint8_t *ptr;
      uint8_t *mutablePtr;
    };

    Bank(QByteArray d = QByteArray()) { *this = d; }

    Bank &operator=(const QByteArray &d) {
      this->data = d;
      this->ptr = reinterpret_cast<const uint8_t *>(this->data.constData());
      return *this;
    }
  };

  void writeRegister(int address, uint8_t value);
  void updateRegister(int address, uint8_t value);
  void updateCharMapping();
  void updateProgramMapping();

  Core::InesFile m_ines;
  uint8_t m_control; // Register 0
  uint8_t m_charLow; // Register 1
  uint8_t m_charHigh; // Register 2
  uint8_t m_prg; // Register 3
  uint8_t m_serial; // Serial shift register
  int m_serialPos; // Write counter for ^

  bool m_charIsRam;
  Bank m_charLowBank;
  Bank m_charHighBank;
  Bank m_programLowBank;
  Bank m_programHighBank;
  Bank m_ramBank;
};
}

#endif // CARTRIDGE_MMC1_HPP
