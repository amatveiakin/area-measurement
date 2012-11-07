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


static inline QColor getFillColor(QColor penColor)
{
  QColor fillColor = penColor;
  fillColor.setAlpha(100);
  return fillColor;
}


CanvasWidget::CanvasWidget(const QPixmap* image, MainWindow* mainWindow, QScrollArea* scrollArea, QLabel* statusLabel, QWidget* parent) :
  QWidget(parent),
  mainWindow_(mainWindow),
  scrollArea_(scrollArea),
  statusLabel_(statusLabel),
  image_(image)
{
  etalonStaticPen_ = QColor(0, 150, 0);
  etalonActivePen_ = QColor(0, 200, 0);
  staticPen_  = QColor(  0,  50, 240);
  activePen_  = QColor(  0, 100, 240);
  errorPen_   = QColor(255,   0,   0);
  staticFill_ = getFillColor(staticPen_);
  activeFill_ = getFillColor(activePen_);
  errorFill_  = getFillColor(errorPen_);

  setFixedSize(image_->size());
  setMouseTracking(true);
  mode_ = SET_ETALON;
  resetEtalon();
  resetPolygon();
}

CanvasWidget::~CanvasWidget()
{
  delete image_;
}


void CanvasWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.drawPixmap(event->rect().topLeft() , *image_, event->rect());

  if (mode_ == SET_ETALON) {
    if (nEthalonPointsSet_ == 2) {
      painter.setPen(etalonStaticPen_);
      painter.drawLine(etalon_);
    }
    else if (nEthalonPointsSet_ == 1) {
      painter.setPen(etalonActivePen_);
      painter.drawLine(etalon_.p1(), pointUnderMouse_);
    }
  }
  else {
    QPolygon activePolygon = getActivePolygon();
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

  drawRuler(painter, event->rect());
}

void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->buttons() == Qt::LeftButton) {
    QPoint newPoint = event->pos();

    if (polygonFinished_)
      resetPolygon();

    if (mode_ == SET_ETALON) {
      if (nEthalonPointsSet_ >= 2)
        resetEtalon();

      if (nEthalonPointsSet_ == 0) {
        etalon_.setP1(newPoint);
        nEthalonPointsSet_ = 1;
      }
      else if (nEthalonPointsSet_ == 1) {
        bool ok;
        etalon_.setP2(newPoint);
        etalonLength_ = QInputDialog::getDouble(this, mainWindow_->appName(), QString::fromUtf8("Укажите длину эталона (%1): ").arg(linearUnit), 1., 0.001, 1e9, 3, &ok);
        double etalonPixelLength_ = segmentLenght(etalon_.p1(), etalon_.p2());
        if (ok && etalonPixelLength_ > eps) {
          metersPerPixel_ = etalonLength_ / etalonPixelLength_;
          nEthalonPointsSet_ = 2;
          mainWindow_->setMeasurementEnabled(true);
        }
        else
          resetEtalon();
      }
    }
    else {
      polygonFinished_ = addPoint(polygon_, newPoint, mode_);
      if (polygonFinished_)
        finishPolygon(polygon_, mode_);
    }
    updateAll();
  }
  else if (event->buttons() == Qt::RightButton) {
    scrollStartPoint_ = event->globalPos();
    scrollStartHValue_ = scrollArea_->horizontalScrollBar()->value();
    scrollStartVValue_ = scrollArea_->verticalScrollBar()  ->value();
  }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() == Qt::NoButton) {
    pointUnderMouse_ = event->pos();
    updateAll();
  }
  else if (event->buttons() == Qt::RightButton) {
    QPoint scrollBy = event->globalPos() - scrollStartPoint_;
    scrollArea_->horizontalScrollBar()->setValue(scrollStartHValue_ - scrollBy.x());
    scrollArea_->verticalScrollBar()  ->setValue(scrollStartVValue_ - scrollBy.y());
  }
}


void CanvasWidget::setMode(Mode newMode)
{
  mode_ = newMode;
  polygon_.clear();
  updateAll();
}


QPolygon CanvasWidget::getActivePolygon() const
{
  QPolygon activePolygon = polygon_;
  if (!polygonFinished_) {
    addPoint(activePolygon, pointUnderMouse_, mode_);
    finishPolygon(activePolygon, mode_);
  }
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
  while (true) {
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
  ruler.append(QRect(rulerLeft + pixelLength        , rulerY - rulerSerifsSize / 2, rulerThickness, rulerSerifsSize));
  drawFramed(painter, ruler, rulerFrameThickness, rulerBodyColor, rulerFrameColor);

  QString rulerLabel = QString::number(metersLength) + linearUnit;
  QRect labelRect = painter.fontMetrics().boundingRect(rulerLabel);
  QPoint labelPos(rulerLeft + rulerFrameThickness + rulerTextMargin, rulerY - rulerThickness / 2 - rulerFrameThickness - rulerTextMargin - painter.fontMetrics().descent());
  painter.fillRect(labelRect.translated(labelPos), rulerFrameColor);
  painter.setPen(rulerBodyColor);
  painter.drawText(labelPos, rulerLabel);
}


void CanvasWidget::resetEtalon()
{
  nEthalonPointsSet_ = 0;
  etalon_ = QLine();
  etalonLength_ = 0.;
  metersPerPixel_ = 0.;
  mainWindow_->setMeasurementEnabled(false);
}

void CanvasWidget::resetPolygon()
{
  polygon_.clear();
  polygonFinished_ = false;
}



void CanvasWidget::updateStatusText()
{
  switch (getModeKind(mode_)) {
    case ETALON: {
      statusLabel_->setText(etalonLength_ > eps ? QString::fromUtf8("Длина эталона: %1 %2").arg(etalonLength_).arg(linearUnit) : QString());
      break;
    }
    case LENGTH: {
      double length = polylineLenght(getActivePolygon()) * metersPerPixel_;
      statusLabel_->setText(length > eps ? QString::fromUtf8("Длина: %1 %2").arg(length).arg(linearUnit) : QString());
      break;
    }
    case AREA: {
      QPolygon activePolygon = getActivePolygon();
      if (isValidPolygon(activePolygon, mode_)) {
        double area = polygonArea(activePolygon) * sqr(metersPerPixel_);
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
