#include <bmpfile.hpp>

#include <QIODevice>
#include <QtEndian>
#include <cstdlib>
#include <cstring>

namespace Test {
static constexpr char FILE_MAGIC[2] = { 'B', 'M' };

struct FileHeader {
  // File header
  char magic[sizeof(FILE_MAGIC)];
  int32_t file_size;
  int32_t reserved;
  int32_t offset;

  // Base information header
  uint32_t bi_size;
  int32_t bi_width;
  int32_t bi_height;
  uint16_t bi_planes;
  uint16_t bi_bit_count;
  uint32_t bi_compression;
  uint32_t bi_image_size;
  int32_t bi_xpels_per_meter;
  int32_t bi_ypels_per_meter;
  uint32_t bi_clr_used;
  uint32_t bi_clr_important;
} __attribute__((packed));

static_assert (sizeof(FileHeader) == 54, "FileHeader must be 54B in size");

#define LE_TO_HOST(Field) Field = qFromLittleEndian(Field)
#define HOST_TO_LE(Field) Field = qToLittleEndian(Field)

// The BMP header is stored Little-Endian
static void headerToHost(FileHeader &hdr) {
  LE_TO_HOST(hdr.file_size);
  LE_TO_HOST(hdr.offset);
  LE_TO_HOST(hdr.bi_size);
  LE_TO_HOST(hdr.bi_width);
  LE_TO_HOST(hdr.bi_height);
  LE_TO_HOST(hdr.bi_planes);
  LE_TO_HOST(hdr.bi_bit_count);
  LE_TO_HOST(hdr.bi_compression);
  LE_TO_HOST(hdr.bi_image_size);
  LE_TO_HOST(hdr.bi_xpels_per_meter);
  LE_TO_HOST(hdr.bi_ypels_per_meter);
  LE_TO_HOST(hdr.bi_clr_used);
  LE_TO_HOST(hdr.bi_clr_important);
}

static void headerToLittleEndian(FileHeader &hdr) {
  HOST_TO_LE(hdr.file_size);
  HOST_TO_LE(hdr.offset);
  HOST_TO_LE(hdr.bi_size);
  HOST_TO_LE(hdr.bi_width);
  HOST_TO_LE(hdr.bi_height);
  HOST_TO_LE(hdr.bi_planes);
  HOST_TO_LE(hdr.bi_bit_count);
  HOST_TO_LE(hdr.bi_compression);
  HOST_TO_LE(hdr.bi_image_size);
  HOST_TO_LE(hdr.bi_xpels_per_meter);
  HOST_TO_LE(hdr.bi_ypels_per_meter);
  HOST_TO_LE(hdr.bi_clr_used);
  HOST_TO_LE(hdr.bi_clr_important);
}

#undef LE_TO_HOST
#undef HOST_TO_LE

BmpFile::BmpFile(size_t width, size_t height, const uint8_t *bytes)
  : m_width(width), m_height(height)
{
  size_t byteCount = this->bitmapByteSize();
  this->m_canvas = reinterpret_cast<uint8_t *>(::calloc(1, byteCount));

  if (bytes) {
    ::memcpy(this->m_canvas, bytes, byteCount);
  }
}

BmpFile::BmpFile(QIODevice *device) {
  FileHeader header;
  if (device->read(reinterpret_cast<char *>(&header), sizeof(header)) != sizeof(header)) {
    throw std::runtime_error("Failed to read BMP header");
  }

  // Check file magic
  if (::memcmp(header.magic, FILE_MAGIC, sizeof(FILE_MAGIC)) != 0) {
    throw std::runtime_error("File does not appear to be a BMP file");
  }

  // Flip bytes:
  headerToHost(header);

  // Sign check dimensions:
  if (header.bi_width < 0 || header.bi_height > 0) {
    throw std::runtime_error("Expected BMP to have inverted Y-axis");
  }

  this->m_width = static_cast<size_t>(header.bi_width);
  this->m_height = static_cast<size_t>(-header.bi_height);

  // Fill canvas
  ssize_t toSkip = header.offset - sizeof(header);
  if (toSkip > 0) device->skip(toSkip);

  size_t canvasSize = this->bitmapByteSize();
  this->m_canvas = reinterpret_cast<uint8_t *>(::calloc(1, canvasSize));
  if (device->read(reinterpret_cast<char *>(this->m_canvas), qint64(canvasSize)) != qint64(canvasSize)) {
    ::free(this->m_canvas);
    throw std::runtime_error("Failed to read full BMP canvas");
  }
}

BmpFile::BmpFile(const BmpFile &other)
  : BmpFile(other.width(), other.height())
{
  ::memcpy(this->m_canvas, other.m_canvas, this->bitmapByteSize());
}

BmpFile::~BmpFile() {
  ::free(this->m_canvas);
}

size_t BmpFile::width() const {
  return this->m_width;
}

size_t BmpFile::height() const {
  return this->m_height;
}

void BmpFile::write(QIODevice *device) const {
  size_t canvasSize = this->bitmapByteSize();

  // Build file header
  FileHeader header;
  ::memcpy(header.magic, FILE_MAGIC, sizeof(header.magic));
  header.file_size = static_cast<uint32_t>(sizeof(header) + canvasSize);
  header.reserved = 0;
  header.offset = sizeof(header);
  header.bi_size = sizeof(header) - 12;
  header.bi_width = static_cast<int32_t>(this->m_width);
  header.bi_height = -static_cast<int32_t>(this->m_height); // Mirrored!
  header.bi_planes = 1;
  header.bi_bit_count = 32;
  header.bi_compression = 0;
  header.bi_image_size = canvasSize;
  header.bi_xpels_per_meter = 0;
  header.bi_ypels_per_meter = 0;
  header.bi_clr_used = 0;
  header.bi_clr_important = 0;

  // Write data
  headerToLittleEndian(header);
  device->write(reinterpret_cast<const char *>(&header), sizeof(header));
  device->write(reinterpret_cast<const char *>(this->m_canvas), qint64(canvasSize));
}

size_t BmpFile::bitmapByteSize() const {
  return this->m_width * this->m_height * sizeof(uint32_t);
}

bool BmpFile::operator==(const BmpFile &other) const {
  if (this == &other) return true;

  return ::memcmp(this->m_canvas, other.m_canvas, this->bitmapByteSize()) == 0;
}
}
