// TODO: make cursor the fixed point of the zoom
// TODO: compute area for selfintersecting polygons
// TODO: polygon editing: move caption, change color, move segments (?), add points (?), delete points (?)
// TODO: set scale by two points GPS coordinates
// TODO: result printing
// TODO: perhaps, it's time to use 3 modes instead of 2: normal draw, draw etalon, edit?
// TODO: won't it be easier to use weak pointers (e.g., QPointers) to figures?
// TODO: reduce number of digits after the decimal point

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
#include "shape.h"


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


CanvasWidget::CanvasWidget(const QPixmap& image, MainWindow* mainWindow, QScrollArea* scrollArea,
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
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  shapeType_ = DEFAULT_TYPE;
  isDefiningEtalon_ = true;
  showRuler_ = false;
  etalonFigure_ = 0;
  activeFigure_ = 0;
  clearEtalon();
  scaleChanged();
}

CanvasWidget::~CanvasWidget()
{
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
  event->accept();
}

void CanvasWidget::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Delete) {
    if (!selection_.isEmpty() && !selection_.figure->isEtalon()) {
      removeFigure(selection_.figure);
      updateAll();
    }
  }
  else {
    QWidget::keyPressEvent(event);
  }
}

void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
  updateMousePos(event->pos());
  if (event->buttons() == Qt::LeftButton) {
    selection_ = hover_;
    if (hover_.isEmpty()) {
      if (!activeFigure_) {
        if (isDefiningEtalon_) {
          clearEtalon();
          removeFigure(etalonFigure_);
        }
        addActiveFigure();
      }
      bool polygonFinished = activeFigure_->addPoint(originalPointUnderMouse_);
      if (polygonFinished)
        finishDrawing();
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

void CanvasWidget::mouseReleaseEvent(QMouseEvent* event)
{
  updateHover();
  event->accept();
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() == Qt::NoButton) {
    updateMousePos(event->pos());
    updateAll();
  }
  else if (event->buttons() == Qt::LeftButton) {
    updateMousePos(event->pos());
    selection_.dragTo(originalPointUnderMouse_);
    if (!selection_.isEmpty() && selection_.figure->isEtalon())
      defineEtalon(selection_.figure);
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
  if (event->buttons() == Qt::LeftButton) {
    if (activeFigure_) {
      if (activeFigure_->originalShape().nVertices() == 1) {
        removeFigure(activeFigure_);
        selection_.clear();
        updateAll();
      }
      else {
        finishDrawing();
      }
    }
  }
  event->accept();
}

bool CanvasWidget::eventFilter(QObject* object, QEvent* event__)
{
  if (object == scrollArea_->viewport() && event__->type() == QEvent::Wheel) {
    QWheelEvent* event = static_cast<QWheelEvent*>(event__);
    int numSteps = event->delta() / 120;
    iScale_ = qBound(0, iScale_ + numSteps, acceptableScales_.size() - 1);
    int size = qMax(originalImage_.width(), originalImage_.height());
    while (size * acceptableScales_[iScale_] > maxImageSize && acceptableScales_[iScale_] > 1.)
      iScale_--;
    scaleChanged();
    return true;
  }
  return false;
}



void CanvasWidget::setMode(ShapeType newMode)
{
  shapeType_ = newMode;
  resetAll();
}

bool CanvasWidget::hasEtalon() const
{
  return originalMetersPerPixel_ > 0.;
}

QPixmap CanvasWidget::getModifiedImage()
{
  Selection oldSelection_ = selection_;
  Selection oldHover = hover_;
  int oldIScale = iScale_;
  iScale_ = acceptableScales_.indexOf(1.00);
  selection_.clear();
  hover_.clear();
  scaleChanged();
  QPixmap resultingImage(size());
  render(&resultingImage);
  iScale_ = oldIScale;
  selection_ = oldSelection_;
  hover_ = oldHover;
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


void CanvasWidget::addActiveFigure()
{
  ASSERT_RETURN(!activeFigure_);
  figures_.append(Figure(shapeType_, isDefiningEtalon_, this));
  activeFigure_ = &figures_.last();
}

void CanvasWidget::removeFigure(const Figure* figure)
{
  if (!figure)
    return;

  if (etalonFigure_ == figure)
    etalonFigure_ = 0;
  if (activeFigure_ == figure)
    activeFigure_ = 0;
  if (selection_.figure == figure)
    selection_.clear();
  if (hover_.figure == figure)
    hover_.clear();

  bool erased = false;
  for (FigureIter it = figures_.begin(); it != figures_.end(); ++it) {
    if (&(*it) == figure) {
      figures_.erase(it);
      erased = true;
      break;
    }
  }
  ASSERT_RETURN(erased);
}


void CanvasWidget::drawRuler(QPainter& painter, const QRect& rect)
{
  int maxLength = qMin(rulerMaxLength, rect.width() - 2 * rulerMargin);
  if (!hasEtalon() || maxLength < rulerMinLength)
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


void CanvasWidget::updateMousePos(QPoint mousePos)
{
  mousePos.setX(qBound(0, mousePos.x(), image_.width()));
  mousePos.setY(qBound(0, mousePos.y(), image_.height()));
  pointUnderMouse_ = mousePos;
  originalPointUnderMouse_ = pointUnderMouse_ / scale_;
}

void CanvasWidget::updateHover()
{
  Selection newHover;
  if (activeFigure_) {
    newHover.clear();
  }
  else {
    SelectionFinder selectionFinder(pointUnderMouse_);
    for (FigureIter it = figures_.begin(); it != figures_.end(); ++it)
      if (it->isFinished())
        it->testSelection(selectionFinder);
    newHover = selectionFinder.bestSelection();
  }
  if (hover_ != newHover) {
    hover_ = newHover;
    update();
  }
}

void CanvasWidget::updateStatus()
{
  QString statusString;
  if (activeFigure_)
    statusString = activeFigure_->statusString();
  else if (!selection_.isEmpty())
    statusString = selection_.figure->statusString();
  statusLabel_->setText(statusString);
}

void CanvasWidget::defineEtalon(Figure* newEtalonFigure)
{
  ASSERT_RETURN(newEtalonFigure && newEtalonFigure->isFinished());
  bool isResizing = (etalonFigure_ == newEtalonFigure);
  etalonFigure_ = newEtalonFigure;
  const Shape originalShapeDrawn = newEtalonFigure->originalShape();
  double originalEtalonPixelLength = 0.;
  QString prompt;
  switch (originalShapeDrawn.dimensionality()) {
    case SHAPE_1D:
      originalEtalonPixelLength = originalShapeDrawn.length();
      prompt = QString::fromUtf8("Укажите длину эталона (%1): ").arg(linearUnitSuffix);
      break;
    case SHAPE_2D:
      originalEtalonPixelLength = std::sqrt(originalShapeDrawn.area());
      prompt = QString::fromUtf8("Укажите площадь эталона (%1): ").arg(squareUnitSuffix);
      break;
  }
  bool userInputIsOk = true;
  if (!isResizing)
    etalonMetersSize_ = QInputDialog::getDouble(this, mainWindow_->appName(), prompt, 1., 0.001, 1e9, 3, &userInputIsOk);
  if (originalShapeDrawn.isValid() && originalEtalonPixelLength > 0. && userInputIsOk) {
    double etalonMetersLength = 0.;
    switch (originalShapeDrawn.dimensionality()) {
      case SHAPE_1D: etalonMetersLength = etalonMetersSize_;            break;
      case SHAPE_2D: etalonMetersLength = std::sqrt(etalonMetersSize_); break;
    }
    originalMetersPerPixel_ = etalonMetersLength / originalEtalonPixelLength;
    metersPerPixel_ = originalMetersPerPixel_ / scale_;
  }
  else {
    clearEtalon(true);
  }
  if (!isResizing)
    mainWindow_->toggleEtalonDefinition(false);
}

void CanvasWidget::clearEtalon(bool invalidateOnly)
{
  if (!invalidateOnly)
    etalonMetersSize_ = 0;
  originalMetersPerPixel_ = 0.;
  metersPerPixel_ = 0.;
}

void CanvasWidget::finishDrawing()
{
  ASSERT_RETURN(activeFigure_);
  activeFigure_->finish();
  Figure *oldActiveFigure = activeFigure_;
  activeFigure_ = 0;
  if (isDefiningEtalon_)
    defineEtalon(oldActiveFigure);
  selection_.setFigure(oldActiveFigure);
  updateAll();
}

void CanvasWidget::resetAll()
{
  removeFigure(activeFigure_);
  updateAll();
}

void CanvasWidget::scaleChanged()
{
  scale_ = acceptableScales_[iScale_];
  image_ = originalImage_.scaled(originalImage_.size() * scale_, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  metersPerPixel_ = originalMetersPerPixel_ / scale_;
  setFixedSize(image_.size());
  scaleLabel_->setText(QString::number(scale_ * 100.) + "%");
  updateAll();
}

void CanvasWidget::updateAll()
{
  updateHover();
  updateStatus();
  update();
}
