#include <QInputDialog>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QScrollArea>
#include <QScrollBar>

#include "canvaswidget.h"
#include "mainwindow.h"
#include "polygon_math.h"


const int rulerMargin         = 16;
const int rulerThickness      = 2;
const int rulerFrameThickness = 1;
const int rulerTextMargin     = 3;
const int rulerSerifsSize     = 8;
const int rulerMaxLength      = 180;
const int rulerMinLength      = 40;
const QColor rulerBodyColor   = Qt::black;
const QColor rulerFrameColor  = Qt::white;

const QString linearUnit = QString::fromUtf8("м");
const QString squareUnit = linearUnit + QString::fromUtf8("²");

const int maxImageSize = 4096;


static inline QColor getFillColor(QColor penColor)
{
  QColor fillColor = penColor;
  fillColor.setAlpha(100);
  return fillColor;
}


CanvasWidget::CanvasWidget(const QPixmap* image, MainWindow* mainWindow, QScrollArea* scrollArea, QLabel* scaleLabel, QLabel* statusLabel, QWidget* parent) :
  QWidget(parent),
  mainWindow_(mainWindow),
  scrollArea_(scrollArea),
  scaleLabel_(scaleLabel),
  statusLabel_(statusLabel),
  originalImage_(image)
{
  etalonStaticPen_ = QColor(0, 150, 0);
  etalonActivePen_ = QColor(0, 200, 0);
  staticPen_  = QColor(  0,  50, 240);
  activePen_  = QColor(  0, 100, 240);
  errorPen_   = QColor(255,   0,   0);
  staticFill_ = getFillColor(staticPen_);
  activeFill_ = getFillColor(activePen_);
  errorFill_  = getFillColor(errorPen_);

  acceptableScales_ << 0.05 << 0.07 << 0.1 << 0.17 << 0.25 << 0.33 << 0.5 << 0.67 << 1. << 1.5 << 2. << 3. << 4.;
  iScale_ = 8;

  scrollArea_->viewport()->installEventFilter(this);
  setMouseTracking(true);
  mode_ = SET_ETALON;
  showRuler_ = false;
  resetEtalon();
  resetPolygon();
  scaleChanged();
}

CanvasWidget::~CanvasWidget()
{
  delete originalImage_;
}


void CanvasWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.drawPixmap(event->rect().topLeft() , image_, event->rect());

  if (mode_ == SET_ETALON) {
    if (nEthalonPointsSet_ == 2) {
      painter.setPen(etalonStaticPen_);
      painter.drawLine(originalEtalon_.p1() * scale_, originalEtalon_.p2() * scale_);
    }
    else if (nEthalonPointsSet_ == 1) {
      painter.setPen(etalonActivePen_);
      painter.drawLine(originalEtalon_.p1() * scale_, originalPointUnderMouse_ * scale_);
    }
  }
  else {
    QPolygon activePolygon = getActivePolygon(true);
    if (!isValidPolygon(activePolygon, mode_)) {
      painter.setPen(errorPen_);
      painter.setBrush(errorFill_);
    }
    else if (polygonFinished_) {
      painter.setPen(staticPen_);
      painter.setBrush(staticFill_);
    }
    else {
      painter.setPen(activePen_);
      painter.setBrush(activeFill_);
    }
    if (getModeKind(mode_) == LENGTH)
      painter.drawPolyline(activePolygon);
    else
      painter.drawPolygon(activePolygon);
  }

  if (showRuler_)
    drawRuler(painter, event->rect());
}

