#ifndef ANALYSIS_FUNCTION_HPP
#define ANALYSIS_FUNCTION_HPP

#include <QMap>

namespace Analysis {
class Branch;

/**
 * Container for data of an analyzed function.
 */
class Function {
public:
  Function(uint64_t tag, uint16_t begin, bool cacheable);
  ~Function();

  /** Branches of this function. */
  const QMap<uint16_t, Branch *> &branches() const;

  /** Branch at \a address.  If no branch is found, returns \c nullptr. */
  Branch *branch(uint16_t address);

  /** Branch at \a address.  If no branch is found, returns \c nullptr. */
  const Branch *branch(uint16_t address) const;

  /** Start address of this function. */
  uint16_t begin() const;

  /** Cartridge specific configuration tag, for caching. */
  uint64_t tag() const;

  /** The root branch of the function, where execution starts. */
  Branch *root();

  /** Adds \a branch to the function. */
  void add(Branch *branch);

  /** Native name of this function in memory */
  QString nativeName() const;

  /** Can this function be cached? */
  bool cacheable() const { return this->m_cacheable; }

private:
  uint64_t m_tag;
  uint16_t m_begin;
  QMap<uint16_t, Branch *> m_branches;
  bool m_cacheable;
};
}

#endif // DYNAREC_FUNCTION_HPP
