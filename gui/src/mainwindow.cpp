#include <crtwidget.hpp>
#include <mainwindow.hpp>
#include <settingsdialog.hpp>

#include <core/configuration.hpp>
#include <core/runner.hpp>

#include <QTimer>
#include <QKeyEvent>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>

#include <ppu/memory.hpp>

#include <cpu/base.hpp>
#include <cpu/memory.hpp>

namespace Gui {

/**
 * Displays a critical message box showing the \a error.  The user can then
 * choose to abort the proces (Which is done so immediately), or to ignore
 * the error.
 */
static void showException(const std::exception &error) {
  QString message = MainWindow::tr("A runtime error has occured:\n%1").arg(QLatin1String(error.what()));
  QMessageBox::critical(nullptr, MainWindow::tr("Runtime error"), message, QMessageBox::Abort);

  abort();
}

struct MainWindowPrivate {
  Core::Configuration config;

  CrtWidget *crt;
  Core::Runner *runner = nullptr;
  QTimer *frameTicker;
  QTimer *fpsTicker;
  QLabel *fpsLabel;
  QLabel *coreLabel;

  QAction *resumeAction;
  QAction *pauseAction;
  QAction *softResetAction;
  QAction *hardResetAction;

  int tickCount = 0;

  void updateActionState() {
    bool canRun = this->runner;
    bool isRunning = this->frameTicker->isActive();

    this->resumeAction->setEnabled(canRun && !isRunning);
    this->pauseAction->setEnabled(canRun && isRunning);
    this->softResetAction->setEnabled(canRun);
    this->hardResetAction->setEnabled(canRun);
  }
};

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent), d(new MainWindowPrivate)
{
  this->d->crt = new CrtWidget;

  this->d->frameTicker = new QTimer(this);
  this->d->fpsTicker = new QTimer(this);
  this->d->coreLabel = new QLabel;
  this->d->fpsLabel = new QLabel;

  // Qts timer is only millisecond-accurate.  17ms wukk get us 59-ish FPS, which
  // is better than the 62-ish FPS a value of 16ms would give.
  this->d->frameTicker->setInterval(/* 1000 / 60 = 16.66 */ 17);
  this->d->fpsTicker->start(1000);

  this->statusBar()->addPermanentWidget(this->d->fpsLabel);
  this->statusBar()->addPermanentWidget(this->d->coreLabel);

  connect(this->d->frameTicker, &QTimer::timeout, this, &MainWindow::tickFrame);

  connect(this->d->fpsTicker, &QTimer::timeout, [this]{
    this->d->fpsLabel->setText(QStringLiteral("%1 FPS").arg(this->d->tickCount));
    this->d->tickCount = 0;
  });

  this->setCentralWidget(this->d->crt);

  this->addMenus();
  this->d->updateActionState();
  this->setWindowTitle(QString());
  this->menuBar()->installEventFilter(this);
}

MainWindow::~MainWindow() {
  delete this->d;
}

void MainWindow::setWindowTitle(const QString &title) {
  if (title.isEmpty())
    QMainWindow::setWindowTitle(tr("Dynes"));
  else
    QMainWindow::setWindowTitle(tr("%1 – Dynes").arg(title));
}

/** RAII structure to temporarily halt the emulation. */
struct TemporaryHalt {
  bool restart;
  QTimer *ticker;

  TemporaryHalt(QTimer *t) : ticker(t) {
    this->restart = this->ticker->isActive();
    this->ticker->stop();
  }

  ~TemporaryHalt() {
    if (this->restart) this->ticker->start();
  }

  void release() { this->restart = false; }
};

void MainWindow::openSettings() {
  TemporaryHalt halter(this->d->frameTicker);

  SettingsDialog settings(this->d->config, this);
  settings.exec();

  (void)halter;
}

void MainWindow::askOpenRom() {
  TemporaryHalt halter(this->d->frameTicker);
  QString path = QFileDialog::getOpenFileName(this, tr("NES ROM"), QString(), tr("NES ROMs  (*.nes)"));

  if (!path.isEmpty()) {
    halter.release();
    this->openRom(path);
  }
}

void MainWindow::openRom(const QString &path) {
  this->halt();
  delete this->d->runner;
  this->d->runner = nullptr;

  QString cpuCore = this->d->config.cpuImplementation();

  try { // This can fail!
    Core::InesFile ines = Core::InesFile::load(path);
    this->d->runner = new Core::Runner(ines, cpuCore, this->d->crt, this);
  } catch (std::runtime_error error) {
    this->setWindowTitle(QString());
    this->d->updateActionState();
    this->statusBar()->showMessage(error.what(), 10000);
    return;
  }

  this->setWindowTitle(QFileInfo(path).baseName());
  this->d->coreLabel->setText(cpuCore);
  this->reset(true);
}

void MainWindow::togglePause() {
  if (!this->d->runner) return;

  if (this->d->frameTicker->isActive())
    this->halt();
  else
    this->resume();
}