void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->buttons() == Qt::LeftButton) {
    QPoint originalNewPoint = event->pos() / scale_;

    if (polygonFinished_)
      resetPolygon();

    if (mode_ == SET_ETALON) {
      if (nEthalonPointsSet_ >= 2)
        resetEtalon();

      if (nEthalonPointsSet_ == 0) {
        originalEtalon_.setP1(originalNewPoint);
        nEthalonPointsSet_ = 1;
      }
      else if (nEthalonPointsSet_ == 1) {
        bool ok;
        originalEtalon_.setP2(originalNewPoint);
        etalonMetersLength_ = QInputDialog::getDouble(this, mainWindow_->appName(), QString::fromUtf8("Укажите длину эталона (%1): ").arg(linearUnit), 1., 0.001, 1e9, 3, &ok);
        double originalEtalonPixelLength_ = segmentLenght(originalEtalon_.p1(), originalEtalon_.p2());
        if (ok && originalEtalonPixelLength_ > eps) {
          originalMetersPerPixel_ = etalonMetersLength_ / originalEtalonPixelLength_;
          metersPerPixel_ = originalMetersPerPixel_ / scale_;
          nEthalonPointsSet_ = 2;
          mainWindow_->setMeasurementEnabled(true);
        }
        else
          resetEtalon();
      }
    }
    else {
      polygonFinished_ = addPoint(originalPolygon_, originalNewPoint, mode_);
      if (polygonFinished_)
        finishPolygon(originalPolygon_, mode_);
    }
    updateAll();
  }
  else if (event->buttons() == Qt::RightButton) {
    scrollStartPoint_ = event->globalPos();
    scrollStartHValue_ = scrollArea_->horizontalScrollBar()->value();
    scrollStartVValue_ = scrollArea_->verticalScrollBar()  ->value();
  }
  event->accept();
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() == Qt::NoButton) {
    originalPointUnderMouse_ = event->pos() / scale_;
    updateAll();
  }
  else if (event->buttons() == Qt::RightButton) {
    QPoint scrollBy = event->globalPos() - scrollStartPoint_;
    scrollArea_->horizontalScrollBar()->setValue(scrollStartHValue_ - scrollBy.x());
    scrollArea_->verticalScrollBar()  ->setValue(scrollStartVValue_ - scrollBy.y());
  }
  event->accept();
}

bool CanvasWidget::eventFilter(QObject* object, QEvent* event__)
{
  if (object == scrollArea_->viewport() && event__->type() == QEvent::Wheel) {
    QWheelEvent* event = static_cast<QWheelEvent*>(event__);
    int numSteps = event->delta() / 120;
    iScale_ = qBound(0, iScale_ + numSteps, acceptableScales_.size() - 1);
    int size = qMax(originalImage_->width(), originalImage_->height());
    while (size * acceptableScales_[iScale_] > maxImageSize && acceptableScales_[iScale_] > 1.)
      iScale_--;
    scaleChanged();
    updateAll();
    return true;
  }
  return false;
}



void CanvasWidget::setMode(Mode newMode)
{
  mode_ = newMode;
  originalPolygon_.clear();
  updateAll();
}


void CanvasWidget::toggleRuler(bool showRuler)
{
  showRuler_ = showRuler;
  update();
}


QPolygon CanvasWidget::getActivePolygon(bool scaled) const
{
  QPolygon activePolygon = originalPolygon_;
  if (!polygonFinished_) {
    addPoint(activePolygon, originalPointUnderMouse_, mode_);
    finishPolygon(activePolygon, mode_);
  }
  if (scaled)
    for (QPolygon::Iterator it = activePolygon.begin(); it != activePolygon.end(); ++it)
      *it *= scale_;
  return activePolygon;
}


void CanvasWidget::drawFramed(QPainter& painter, const QList<QRect>& objects, int frameThickness, const QColor& objectsColor, const QColor& frameColor)
{
  foreach (const QRect& rect, objects)
    painter.fillRect(rect.adjusted(-frameThickness, -frameThickness, frameThickness, frameThickness), frameColor);
  foreach (const QRect& rect, objects)
    painter.fillRect(rect, objectsColor);
}

