#include <cartridge/base.hpp>
#include <cartridge/nrom.hpp>
#include <cartridge/mmc1.hpp>

namespace Cartridge {

Base::Base(const Core::InesFile &ines) {
  if (ines.flags().testFlag(Core::InesFile::VerticalMirroring)) {
    this->m_nameTableMirroring = Ppu::Vertical;
  } else {
    this->m_nameTableMirroring = Ppu::Horizontal;
  }

}

Base::~Base() {
  // Do nothing.
}

Base::Ptr Base::createById(int id, const Core::InesFile &ines) {
  switch (id) {
  case 0: return Ptr(new Nrom(ines));
  case 1: return Ptr(new Mmc1(ines));
  default: throw std::runtime_error("Unknown mapper id");
  }
}
}
