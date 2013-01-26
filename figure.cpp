#include <cassert>
#include <cmath>

#include <QPainter>

#include "canvaswidget.h"
#include "figure.h"
#include "paint_utils.h"
#include "selection.h"


const QString linearUnitSuffix = QString::fromUtf8("м");
const QString squareUnitSuffix = linearUnitSuffix + QString::fromUtf8("²");

const double selectionBallRadius = 3;

const QColor etalonDefaultPen_ = QColor(  0, 150,   0);
const QColor defaultPen_       = QColor(  0,  50, 240);
const QColor errorPen_         = QColor(255,   0,   0);

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


Figure::Figure(ShapeType shapeType, bool isEtalon, const CanvasWidget* canvas) :
  originalShape_(shapeType),
  isEtalon_(isEtalon),
  originalInscriptionPos_(),
  canvas_(canvas),
  size_(0.),
  penColor_(isEtalon ? etalonDefaultPen_ : defaultPen_)
  //penColor_(QColor::fromHsv(rand() % 360, 255, 127))
{
}


bool Figure::addPoint(QPointF originalNewPoint)
{
  return originalShape_.addPoint(originalNewPoint);
}

void Figure::finish()
{
  originalShape_.finish();
}


void Figure::testSelection(SelectionFinder& selectionFinder)
{
  Shape activeShape = getActiveOriginalShape();
  activeShape.scale(canvas_->scale_);

  switch (originalShape_.dimensionality()) {
    case SHAPE_1D: selectionFinder.testPolyline(activeShape.polygon(), this); break;
    case SHAPE_2D: selectionFinder.testPolygon (activeShape.polygon(), this); break;
  }

  QPolygonF vertices = activeShape.vertices();
  for (int i = 0; i < vertices.size(); ++i)
    selectionFinder.testVertex(vertices[i], this, i);

//  selectionFinder.testInscription();  // TODO
}

void Figure::dragTo(const Selection& selection, QPointF newPos)
{
  switch (selection.type) {
    case Selection::FIGURE:
      break;
    case Selection::VERTEX:
      originalShape_.dragVertex(selection.iVertex, newPos);
      break;
    case Selection::INSCRIPTION:
      // TODO: Make inscription draggable
      break;
  }
}

void Figure::draw(QPainter& painter) const
{
  Shape activeShape = getActiveOriginalShape();
  activeShape.scale(canvas_->scale_);
  QPolygonF activePolygon = activeShape.polygon();

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

  if (activeShape.correctness() != VALID_SHAPE) {
    setColor(painter, errorPen_);
  }
  else {
    if (isHovered())
      setColor(painter, penColor_.lighter(130));
    else
      setColor(painter, penColor_);
  }

  snapPolygonToPixelGrid(activePolygon);
  switch (originalShape_.dimensionality()) {
    case SHAPE_1D: painter.drawPolyline(activePolygon); break;
    case SHAPE_2D: painter.drawPolygon (activePolygon); break;
  }

  if (activePolygon.isClosed())
    activePolygon.pop_back();
  if (isSelected() || isHovered()) {
    QColor brushColor(255, 255, 255);
    QColor hoveredBrushColor(255, 255, 80);
    QColor penColor(0, 0, 0);
    if (!isSelected()) {
      brushColor.setAlpha(100);
      hoveredBrushColor.setAlpha(100);
      penColor.setAlpha(100);
    }
    painter.setBrush(brushColor);
    painter.setPen(penColor);

    for (int i = 0; i < activePolygon.size(); ++i) {
      painter.setBrush(i == hoveredVertex() ? hoveredBrushColor : brushColor);
      painter.drawEllipse(activePolygon[i], selectionBallRadius, selectionBallRadius);
    }
  }
}

QString Figure::statusString() const
{
  ShapeCorrectness correctness;
  QString sizeString = getSizeString(correctness);

  switch (correctness) {
    case VALID_SHAPE: {
      if (sizeString.isEmpty())
        return QString();
      QString etalonString = isEtalon_ ? QString::fromUtf8(" эталона") : QString();
      switch (originalShape_.dimensionality()) {
        case SHAPE_1D: return QString::fromUtf8("Длина%1: %2"  ).arg(etalonString).arg(sizeString);
        case SHAPE_2D: return QString::fromUtf8("Площадь%1: %2").arg(etalonString).arg(sizeString);
      }
      break;
    }
    case SELF_INTERSECTING_POLYGON: {
      return QString::fromUtf8("Многоугольник не должен самопересекаться!");
    }
  }
  assert(false);
}


Shape Figure::getActiveOriginalShape() const
{
  Shape activeOriginalShape = originalShape_;
  if (!activeOriginalShape.isFinished())
    activeOriginalShape.addPoint(canvas_->originalPointUnderMouse_);
  return activeOriginalShape;
}

// QPainter with antialiasing gives clearer results for horisontal and vertical lines after this function
void Figure::snapPolygonToPixelGrid(QPolygonF& polygon) const
{
  for (int i = 0; i < polygon.size(); ++i)
    polygon[i] = QPointF(floor(polygon[i].x()) + 0.5, floor(polygon[i].y()) + 0.5);
}

QString Figure::getSizeString(ShapeCorrectness& correctness) const
{
  Shape activeShape = getActiveOriginalShape();
  correctness = activeShape.correctness();
  if (!canvas_->hasEtalon())
    return QString();
  switch (correctness) {
    case VALID_SHAPE:
      switch (activeShape.dimensionality()) {
        case SHAPE_1D: {
          double length = activeShape.length() * canvas_->originalMetersPerPixel_;
          return QString("%1 %2").arg(length).arg(linearUnitSuffix);
        }
        case SHAPE_2D: {
          double area = activeShape.area() * sqr(canvas_->originalMetersPerPixel_);
          return QString("%1 %2").arg(area).arg(squareUnitSuffix);
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
  ShapeCorrectness correctness;
  QString sizeString = getSizeString(correctness);
  if (correctness == VALID_SHAPE && !sizeString.isEmpty())
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

int Figure::hoveredVertex() const
{
  return canvas_->hover_.iVertex;
}
