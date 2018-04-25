#include <QApplication>
#include <mainwindow.hpp>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  Gui::MainWindow mainWindow;
  mainWindow.show();

  if (app.arguments().size() > 1) {
    mainWindow.openRom(app.arguments().at(1));
  }

  return app.exec();
}