void CanvasWidget::drawRuler(QPainter& painter, const QRect& rect)
{
  int maxLength = qMin(rulerMaxLength, rect.width() - 2 * rulerMargin);
  if (nEthalonPointsSet_ != 2 || maxLength < rulerMinLength)
    return;

  double pixelLengthF;
  double metersLength;
  double base = 1e10;
  while (base > 1e-10) {
    if ((pixelLengthF = (metersLength = base * 5.) / metersPerPixel_) < maxLength)
      break;
    if ((pixelLengthF = (metersLength = base * 2.) / metersPerPixel_) < maxLength)
      break;
    if ((pixelLengthF = (metersLength = base * 1.) / metersPerPixel_) < maxLength)
      break;
    base /= 10.;
  }
  int pixelLength = pixelLengthF;

  QList<QRect> ruler;
  int rulerLeft = rect.left()   + rulerMargin;
  int rulerY    = rect.bottom() - rulerMargin;
  ruler.append(QRect(rulerLeft, rulerY - rulerThickness / 2, pixelLength, rulerThickness));
  ruler.append(QRect(rulerLeft - rulerThickness, rulerY - rulerSerifsSize / 2, rulerThickness, rulerSerifsSize));
  ruler.append(QRect(rulerLeft + pixelLength   , rulerY - rulerSerifsSize / 2, rulerThickness, rulerSerifsSize));
  drawFramed(painter, ruler, rulerFrameThickness, rulerBodyColor, rulerFrameColor);

  QString rulerLabel = QString::number(metersLength) + " " + linearUnit;
  QRect labelRect = painter.fontMetrics().boundingRect(rulerLabel);
  QPoint labelPos(rulerLeft + rulerFrameThickness + rulerTextMargin, rulerY - rulerThickness / 2 - rulerFrameThickness - rulerTextMargin - painter.fontMetrics().descent());
  painter.fillRect(labelRect.translated(labelPos), rulerFrameColor);
  painter.setPen(rulerBodyColor);
  painter.drawText(labelPos, rulerLabel);
}


void CanvasWidget::resetEtalon()
{
  nEthalonPointsSet_ = 0;
  originalEtalon_ = QLine();
  etalonMetersLength_ = 0.;
  originalMetersPerPixel_ = 0.;
  metersPerPixel_ = 0.;
  mainWindow_->setMeasurementEnabled(false);
}

void CanvasWidget::resetPolygon()
{
  originalPolygon_.clear();
  polygonFinished_ = false;
}

void CanvasWidget::scaleChanged()
{
  scale_ = acceptableScales_[iScale_];
  image_ = originalImage_->scaled(originalImage_->size() * scale_, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  metersPerPixel_ = originalMetersPerPixel_ / scale_;
  setFixedSize(image_.size());
  scaleLabel_->setText(QString::number(scale_ * 100.) + "%");
}



void CanvasWidget::updateStatusText()
{
  switch (getModeKind(mode_)) {
    case ETALON: {
      statusLabel_->setText(etalonMetersLength_ > eps ? QString::fromUtf8("Длина эталона: %1 %2").arg(etalonMetersLength_).arg(linearUnit) : QString());
      break;
    }
    case LENGTH: {
      double length = polylineLength(getActivePolygon(false)) * originalMetersPerPixel_;
      statusLabel_->setText(length > eps ? QString::fromUtf8("Длина: %1 %2").arg(length).arg(linearUnit) : QString());
      break;
    }
    case AREA: {
      QPolygon activePolygon = getActivePolygon(false);
      if (isValidPolygon(activePolygon, mode_)) {
        double area = polygonArea(activePolygon) * sqr(originalMetersPerPixel_);
        statusLabel_->setText(area > eps ? QString::fromUtf8("Площадь: %1 %2").arg(area).arg(squareUnit) : QString());
      }
      else {
        statusLabel_->setText(QString::fromUtf8("Многоугольник не должен самопересекаться!"));
      }
      break;
    }
  }
}

void CanvasWidget::updateAll()
{
  updateStatusText();
  update();
}
