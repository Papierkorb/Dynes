#ifndef GUI_SETTINGSDIALOG_HPP
#define GUI_SETTINGSDIALOG_HPP

#include <QDialog>

namespace Core { class Configuration; }

namespace Gui {
struct SettingsDialogPrivate;

/**
 * The settings dialog allows the user to re-configure the emulator from within
 * the GUI.
 */
class SettingsDialog : public QDialog {
  Q_OBJECT
public:
  explicit SettingsDialog(Core::Configuration &config, QWidget *parent = nullptr);
  ~SettingsDialog() override;

signals:

public slots:
  void accept() override;

private:
  SettingsDialogPrivate *d;
};
}

#endif // GUI_SETTINGSDIALOG_HPP
