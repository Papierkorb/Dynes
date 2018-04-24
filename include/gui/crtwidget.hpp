#ifndef GUI_CRTWIDGET_HPP
#define GUI_CRTWIDGET_HPP

#include <QWidget>

#include <ppu/surfacemanager.hpp>

namespace Gui {
class CrtWidgetPrivate;

/**
 * Displays a single frame as given by the back-end.
 *
 * \sa Ppu::SurfaceManager
 * \sa Core::Runner
 */
class CrtWidget : public QWidget, public Ppu::SurfaceManager {
  Q_OBJECT
public:
  explicit CrtWidget(QWidget *parent = nullptr);
  ~CrtWidget() override;

  /** Scaling multiplier for the displayed picture. */
  float scale() const;
  void setScale(float scale);

  virtual uint32_t *nextFrameBuffer() override;
  virtual void displayFrameBuffer(uint32_t *buffer) override;

signals:

public slots:

protected:
  void paintEvent(QPaintEvent *) override;

private:
  CrtWidgetPrivate *d;

};
}

#endif // GUI_CRTWIDGET_HPP
