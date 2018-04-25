#ifndef CORE_INESFILE_HPP
#define CORE_INESFILE_HPP

#include <QSharedDataPointer>
#include <QVector>
#include <QByteArray>
#include <cstdint>
class QIODevice;

namespace Core {
class InesFileData;

/**
 * Reader for the \c iNES file format (\c .nes files).
 */
class InesFile {
public:
  /** Size of "trainer" data.  We ignore it. */
  static const int TRAINER_SIZE = 512;

  /** Size of a program code bank. */
  static constexpr int ROM_BANK_SIZE = 16 * 1024;

  /** Size of a character rom bank. */
  static constexpr int VROM_BANK_SIZE = 8 * 1024;

  /** Size of a RAM bank, if supported. */
  static constexpr int RAM_BANK_SIZE = 8 * 1024;

  /** Configuration flags. */
  enum Flag {
    None = 0,

    /** Use vertical name-table mirroring? (Instead of horizontal) */
    VerticalMirroring = 0x01, // flags1 bit 0

    /** Are additional RAM banks saved between sessions? */
    BatteryBackedRam = 0x02, // flags1 bit 1

    /** Is trainer data available? */
    HasTrainer = 0x04, // flags1 bit 2

    /** Use four name-tables?  If yes, \c VerticalMirroring is ignored. */
    FourScreenVram = 0x08, // flags1 bit 3

    /** Is this game for a NES-based arcade machine? */
    VsSystemCartridge = 0x10, // flags2 bit 0

    /** Is this game for PAL (as opposed for NTSC)? */
    IsPal = 0x20, // flags3 bit 0
  };

  Q_DECLARE_FLAGS(Flags, Flag)

  InesFile();
  InesFile(const InesFile &);
  InesFile &operator=(const InesFile &);
  ~InesFile();

  /** Initializes the structure using the data in \a handle. */
  void loadFrom(QIODevice *handle);

  /** Loads an \c InesFile from the file at \a path. */
  static InesFile load(const QString &path);

  /** Configuration flags. */
  InesFile::Flags flags() const;

  /** Identifier of the used mapper chip.  \sa Cartridge::Base */
  int mapperType() const;

  /** Program ROM banks. */
  QVector<QByteArray> romBanks() const;

  /** Character ROM ("Video ROM") banks. */
  QVector<QByteArray> vromBanks() const;

  /** Count of additional RAM banks. */
  int ramBanks() const;

private:
  QSharedDataPointer<InesFileData> d;
};
}

#endif // CORE_INESFILE_HPP
