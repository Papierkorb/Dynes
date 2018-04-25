#ifndef CORE_DATA_HPP
#define CORE_DATA_HPP

#include <cstdint>
#include <memory>
#include <QByteArray>

namespace Core {
/** Abstract data access structure. */
class Data {
public:
  typedef std::shared_ptr<Data> Ptr;

  virtual ~Data() { }

  /**
   * State hash of this object.  If the state of the object is changed,
   * this tag value is expected to change as well.
   *
   * Used by \c Analysis::Repository to support caching of banked functions.
   */
  virtual uint64_t tag() const = 0;

  /** Reads the byte at \a address. */
  virtual uint8_t read(int address) = 0;

  /** Writes \a value into the byte at \a address. */
  virtual void write(int address, uint8_t value) = 0;

  /**
   * Reads up to \a size bytes from \a address on into \a buffer.
   *
   * @return The count of bytes copied.
   */
  virtual int read(int address, int size, uint8_t *buffer);

};
}

#endif // CORE_DATA_HPP
