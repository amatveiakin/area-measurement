#include <cassert>

#include <QPainter>

#include "canvaswidget.h"
#include "figure.h"
#include "paint_utils.h"
#include "polygon_math.h"
#include "selection.h"


const QString linearUnitSuffix = QString::fromUtf8("м");
const QString squareUnitSuffix = linearUnitSuffix + QString::fromUtf8("²");


const QColor etalonStaticPen_ = QColor(  0, 150,   0);
const QColor etalonActivePen_ = QColor(  0, 200,   0);
const QColor staticPen_       = QColor(  0,  50, 240);
const QColor activePen_       = QColor(  0, 100, 240);
const QColor errorPen_        = QColor(255,   0,   0);

static inline QColor getFillColor(QColor penColor)
{
  penColor.setAlpha(80);
  return penColor;
}

static inline void setColor(QPainter& painter, QColor penColor)
{
  painter.setPen(penColor);
  painter.setBrush(getFillColor(penColor));
}


Figure::Figure(FigureType figureType, bool isEtalon, const CanvasWidget* canvas) :
  figureType_(figureType),
  isEtalon_(isEtalon),
  isFinished_(false),
  originalPolygon_(),
  originalInscriptionPos_(),
  canvas_(canvas),
  size_(0.),
  penColor_(isEtalon ? etalonStaticPen_ : staticPen_)
  //penColor_(QColor::fromHsv(rand() % 360, 255, 127))
{
}


bool Figure::addPoint(QPointF originalNewPoint)
{
  return addPointToPolygon(originalPolygon_, originalNewPoint, figureType_);
}

void Figure::finish()
{
  finishPolygon(originalPolygon_, figureType_);
  isFinished_ = true;
}


void Figure::testSelection(SelectionFinder& selectionFinder) const
{
  QPolygonF activePolygon = getActiveOriginalPolygon();
  scalePolygon(activePolygon);

  switch (getDimensionality(figureType_)) {
    case FIGURE_1D: selectionFinder.testPolyline(activePolygon, this); break;
    case FIGURE_2D: selectionFinder.testPolygon (activePolygon, this); break;
  }

  for (int i = 0; i < activePolygon.size(); ++i)
    selectionFinder.testVertex(activePolygon[i], this, i);

//  selectionFinder.testInscription();  // TODO
}

void Figure::draw(QPainter& painter) const
{
  PolygonCorrectness correctness;
  QPolygonF activePolygon = getActiveOriginalPolygon(&correctness);
  scalePolygon(activePolygon);

  TextDrawer inscriptionTextDrawer;
  QString inscription = getInscription();
  if (!inscription.isEmpty()) {
    QPointF pivot = activePolygon.first();
    foreach (QPointF v, activePolygon)
      if (    v.y() <  pivot.y()
          || (v.y() == pivot.y() && v.x() < pivot.x()))
        pivot = v;
    QPoint inscriptionPos = pivot.toPoint() + QPoint(painter.fontMetrics().averageCharWidth() / 2, painter.fontMetrics().height());
    inscriptionTextDrawer = drawTextWithBackground(painter, inscription, inscriptionPos);
  }

  if (correctness != VALID_POLYGON) {
    setColor(painter, errorPen_);
  }
  else if (isFinished_) {
    if (isHovered())
      setColor(painter, penColor_.lighter());
    else
      setColor(painter, penColor_);
  }
  else {
    if (isEtalon_)
      setColor(painter, etalonActivePen_);
    else
      setColor(painter, activePen_);
  }

  switch (getDimensionality(figureType_)) {
    case FIGURE_1D: painter.drawPolyline(activePolygon); break;
    case FIGURE_2D: painter.drawPolygon (activePolygon); break;
  }
}

QString Figure::statusString() const
{
  PolygonCorrectness correctness;
  QString sizeString = getSizeString(correctness);

  switch (correctness) {
    case VALID_POLYGON: {
      if (sizeString.isEmpty())
        return QString();
      QString etalonString = isEtalon_ ? QString::fromUtf8(" эталона") : QString();
      switch (getDimensionality(figureType_)) {
        case FIGURE_1D: return QString::fromUtf8("Длина%1: %2"  ).arg(etalonString).arg(sizeString);
        case FIGURE_2D: return QString::fromUtf8("Площадь%1: %2").arg(etalonString).arg(sizeString);
      }
      break;
    }
    case SELF_INTERSECTING_POLYGON: {
      return QString::fromUtf8("Многоугольник не должен самопересекаться!");
    }
  }
  assert(false);
}


QPolygonF Figure::getActiveOriginalPolygon(PolygonCorrectness* correctness) const
{
  QPolygonF activeOriginalPolygon = originalPolygon_;
  if (!isFinished_) {
    addPointToPolygon(activeOriginalPolygon, canvas_->originalPointUnderMouse_, figureType_);
    finishPolygon(activeOriginalPolygon, figureType_);
  }
  if (correctness)
    *correctness = polygonCorrectness(activeOriginalPolygon, figureType_);
  return activeOriginalPolygon;
}

void Figure::scalePolygon(QPolygonF& polygon) const
{
  for (QPolygonF::Iterator it = polygon.begin(); it != polygon.end(); ++it)
    *it *= canvas_->scale_;
}

QString Figure::getSizeString(PolygonCorrectness& correctness) const
{
  QPolygonF activePolygon = getActiveOriginalPolygon(&correctness);
  switch (correctness) {
    case VALID_POLYGON:
      switch (getDimensionality(figureType_)) {
        case FIGURE_1D: {
          double length = polylineLength(activePolygon) * canvas_->originalMetersPerPixel_;
          if (length > eps)
            return QString("%1 %2").arg(length).arg(linearUnitSuffix);
          break;
        }
        case FIGURE_2D: {
          double area = polygonArea(activePolygon) * sqr(canvas_->originalMetersPerPixel_);
          if (area > eps)
            return QString("%1 %2").arg(area).arg(squareUnitSuffix);
          break;
        }
      }
      break;
    case SELF_INTERSECTING_POLYGON:
      break;
  }
  return QString();
}

QString Figure::getInscription() const
{
  PolygonCorrectness correctness;
  QString sizeString = getSizeString(correctness);
  if (correctness == VALID_POLYGON && !sizeString.isEmpty())
    return sizeString + (isEtalon_ ? QString::fromUtf8(" [эталон]") : QString());
  else
    return QString();
}

bool Figure::isSelected() const
{
  return canvas_->selection_.figure == this;
}

bool Figure::isHovered() const
{
  return canvas_->hover_.figure == this;
}
