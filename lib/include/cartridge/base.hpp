#ifndef CARTRIDGE_BASE_HPP
#define CARTRIDGE_BASE_HPP

#include <core/inesfile.hpp>
#include <ppu.hpp>
#include <memory>

namespace Cartridge {
/**
 * Base class for cartridge mappers.
 *
 * A "mapper" was a physical chip inside any NES cartridge which mapped
 * memory requests from the CPU and PPU into its own address space.  Inside it,
 * it can treat read and write requests any way it likes.
 *
 * All mappers can respond to requests from the CPU for program memory ("PRG"),
 * and from the PPU for character memory ("CHR").
 */
class Base {
public:
  typedef std::shared_ptr<Base> Ptr;

  Base(const Core::InesFile &ines);

  virtual ~Base();

  /** The readable name of the mapper chip. */
  virtual QString name() const = 0;

  /**
   * Tag of the current mapping configuration.  Used to cache functions.
   *
   * If the mapping configuration changes (E.g., bank switches, RAM content
   * changes), this should be reflected in a changed tag.
   */
  virtual uint64_t tag() const = 0;

  /** Reads from PRG at \a address. */
  virtual uint8_t read(int address) = 0;

  /** Writes \a value into PRG at \a address. */
  virtual void write(int address, uint8_t value) = 0;

  /** Reads from CHR at \a address. */
  virtual uint8_t readChr(int address) = 0;

  /** Writes \a value into CHR at \a address. */
  virtual void writeChr(int address, uint8_t value) = 0;

  /** The currently active name table mirroring mode. */
  Ppu::Mirroring nameTableMirroring() const
  { return this->m_nameTableMirroring; }

  /**
   * Factory method, creates a mapper instance for \a id, using data from
   * \a ines.
   */
  static Ptr createById(int id, const Core::InesFile &ines);

protected:
  Ppu::Mirroring m_nameTableMirroring;
};
}

#endif // CARTRIDGE_BASE_HPP
