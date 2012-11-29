// TODO: make cursor the fixed point of the zoom
// TODO: compute area for selfintersecting polygons
// TODO: polygon editing: move points, move segments, add points, delete points
// TODO: set scale by two points GPS coordinates
// TODO: use floating-point numbers to store polygon in original coodinates, for exactness (?)
// TODO: save areas measured, allow to set their colors; save & print the result

#include <cassert>
#include <cmath>

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


static inline QColor getFillColor(const QColor& penColor)
{
  QColor fillColor = penColor;
  fillColor.setAlpha(100);
  return fillColor;
}

static inline void setColor(QPainter& painter, const QColor& penColor)
{
  painter.setPen(penColor);
  painter.setBrush(getFillColor(penColor));
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

  acceptableScales_ << 0.01 << 0.015 << 0.02 << 0.025 << 0.03 << 0.04 << 0.05 << 0.06 << 0.07 << 0.08 << 0.09;
  acceptableScales_ << 0.10 << 0.12 << 0.14 << 0.17 << 0.20 << 0.23 << 0.26 << 0.30 << 0.35 << 0.40 << 0.45;
  acceptableScales_ << 0.50 << 0.60 << 0.70 << 0.80 << 0.90;
  acceptableScales_ << 1.00 << 1.25 << 1.50 << 1.75 << 2.00 << 2.50 << 3.00 << 4.00;
  iScale_ = acceptableScales_.indexOf(1.00);

  scrollArea_->viewport()->installEventFilter(this);
  setMouseTracking(true);
  mode_ = DEFAULT_MODE;
  etalonDefinition_ = true;
  etalonDefinedRecently_ = true;
  showRuler_ = false;
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

  QPolygon activePolygon;
  Mode activeMode;
  bool isEtalon;
  PolygonCorrectness correctness;
  getActivePolygon(true, activePolygon, activeMode, isEtalon, correctness);

  if (correctness != VALID_POLYGON) {
    setColor(painter, errorPen_);
  }
  else if (isEtalon) {
    if (polygonFinished_)
      setColor(painter, etalonStaticPen_);
    else
      setColor(painter, etalonActivePen_);
  }
  else {
    if (polygonFinished_)
      setColor(painter, staticPen_);
    else
      setColor(painter, activePen_);
  }

  if (getModeKind(activeMode) == LENGTH)
    painter.drawPolyline(activePolygon);
  else
    painter.drawPolygon(activePolygon);

  if (showRuler_)
    drawRuler(painter, event->rect());
}

void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->buttons() == Qt::LeftButton) {
    QPoint originalNewPoint = event->pos() / scale_;

    if (!etalonDefinition_)
      etalonDefinedRecently_ = false;

    if (polygonFinished_)
      resetPolygon();

    bool polygonFinished = addPoint(originalPolygon_, originalNewPoint, mode_);
    if (polygonFinished)
      finishPlotting();
    else
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
    QPoint scrollBy = scrollStartPoint_ - event->globalPos();
    scrollArea_->horizontalScrollBar()->setValue(scrollStartHValue_ + scrollBy.x());
    scrollArea_->verticalScrollBar()  ->setValue(scrollStartVValue_ + scrollBy.y());
  }
  event->accept();
}

void CanvasWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
  finishPlotting();
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
  resetAll();
}

bool CanvasWidget::isEtalonCorrect() const
{
  return etalonMetersLength_ > 0.;
}


void CanvasWidget::toggleEtalonDefinition(bool isDefiningEtalon)
{
  if (etalonDefinition_ == isDefiningEtalon)
    return;
  etalonDefinition_ = isDefiningEtalon;
  if (etalonDefinition_)
    etalonDefinedRecently_ = true;
  resetAll();
}

void CanvasWidget::toggleRuler(bool showRuler)
{
  if (showRuler_ == showRuler)
    return;
  showRuler_ = showRuler;
  update();
}


