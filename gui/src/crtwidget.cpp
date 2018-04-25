#include <crtwidget.hpp>

#include <QPainter>

namespace Gui {
class CrtWidgetPrivate {
public:
  float scale = 2.0;
  QImage display;

  CrtWidgetPrivate() {
    this->display = QImage(256, 240, QImage::Format_ARGB32);
  }
};

CrtWidget::CrtWidget(QWidget *parent) : QWidget(parent) {
  this->d = new CrtWidgetPrivate;
  setScale(2.0);
}

CrtWidget::~CrtWidget() {
  delete this->d;
}

float CrtWidget::scale() const {
  return this->d->scale;
}

void CrtWidget::setScale(float scale) {
  this->d->scale = scale;

  QImage &display = this->d->display;
  this->setFixedSize(display.width() * scale, display.height() * scale);
  this->update();
}

uint32_t *CrtWidget::nextFrameBuffer() {
  throw std::runtime_error("Shouldn't be used atm");
}

void CrtWidget::displayFrameBuffer(uint32_t *buffer) {
  const uchar *data = reinterpret_cast<uchar *>(buffer);
  uchar *destination = this->d->display.bits();
  ::memcpy(destination, data, 256 * 240 * sizeof(uint32_t));

  this->repaint();
}

void CrtWidget::paintEvent(QPaintEvent *) {
  QPainter p(this);

  p.scale(this->d->scale, this->d->scale);
  p.drawImage(0, 0, this->d->display);
}

}
