#ifndef TEST_BMPFILE_HPP
#define TEST_BMPFILE_HPP

#include <cstdint>
#include <cstddef>

class QIODevice;

namespace Test {
/**
 * Simplistic BMP-file reader and writer.  Only supports a subset of BMP
 * features.  Used by the test-runner to save and load screenshots for
 * comparison.
 */
class BmpFile {
public:
  /** Creates an empty, black bitmap. */
  BmpFile(size_t width, size_t height, const uint8_t *bytes = nullptr);

  /** Reads a bitmap from \a device. */
  BmpFile(QIODevice *device);

  /** Copy constructor. */
  BmpFile(const BmpFile &other);

  ~BmpFile();

  /** Width of the bitmap in pixels. */
  size_t width() const;

  /** Height of the bitmap in pixels. */
  size_t height() const;

  /** Writes the bitmap out to \a device. */
  void write(QIODevice *device) const;

  /** Counf of bytes allocated for the bitmap. */
  size_t bitmapByteSize() const;

  /** Is this bitmap the same as \a other? */
  bool operator==(const BmpFile &other) const;

private:
  size_t m_width;
  size_t m_height;
  uint8_t *m_canvas;
};
}

#endif // TEST_BMPFILE_HPP
