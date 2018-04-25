#ifndef CPU_BASE_HPP
#define CPU_BASE_HPP

#include <QObject>
#include <cpu.hpp>
#include "state.hpp"

#include <cpu/memory.hpp>
#include <core/data.hpp>
#include <QMap>

#include "hook.hpp"

namespace Cpu {

/**
 * Base class for CPU cores.  Implemented by the front-end of all available CPU
 * cores.  Offers a common API, and commonly useful functionality.
 */
class Base : public QObject {
  Q_OBJECT
public:
  Base(const Memory::Ptr &memory, State state = State(), QObject *parent = nullptr);
  ~Base() override;

  /**
   * Advances the simulated processor by at least \a cycles.
   * Returns the count of remaining cycles.  This number may be negative.
   */
  virtual int run(int cycles) = 0;

  /**
   * Replaces the current program counter with \a address.
   */
  virtual void jump(uint16_t address) = 0;

  /** Jumps to the vector of \a intr without further checks. */
  void jumpToVector(Interrupt intr);

  /**
   * Triggers the \a intr.  If \a intr is masked, and \a force is \c false,
   * the interrupt will \b ignored.
   *
   * The method otherwise treats the interrupt like a real one.  It will rescue
   * the current state onto the stack of the guest CPU, and then jump to the
   * interrupt vector.
   */
  void interrupt(Interrupt intr, bool force = false);

  /** Pulls a 8-bit integer from the guest stack. */
  uint8_t pull();

  /** Pulls a 16-bit integer from the guest stack. */
  uint16_t pull16();

  /** Pushes a 8-bit integer onto the guest stack. */
  void push(uint8_t value);

  /** Pushes a 16-bit integer onto the guest stack. */
  void push(uint16_t value);

  /** Returns the memory. */
  Memory::Ptr &mem();

  /**
   * Factory method, instantiates the CPU implementation by \a name.
   *
   * \sa availableImplementations()
   */
  static Base *createByName(const QString &name, const Memory::Ptr &mem, QObject *parent);

  /** Returns the internal state. */
  State &state();

  /** Installed hook. */
  Hook *hook() const;

  /**
   * Installs the \a hook which can supervise executed instructions for
   * debugging purposes.
   *
   * \warning Not all CPU cores support this feature.
   * \note Doesn't take ownership of \a hook.
   */
  void setHook(Hook *hook);

  /**
   * All available implementations.  Maps from the internal identifier name of
   * the core to its human-readable title.
   *
   * \sa createByName()
   */
  static QMap<QString, QString> availableImplementations();

protected:
  Hook *m_hook;
  State m_state;
  Memory::Ptr m_mem;
};
}

#endif // BASE_HPP
