#include <core/configuration.hpp>

#include <QSettings>
#include <QDebug>
#include <QMetaEnum>

static const QString CONFIG_FILE = "config.ini";
static const QString DEFAULT_CPU = "interpret"; // Is guaranteed to exist.

static const Core::GamepadKeys DEFAULT_KEYS_FIRST(
    Qt::Key_Left,
    Qt::Key_Right,
    Qt::Key_Up,
    Qt::Key_Down,
    Qt::Key_A,
    Qt::Key_S,
    Qt::Key_Return,
    Qt::Key_Space
);

static const Core::GamepadKeys DEFAULT_KEYS_SECOND(
    Qt::Key_4,
    Qt::Key_6,
    Qt::Key_8,
    Qt::Key_5,
    Qt::Key_7,
    Qt::Key_9,
    Qt::Key_Return,
    Qt::Key_Space
);

namespace Core {
struct ConfigurationPrivate : public QSharedData {
  ConfigurationPrivate(const QString &path) {
    this->settings = new QSettings(path, QSettings::IniFormat);
    qDebug() << "Reading configuration from" << this->settings->fileName();
    this->reload();
  }

  ~ConfigurationPrivate() {
    delete this->settings;
  }

  QSettings *settings;

  GamepadKeys first;
  GamepadKeys second;
  QString cpu;

  void reload() {
    this->readInput(this->first, "firstPlayer", DEFAULT_KEYS_FIRST);
    this->readInput(this->second, "secondPlayer", DEFAULT_KEYS_SECOND);

    this->settings->beginGroup("cpu");
    this->cpu = this->settings->value("impl", DEFAULT_CPU).value<QString>();
    this->settings->endGroup();
  }

  void readInput(GamepadKeys &keys, const QString &group, const Core::GamepadKeys &defaults) {
    this->settings->beginGroup(group);
    keys.left = this->readKey("left", defaults.left);
    keys.right = this->readKey("right", defaults.right);
    keys.up = this->readKey("up", defaults.up);
    keys.down = this->readKey("down", defaults.down);
    keys.a = this->readKey("a", defaults.a);
    keys.b = this->readKey("b", defaults.b);
    keys.start = this->readKey("start", defaults.start);
    keys.select = this->readKey("select", defaults.select);
    this->settings->endGroup();
  }

  Qt::Key readKey(const QString &name, Qt::Key defaultKey) {
    static QMetaEnum keyEnum = QMetaEnum::fromType<Qt::Key>();
    QVariant variant = this->settings->value(name, defaultKey);

    if (variant.isValid()) {
      bool ok = false;
      int key = keyEnum.keyToValue(variant.toString().toLatin1(), &ok);

      if (ok) return static_cast<Qt::Key>(key);
      qWarning() << "Key value" << variant.toString() << "for key" << name << "is unknown, using default.";
    }

    // Fall back to default key otherwise.
    return defaultKey;
  }

  void write() {
    this->writeInput(this->first, "firstPlayer");
    this->writeInput(this->second, "secondPlayer");

    this->settings->beginGroup("cpu");
    this->settings->setValue("impl", this->cpu);
    this->settings->endGroup();
  }

  /** Returns the stringified name of \a key. */
  QString keyName(Qt::Key key) {
    static QMetaEnum keyEnum = QMetaEnum::fromType<Qt::Key>();
    return QLatin1String(keyEnum.valueToKey(key));
  }

  void writeInput(GamepadKeys &keys, const QString &group) {
    this->settings->beginGroup(group);
    this->settings->setValue("left", keyName(keys.left));
    this->settings->setValue("right", keyName(keys.right));
    this->settings->setValue("up", keyName(keys.up));
    this->settings->setValue("down", keyName(keys.down));
    this->settings->setValue("a", keyName(keys.a));
    this->settings->setValue("b", keyName(keys.b));
    this->settings->setValue("start", keyName(keys.start));
    this->settings->setValue("select", keyName(keys.select));
    this->settings->endGroup();
  }
};

Configuration::Configuration()
  : d(new ConfigurationPrivate(CONFIG_FILE)) {

}

Configuration::Configuration(const Configuration &rhs)
  : d(rhs.d) {

}

Configuration &Configuration::operator=(const Configuration &rhs) {
  if (this != &rhs)
    d.operator=(rhs.d);
  return *this;
}

Configuration::~Configuration() {
}

QString Configuration::cpuImplementation() const {
  return this->d->cpu;
}

void Configuration::setCpuImplementation(const QString &name) {
  this->d->cpu = name;
}

GamepadKeys Configuration::firstPlayer() const {
  return this->d->first;
}

void Configuration::setFirstPlayer(GamepadKeys keys) {
  this->d->first = keys;
}

GamepadKeys Configuration::secondPlayer() const {
  return this->d->second;
}

void Configuration::setSecondPlayer(GamepadKeys keys) {
  this->d->second = keys;
}

bool Configuration::save() {
  this->d->write();
  return true;
}
}