void MainWindow::resume() {
  if (!this->d->runner) return;

  this->d->tickCount = 0;
  this->d->fpsLabel->setText(QString());
  this->d->frameTicker->start();
  this->d->fpsTicker->start();
  this->d->updateActionState();
}

void MainWindow::halt() {
  this->d->fpsTicker->stop();
  this->d->frameTicker->stop();
  this->d->fpsLabel->setText(tr("Paused"));
  this->d->updateActionState();
}

void MainWindow::reset(bool hard) {
  if (this->d->runner) {
    this->d->runner->reset(hard);
    this->resume();
  }
}

void MainWindow::hardReset() {
  this->reset(true);
}

void MainWindow::setScale(float scale) {
  this->d->crt->setScale(scale);
  QTimer::singleShot(0, [this]{ this->resize(0, 0); });
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
  if (event->isAutoRepeat()) return;

  if (!this->handleKeyInput(event, true))
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
  if (event->isAutoRepeat()) return;

  if (!this->handleKeyInput(event, false))
    QMainWindow::keyReleaseEvent(event);
}

static bool handleGamepadInput(int key, bool newValue, const Core::GamepadKeys &keys, Core::Gamepad &pad) {
  if (key == keys.up) pad.setUp(newValue);
  else if (key == keys.down) pad.setDown(newValue);
  else if (key == keys.left) pad.setLeft(newValue);
  else if (key == keys.right) pad.setRight(newValue);
  else if (key == keys.a) pad.setA(newValue);
  else if (key == keys.b) pad.setB(newValue);
  else if (key == keys.start) pad.setStart(newValue);
  else if (key == keys.select) pad.setSelect(newValue);
  else return false;

  return true;
}

bool MainWindow::handleKeyInput(QKeyEvent *event, bool newValue) {
  if (!this->d->runner) return false;

  Core::GamepadKeys first = this->d->config.firstPlayer();
  Core::GamepadKeys second = this->d->config.secondPlayer();
  Core::Gamepad &firstPad = this->d->runner->ram()->firstPlayer();
  Core::Gamepad &secondPad = this->d->runner->ram()->secondPlayer();

  bool handled = handleGamepadInput(event->key(), newValue, first, firstPad);
  handled = handled || handleGamepadInput(event->key(), newValue, second, secondPad);

  // Check for the F2 key to toggle halt/resume.
  if (!handled && event->key() == Qt::Key_Pause) {
    if (newValue) // Only trigger on key down
      this->togglePause();
    handled = true;
  }

  if (handled) event->accept();

  return handled;
}

void MainWindow::addMenus() {
  QMenu *fileMenu = this->menuBar()->addMenu("&Datei");
  fileMenu->addAction(tr("Open ROM"), this, SLOT(askOpenRom()));
  fileMenu->addAction(tr("Settings"), this, SLOT(openSettings()));
  fileMenu->addSeparator();
  fileMenu->addAction(tr("Quit"), QApplication::instance(), SLOT(quit()));

  QMenu *emuMenu = this->menuBar()->addMenu("&Emulation");
  this->d->resumeAction = emuMenu->addAction(tr("Resume"), this, SLOT(resume()));
  this->d->pauseAction = emuMenu->addAction(tr("Pause"), this, SLOT(halt()));
  this->d->softResetAction = emuMenu->addAction(tr("Restart"), this, SLOT(reset()));
  this->d->hardResetAction = emuMenu->addAction(tr("Cold restart"), this, SLOT(reset()));

  QMenu *scaleMenu = this->menuBar()->addMenu("&Skalierung");
  scaleMenu->addAction(tr("1x"), [this]{ this->setScale(1.0); });
  scaleMenu->addAction(tr("2x"), [this]{ this->setScale(2.0); });
  scaleMenu->addAction(tr("3x"), [this]{ this->setScale(3.0); });
  scaleMenu->addAction(tr("4x"), [this]{ this->setScale(4.0); });
}

void MainWindow::focusInEvent(QFocusEvent *event) {
  // Keyboard input should always go into the CRT.  The widget will bounce key
  // events back into this class, where we handle it.  Otherwise, putting the
  // focus on e.g. the menu bar causes a conflict.
  this->d->crt->grabKeyboard();
  QMainWindow::focusInEvent(event);
}

void MainWindow::focusOutEvent(QFocusEvent *event) {
  this->d->crt->releaseKeyboard();
  QMainWindow::focusOutEvent(event);
}

bool MainWindow::eventFilter(QObject *, QEvent *event) {
  if (event->type() == QEvent::KeyPress) this->keyPressEvent(static_cast<QKeyEvent *>(event));
  else if (event->type() == QEvent::KeyRelease) this->keyReleaseEvent(static_cast<QKeyEvent *>(event));
  else return false; // Leave non-key events alone.

  // If the key event was handled, hide it from the receiver.
  return event->isAccepted();
}

void MainWindow::tickFrame() {
  this->d->tickCount++;

  try {
    this->d->runner->tick();
  } catch (std::runtime_error error) {
    showException(error);
  }
}
}
