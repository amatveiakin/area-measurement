// TODO: make cursor the fixed point of the zoom
// TODO: compute area for selfintersecting polygons
// TODO: polygon editing: move points, move segments, add points, delete points, move caption, change color
// TODO: set scale by two points GPS coordinates
// TODO: result printing

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
#include "paint_utils.h"
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

const int maxImageSize = 4096;


CanvasWidget::CanvasWidget(const QPixmap* image, MainWindow* mainWindow, QScrollArea* scrollArea,
                           QLabel* scaleLabel, QLabel* statusLabel, QWidget* parent) :
  QWidget(parent),
  mainWindow_(mainWindow),
  scrollArea_(scrollArea),
  scaleLabel_(scaleLabel),
  statusLabel_(statusLabel),
  originalImage_(image)
{
  acceptableScales_ << 0.01 << 0.015 << 0.02 << 0.025 << 0.03 << 0.04 << 0.05 << 0.06 << 0.07 << 0.08 << 0.09;
  acceptableScales_ << 0.10 << 0.12 << 0.14 << 0.17 << 0.20 << 0.23 << 0.26 << 0.30 << 0.35 << 0.40 << 0.45;
  acceptableScales_ << 0.50 << 0.60 << 0.70 << 0.80 << 0.90;
  acceptableScales_ << 1.00 << 1.25 << 1.50 << 1.75 << 2.00 << 2.50 << 3.00 << 4.00;
  iScale_ = acceptableScales_.indexOf(1.00);

  scrollArea_->viewport()->installEventFilter(this);
  setMouseTracking(true);
  figureType_ = DEFAULT_TYPE;
  isDefiningEtalon_ = true;
  showRuler_ = false;
  originalMetersPerPixel_ = 0.;
  metersPerPixel_ = 0.;
  figures_.append(newFigure(true));
  scaleChanged();
}

CanvasWidget::~CanvasWidget()
{
  delete originalImage_;
}


void CanvasWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setFont(mainWindow_->getInscriptionFont());
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.drawPixmap(event->rect().topLeft(), image_, event->rect());

  foreach (const Figure& figure, figures_)
    figure.draw(painter);

  if (showRuler_)
    drawRuler(painter, event->rect());
}

void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->buttons() == Qt::LeftButton) {
    bool polygonFinished = activeFigure().addPoint(originalPointUnderMouse_);
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
    originalPointUnderMouse_ = QPointF(event->pos()) / scale_;
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
    return true;
  }
  return false;
}



void CanvasWidget::setMode(FigureType newMode)
{
  figureType_ = newMode;
  resetAll();
}

bool CanvasWidget::isEtalonCorrect() const
{
  return originalMetersPerPixel_ > 0.;
}

QPixmap CanvasWidget::getModifiedImage()
{
  int oldIScale = iScale_;
  iScale_ = acceptableScales_.indexOf(1.00);
  scaleChanged();
  QPixmap resultingImage(size());
  render(&resultingImage);
  iScale_ = oldIScale;
  scaleChanged();
  return resultingImage;
}


void CanvasWidget::toggleEtalonDefinition(bool isDefiningEtalon)
{
  if (isDefiningEtalon_ == isDefiningEtalon)
    return;
  isDefiningEtalon_ = isDefiningEtalon;
  resetAll();
}

void CanvasWidget::toggleRuler(bool showRuler)
{
  if (showRuler_ == showRuler)
    return;
  showRuler_ = showRuler;
  update();
}


Figure& CanvasWidget::activeFigure()
{
  assert(!figures_.isEmpty());
  if (isDefiningEtalon_) {
    assert(figures_.first().isEtalon());
    return figures_.first();
  }
  else {
    assert(!figures_.last().isEtalon());
    return figures_.last();
  }
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

  QString rulerLabel = QString::number(metersLength) + " " + linearUnitSuffix;
  QPoint labelPos(rulerLeft + rulerFrameThickness + rulerTextMargin,
                  rulerY - rulerThickness / 2 - rulerFrameThickness - rulerTextMargin - painter.fontMetrics().descent());
  drawTextWithBackground(painter, rulerLabel, labelPos);
}


void CanvasWidget::finishPlotting()
{
  activeFigure().finish();
  QPolygonF originalPolygonDrawn = activeFigure().originalPolygon();
  figures_.append(newFigure(false));

  if (isDefiningEtalon_) {
    double originalEtalonPixelLength = 0.;
    QString prompt;
    switch (getDimensionality(figureType_)) {
      case FIGURE_1D:
        originalEtalonPixelLength = polylineLength(originalPolygonDrawn);
        prompt = QString::fromUtf8("Укажите длину эталона (%1): ").arg(linearUnitSuffix);
        break;
      case FIGURE_2D:
        originalEtalonPixelLength = std::sqrt(polygonArea(originalPolygonDrawn));
        prompt = QString::fromUtf8("Укажите площадь эталона (%1): ").arg(squareUnitSuffix);
        break;
    }
    bool ok;
    double etalonMetersSize = QInputDialog::getDouble(this, mainWindow_->appName(), prompt, 1., 0.001, 1e9, 3, &ok);
    if (ok && originalEtalonPixelLength > eps) {
      double etalonMetersLength;
      switch (getDimensionality(figureType_)) {
        case FIGURE_1D: etalonMetersLength = etalonMetersSize;            break;
        case FIGURE_2D: etalonMetersLength = std::sqrt(etalonMetersSize); break;
      }
      originalMetersPerPixel_ = etalonMetersLength / originalEtalonPixelLength;
      metersPerPixel_ = originalMetersPerPixel_ / scale_;
      mainWindow_->toggleEtalonDefinition(false);
    }
    else {
      originalMetersPerPixel_ = 0.;
      metersPerPixel_ = 0.;
      figures_.removeLast();
    }
  }
  updateAll();
}

Figure CanvasWidget::newFigure(bool isEtalon)
{
  return Figure(figureType_, isEtalon, &originalMetersPerPixel_, &scale_, &originalPointUnderMouse_);
}

void CanvasWidget::resetAll()
{
  activeFigure() = newFigure(isDefiningEtalon_);
  updateAll();
}

void CanvasWidget::scaleChanged()
{
  scale_ = acceptableScales_[iScale_];
  image_ = originalImage_->scaled(originalImage_->size() * scale_, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  metersPerPixel_ = originalMetersPerPixel_ / scale_;
  setFixedSize(image_.size());
  scaleLabel_->setText(QString::number(scale_ * 100.) + "%");
  updateAll();
}

void CanvasWidget::updateAll()
{
  statusLabel_->setText(activeFigure().statusString());
  update();
}
