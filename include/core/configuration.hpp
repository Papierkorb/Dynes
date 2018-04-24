#ifndef CORE_CONFIGURATION_HPP
#define CORE_CONFIGURATION_HPP

#include <QSharedDataPointer>
#include <dynarec/configuration.hpp>

namespace Core {
class ConfigurationPrivate;

/** Gamepad input keys. */
struct GamepadKeys {
  GamepadKeys(Qt::Key left, Qt::Key right, Qt::Key up, Qt::Key down, Qt::Key a, Qt::Key b, Qt::Key start, Qt::Key select)
    : left(left), right(right), up(up), down(down), a(a), b(b), start(start), select(select)
  {
  }

  GamepadKeys()
    : left(Qt::Key_unknown), right(Qt::Key_unknown), up(Qt::Key_unknown), down(Qt::Key_unknown),
      a(Qt::Key_unknown), b(Qt::Key_unknown), start(Qt::Key_unknown), select(Qt::Key_unknown)
  {
  }

  Qt::Key left;
  Qt::Key right;
  Qt::Key up;
  Qt::Key down;
  Qt::Key a;
  Qt::Key b;
  Qt::Key start;
  Qt::Key select;
};

/**
 * Reads and writes configuration data.
 */
class Configuration {
public:
  Configuration();
  Configuration(const Configuration &);
  Configuration &operator=(const Configuration &);
  ~Configuration();

  /** Name of the CPU implementation */
  QString cpuImplementation() const;
  void setCpuImplementation(const QString &name);

  /** Key configuration for the first player. */
  GamepadKeys firstPlayer() const;
  void setFirstPlayer(GamepadKeys keys);

  /** Key configuration for the second player. */
  GamepadKeys secondPlayer() const;
  void setSecondPlayer(GamepadKeys keys);

  /** Saves the current configuration back. */
  bool save();

private:
  QSharedDataPointer<ConfigurationPrivate> d;
};
}

#endif // CORE_CONFIGURATION_HPP
