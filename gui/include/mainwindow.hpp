#ifndef GUI_MAINWINDOW_HPP
#define GUI_MAINWINDOW_HPP

#include <QMainWindow>

namespace Gui {

struct MainWindowPrivate;

/**
 * The main window class, showing the game display and handling user
 * interactions.
 */
class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

  void setWindowTitle(const QString &title);

public slots:
  void openSettings();
  void askOpenRom();
  void openRom(const QString &path);

  void togglePause();
  void resume();
  void halt();
  void reset(bool hard = false);
  void hardReset();

  void setScale(float scale);
  bool eventFilter(QObject *, QEvent *event) override;

private slots:
  void tickFrame();

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;

private:
  bool handleKeyInput(QKeyEvent *event, bool newValue);
  void addMenus();

  MainWindowPrivate *d;
};

}

#endif // GUI_MAINWINDOW_HPP
