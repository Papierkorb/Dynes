#include <core/inesfile.hpp>

#include <QFile>
#include <QIODevice>
#include <QVector>

/** First bytes of a proper .nes file */
static const char FILE_MAGIC[] = { 'N', 'E', 'S', 0x1A };

union FileMagic {
  uint32_t num;
  char string[sizeof(FILE_MAGIC)];
};

static_assert(sizeof(uint32_t) == sizeof(FILE_MAGIC), "Magic has wrong size");
static_assert(sizeof(FileMagic) == sizeof(FILE_MAGIC), "Union has wrong size");

namespace Core {
struct InesFileData : public QSharedData {
  InesFile::Flags flags = InesFile::None;

  QVector<QByteArray> romBanks;
  QVector<QByteArray> vromBanks;
  int mapperType = -1;
  int ramBanks = 0;
};

InesFile::InesFile()
  : d(new InesFileData) {

}

InesFile::InesFile(const InesFile &rhs)
  : d(rhs.d) {
}

InesFile &InesFile::operator=(const InesFile &rhs) {
  if (this != &rhs)
    d.operator=(rhs.d);
  return *this;
}

InesFile::~InesFile() {
}

template<typename T>
static T checkedRead(QIODevice *handle) {
  T t;

  if (handle->read(reinterpret_cast<char *>(&t), sizeof(t)) != sizeof(t)) {
    throw std::runtime_error("File too small");
  }

  return t;
}

static std::tuple<InesFile::Flags, uint8_t> decodeFlags(uint8_t a, uint8_t b, uint8_t c) {
  uint8_t bits = (a & 0x0F) | ((b & 1) << 4) | ((c & 1) << 5);
  uint8_t type = (a >> 4) | (b & 0xF0);
  return { InesFile::Flags(bits), type };
}

static QVector<QByteArray> readBanks(QIODevice *handle, int count, int size) {
  QVector<QByteArray> list;

  for (int i = 0; i < count; i++) {
    QByteArray bank = handle->read(size);

    if (bank.size() != size) throw std::runtime_error("File too small (Bank)");
    list << bank;
  }

  return list;
}

void InesFile::loadFrom(QIODevice *handle) {
  auto magic = checkedRead<FileMagic>(handle);
  if (memcmp(magic.string, FILE_MAGIC, sizeof(FILE_MAGIC)) != 0)
    throw std::runtime_error("Invalid magic");

  // Read header fields:
  uint8_t romBanks = checkedRead<uint8_t>(handle);
  uint8_t vromBanks = checkedRead<uint8_t>(handle);
  uint8_t flags1 = checkedRead<uint8_t>(handle);
  uint8_t flags2 = checkedRead<uint8_t>(handle);
  uint8_t ramBanks = checkedRead<uint8_t>(handle);
  uint8_t flags3 = checkedRead<uint8_t>(handle);

  handle->skip(6); // Skip unused bytes

  // Process flags
  Flags flags;
  uint8_t mapper;
  std::tie(flags, mapper) = decodeFlags(flags1, flags2, flags3);

  this->d->flags = flags;
  this->d->mapperType = mapper;

  // Skip trainer if any
  if (flags.testFlag(HasTrainer)) handle->skip(TRAINER_SIZE);

  // Read banks
  this->d->romBanks = readBanks(handle, romBanks, ROM_BANK_SIZE);
  this->d->vromBanks = readBanks(handle, vromBanks, VROM_BANK_SIZE);
  this->d->ramBanks = qMax(1, int(ramBanks));
}

InesFile InesFile::load(const QString &path) {
  QFile file(path);
  InesFile ines;

  if (file.open(QIODevice::ReadOnly)) {
    ines.loadFrom(&file);
  } else {
    throw std::runtime_error("Failed to open ROM");
  }

  return ines;
}

InesFile::Flags InesFile::flags() const {
  return this->d->flags;
}

int InesFile::mapperType() const {
  return this->d->mapperType;
}

QVector<QByteArray> InesFile::romBanks() const {
  return this->d->romBanks;
}

QVector<QByteArray> InesFile::vromBanks() const {
  return this->d->vromBanks;
}

int InesFile::ramBanks() const {
  return this->d->ramBanks;
}
}
