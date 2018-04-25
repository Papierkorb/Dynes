#include <settingsdialog.hpp>

#include <functional>

#include <core/configuration.hpp>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMetaEnum>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QComboBox>

#include <cpu/base.hpp>

namespace Gui {
struct KeyInputButton : public QPushButton {
  enum State { Idle, Changing };

  typedef std::function<void(Qt::Key)> Callback;

  KeyInputButton(Qt::Key key, Callback cb) : m_callback(cb), m_key(key) {
    this->setKey(key);
    this->setCheckable(true);

    connect(this, &QPushButton::clicked, this, &KeyInputButton::handleClick);
  }

  void setKey(Qt::Key key) {
    QMetaEnum meta = QMetaEnum::fromType<Qt::Key>(); // Map enum to string
    QString name = QLatin1String(meta.valueToKey(key));

    if (name.startsWith("Key_")) name = name.mid(4);
    this->setText(name);
    this->m_key = key;
  }

  void keyPressEvent(QKeyEvent *evt) override {
    if (this->m_state == Idle) return QPushButton::keyPressEvent(evt);

    this->m_state = Idle;

    Qt::Key key = static_cast<Qt::Key>(evt->key());
    if (key != Qt::Key_Escape) { // Cancelled?
      this->m_key = key;
      this->m_callback(key);
    }

    this->setChecked(false);
    this->setKey(this->m_key);
  }

  void focusOutEvent(QFocusEvent *evt) override {
    this->cancelChange();
    QPushButton::focusOutEvent(evt);
  }

  void cancelChange() {
    if (this->m_state == Changing) {
      this->m_state = Idle;
      this->setKey(this->m_key);
      this->setChecked(false);
    }
  }

private:
  Callback m_callback;
  State m_state = Idle;
  Qt::Key m_key;

  void handleClick() {

    if (this->m_state == Idle) {
      this->setText(QStringLiteral("..."));
      this->m_state = Changing;
      this->setChecked(true);
    } else {
      this->cancelChange();
    }
  }

};

struct InputTab : public QWidget {
  Core::Configuration &m_config;
  Core::GamepadKeys m_first;
  Core::GamepadKeys m_second;

  InputTab(Core::Configuration &config)
    : m_config(config), m_first(config.firstPlayer()), m_second(config.secondPlayer())
  {
    QHBoxLayout *layout = new QHBoxLayout;

    layout->addLayout(this->keyChangeLayout(tr("Player 1"), this->m_first));
    layout->addLayout(this->keyChangeLayout(tr("Player 2"), this->m_second));

    this->setLayout(layout);
  }

  void save() {
    this->m_config.setFirstPlayer(this->m_first);
    this->m_config.setSecondPlayer(this->m_second);
  }

  QLayout *keyChangeLayout(const QString &title, Core::GamepadKeys &pad) {
    QFormLayout *layout = new QFormLayout;

    QLabel *label = new QLabel(title);
    QFont font = label->font();
    font.setBold(true);
    label->setFont(font);

    layout->addRow(label);
    this->addKeyChangeButtons(layout, pad);

    return layout;
  }

  void addKeyChangeButtons(QFormLayout *layout, Core::GamepadKeys &pad) {
    layout->addRow(tr("Up"), this->keyChangeButton(pad.up));
    layout->addRow(tr("Down"), this->keyChangeButton(pad.down));
    layout->addRow(tr("Left"), this->keyChangeButton(pad.left));
    layout->addRow(tr("Right"), this->keyChangeButton(pad.right));
    layout->addRow(tr("A"), this->keyChangeButton(pad.a));
    layout->addRow(tr("B"), this->keyChangeButton(pad.b));
    layout->addRow(tr("Start"), this->keyChangeButton(pad.start));
    layout->addRow(tr("Select"), this->keyChangeButton(pad.select));
  }

  KeyInputButton *keyChangeButton(Qt::Key &key) {
    return new KeyInputButton(key, [&key](Qt::Key k){ key = k; });
  }
};

struct CpuTab : public QWidget {
  Core::Configuration &m_config;
  QComboBox *m_chooser;

  CpuTab(Core::Configuration &config) : m_config(config) {
    QVBoxLayout *layout = new QVBoxLayout;
    this->m_chooser = new QComboBox;

    QLabel *infoLabel = new QLabel(SettingsDialog::tr("Changing this requires a restart of the game to take effect."));
    QFont font = infoLabel->font();
    font.setItalic(true);
    infoLabel->setFont(font);

    layout->addWidget(this->m_chooser);
    layout->addWidget(infoLabel);
    layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
    this->setLayout(layout);

    // Fill combo box with options
    QMap<QString, QString> cores = Cpu::Base::availableImplementations();
    QString current = this->m_config.cpuImplementation();

    int idx = 0;
    for (auto it = cores.constBegin(); it != cores.constEnd(); ++it, idx++) {
      this->m_chooser->addItem(it.value(), it.key());
      if (current == it.key()) this->m_chooser->setCurrentIndex(idx);
    }
  }

  void save() {
    this->m_config.setCpuImplementation(this->m_chooser->currentData().value<QString>());
  }
};

struct SettingsDialogPrivate {
  SettingsDialogPrivate(Core::Configuration &c) : config(c) { }

  Core::Configuration &config;

  InputTab *inputTab;
  CpuTab *cpuTab;
};

SettingsDialog::SettingsDialog(Core::Configuration &config, QWidget *parent)
  : QDialog(parent), d(new SettingsDialogPrivate(config))
{
  QVBoxLayout *layout = new QVBoxLayout;
  QTabWidget *tabs = new QTabWidget;

  this->setModal(true);

  layout->addWidget(tabs);
  this->setLayout(layout);

  this->d->inputTab = new InputTab(config);
  this->d->cpuTab = new CpuTab(config);
  tabs->addTab(this->d->inputTab, tr("Controls"));
  tabs->addTab(this->d->cpuTab, tr("CPU"));

  // Add [Discard] [OK] buttons at the bottom.
  QDialogButtonBox *box = new QDialogButtonBox(
    QDialogButtonBox::Cancel | QDialogButtonBox::Ok,
    Qt::Horizontal
  );

  connect(box, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
  connect(box, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);

  layout->addWidget(box);
}

SettingsDialog::~SettingsDialog() {
  delete this->d;
}

void SettingsDialog::accept() {
  this->d->inputTab->save();
  this->d->cpuTab->save();

  this->d->config.save();

  QDialog::accept();
}
}