void CanvasWidget::getActivePolygon(bool scaled, QPolygon& polygon, Mode& mode, bool& isEtalon, PolygonCorrectness& correctness) const
{
  polygon = originalPolygon_;
  mode = mode_;
  if (!polygonFinished_) {
    addPoint(polygon, originalPointUnderMouse_, mode);
    finishPolygon(polygon, mode);
  }
  isEtalon = etalonDefinedRecently_;
  if (polygon.isEmpty() && isEtalon) {
    polygon = etalonPolygon_;
    mode = etalonPolygonMode_;
  }
  correctness = polygonCorrectness(polygon, mode);
  if (scaled)
    for (QPolygon::Iterator it = polygon.begin(); it != polygon.end(); ++it)
      *it *= scale_;
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
  if (!isEtalonCorrect() || maxLength < rulerMinLength)
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


void CanvasWidget::finishPlotting()
{
  finishPolygon(originalPolygon_, mode_);
  polygonFinished_ = true;

  qDebug(" ");
  foreach (QPoint v, originalPolygon_)
    qDebug("%d, %d", v.x(), v.y());

  if (etalonDefinition_) {
    double originalEtalonPixelLength = 0.;
    QString prompt;
    switch (getModeKind(mode_)) {
      case LENGTH:
        originalEtalonPixelLength = polylineLength(originalPolygon_);
        prompt = QString::fromUtf8("Укажите длину эталона (%1): ").arg(linearUnit);
        break;
      case AREA:
        originalEtalonPixelLength = std::sqrt(polygonArea(originalPolygon_));
        prompt = QString::fromUtf8("Укажите площадь эталона (%1): ").arg(squareUnit);
        break;
    }
    bool ok;
    double etalonMetersSize = QInputDialog::getDouble(this, mainWindow_->appName(), prompt, 1., 0.001, 1e9, 3, &ok);
    if (ok && originalEtalonPixelLength > eps) {
      switch (getModeKind(mode_)) {
        case LENGTH: etalonMetersLength_ = etalonMetersSize;            break;
        case AREA:   etalonMetersLength_ = std::sqrt(etalonMetersSize); break;
      }
      originalMetersPerPixel_ = etalonMetersLength_ / originalEtalonPixelLength;
      metersPerPixel_ = originalMetersPerPixel_ / scale_;
      saveEtalonPolygon();
      mainWindow_->toggleEtalonDefinition(false);
    }
    else
      resetPolygon();
  }

  updateAll();
}

void CanvasWidget::saveEtalonPolygon()
{
  assert(polygonFinished_);
  etalonPolygon_ = originalPolygon_;
  etalonPolygonMode_ = mode_;
}

void CanvasWidget::resetPolygon()
{
  originalPolygon_.clear();
  polygonFinished_ = false;
  if (etalonDefinition_) {
    etalonPolygon_.clear();
    etalonPolygonMode_ = DEFAULT_MODE;
    etalonMetersLength_ = 0.;
    originalMetersPerPixel_ = 0.;
    metersPerPixel_ = 0.;
  }
}

void CanvasWidget::resetAll()
{
  originalPolygon_.clear();
  updateAll();
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
  statusLabel_->clear();
  QPolygon activePolygon;
  Mode activeMode;
  bool isEtalon;
  PolygonCorrectness correctness;
  getActivePolygon(false, activePolygon, activeMode, isEtalon, correctness);

  switch (correctness) {
    case VALID_POLYGON: {
      QString etalonString = isEtalon ? QString::fromUtf8(" эталона") : QString();
      switch (getModeKind(activeMode)) {
        case LENGTH: {
          double length = polylineLength(activePolygon) * originalMetersPerPixel_;
          if (length > eps)
            statusLabel_->setText(QString::fromUtf8("Длина%1: %2 %3").arg(etalonString).arg(length).arg(linearUnit));
          break;
        }
        case AREA: {
          double area = polygonArea(activePolygon) * sqr(originalMetersPerPixel_);
          if (area > eps)
            statusLabel_->setText(QString::fromUtf8("Площадь%1: %2 %3").arg(etalonString).arg(area).arg(squareUnit));
          break;
        }
      }
      break;
    }
    case SELF_INTERSECTING_POLYGON: {
      statusLabel_->setText(QString::fromUtf8("Многоугольник не должен самопересекаться!"));
      break;
    }
  }
}

void CanvasWidget::updateAll()
{
  updateStatusText();
  update();
}
