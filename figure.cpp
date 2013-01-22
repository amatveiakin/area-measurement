#include <cassert>

#include <QPainter>

#include "figure.h"
#include "paint_utils.h"
#include "polygon_math.h"


const QString linearUnitSuffix = QString::fromUtf8("м");
const QString squareUnitSuffix = linearUnitSuffix + QString::fromUtf8("²");


QColor Figure::etalonStaticPen_ = QColor(  0, 150,   0);
QColor Figure::etalonActivePen_ = QColor(  0, 200,   0);
QColor Figure::staticPen_       = QColor(  0,  50, 240);
QColor Figure::activePen_       = QColor(  0, 100, 240);
QColor Figure::errorPen_        = QColor(255,   0,   0);

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


Figure::Figure(FigureType figureType, bool isEtalon, const double* originalMetersPerPixel,
               const double* scale, QPoint* originalPointUnderMouse) :
  figureType_(figureType),
  isEtalon_(isEtalon),
  isFinished_(false),
  originalPolygon_(),
  originalInscriptionPos_(),
  originalMetersPerPixel_(originalMetersPerPixel),
  scale_(scale),
  originalPointUnderMouse_(originalPointUnderMouse),
  size_(0.),
  penColor_(isEtalon ? etalonStaticPen_ : staticPen_)
  //penColor_(QColor::fromHsv(rand() % 360, 255, 127))
{
}


bool Figure::addPoint(QPoint originalNewPoint)
{
  return addPointToPolygon(originalPolygon_, originalNewPoint, figureType_);
}

void Figure::finish()
{
  finishPolygon(originalPolygon_, figureType_);
  isFinished_ = true;
}

void Figure::draw(QPainter& painter) const
{
  PolygonCorrectness correctness;
  QPolygon activePolygon = getActiveOriginalPolygon(correctness);
  scalePolygon(activePolygon);

  TextDrawer inscriptionTextDrawer;
  QString inscription = getInscription();
  if (!inscription.isEmpty()) {
    QPoint pivot = activePolygon.first();
    foreach (QPoint v, activePolygon)
      if (    v.y() <  pivot.y()
          || (v.y() == pivot.y() && v.x() < pivot.x()))
        pivot = v;
    inscriptionTextDrawer = drawTextWithBackground(painter, inscription, pivot + QPoint(painter.fontMetrics().averageCharWidth() / 2, painter.fontMetrics().height()));
  }

  if (correctness != VALID_POLYGON) {
    setColor(painter, errorPen_);
  }
  else if (isFinished_) {
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


QPolygon Figure::getActiveOriginalPolygon(PolygonCorrectness& correctness) const
{
  QPolygon activeOriginalPolygon = originalPolygon_;
  if (!isFinished_) {
    addPointToPolygon(activeOriginalPolygon, *originalPointUnderMouse_, figureType_);
    finishPolygon(activeOriginalPolygon, figureType_);
  }
  correctness = polygonCorrectness(activeOriginalPolygon, figureType_);
  return activeOriginalPolygon;
}

void Figure::scalePolygon(QPolygon& polygon) const
{
  for (QPolygon::Iterator it = polygon.begin(); it != polygon.end(); ++it)
    *it *= *scale_;
}

QString Figure::getSizeString(PolygonCorrectness& correctness) const
{
  QPolygon activePolygon = getActiveOriginalPolygon(correctness);
  switch (correctness) {
    case VALID_POLYGON:
      switch (getDimensionality(figureType_)) {
        case FIGURE_1D: {
          double length = polylineLength(activePolygon) * (*originalMetersPerPixel_);
          if (length > eps)
            return QString("%1 %2").arg(length).arg(linearUnitSuffix);
          break;
        }
        case FIGURE_2D: {
          double area = polygonArea(activePolygon) * sqr(*originalMetersPerPixel_);
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
