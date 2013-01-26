#include "figure.h"
#include "selection.h"


const double vertexActivationRadius      = 8.;
const double polylineActivationRadius    = 6.;
const double polygonActivationRadius     = 2.;
const double inscriptionActivationRadius = polygonActivationRadius;

static inline double computeScore(double distance, double activationRadius)
{
  return qMax(0., activationRadius - distance);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Distance computation

double pointToPointDistance(QPointF point1, QPointF point2)
{
  return QLineF(point1, point2).length();
}

double pointToSegmentDistance(QPointF point, QLineF line)
{
  QPointF normalVector = QPointF(line.dy(), -line.dx()) * 100.;  // The multiplier is to get an ``infinite straight line''
  QPointF projection;
  switch (line.intersect(QLineF(point - normalVector, point + normalVector), &projection)) {
    case QLineF::BoundedIntersection:
      return pointToPointDistance(point, projection);
    case QLineF::NoIntersection:
    case QLineF::UnboundedIntersection:
      return qMin(pointToPointDistance(point, line.p1()), pointToPointDistance(point, line.p2()));
  }
  abort();
}

double pointToPolylineDistance(QPointF point, QPolygonF polyline)
{
  double distance = positiveInf;
  for (int i = 0; i < polyline.size() - 1; i++)
    distance = qMin(distance, pointToSegmentDistance(point, QLineF(polyline[i], polyline[i + 1])));
  return distance;
}

double pointToPolygonDistance(QPointF point, QPolygonF polygon)
{
  return polygon.containsPoint(point, Qt::OddEvenFill) ? 0. : pointToPolylineDistance(point, polygon);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Selection

Selection::Selection()
{
  reset();
}


void Selection::reset()
{
  figure  = 0;
  type    = FIGURE;
  iVertex = -1;
}

void Selection::assign(Figure* figure__, Type type__, int iVertex__)
{
  figure  = figure__;
  type    = type__;
  iVertex = iVertex__;
}


bool Selection::operator==(const Selection& other)
{
  return    (figure  == other.figure)
         && (type    == other.type)
         && (iVertex == other.iVertex);
}

bool Selection::operator!=(const Selection& other)
{
  return !(*this == other);
}


//bool Selection::isDraggable() const
//{
//  if (isEmpty())
//    return false;
//  switch (type) {
//    case FIGURE:
//      return false;
//    case VERTEX:
//    case INSCRIPTION:
//      return true;
//  }
//  abort();
//}

void Selection::dragTo(QPointF newPos)
{
  if (!isEmpty())
    figure->dragTo(*this, newPos);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SelectionFinder

SelectionFinder::SelectionFinder(QPointF cursorPos) :
  cursorPos_(cursorPos),
  bestSelection_(),
  bestScore_(0.)
{
}

void SelectionFinder::testPolygon(QPolygonF polygon, Figure* figure)
{
  double score = computeScore(pointToPolygonDistance(cursorPos_, polygon), polygonActivationRadius);
  if (score > bestScore_) {
    bestSelection_.assign(figure, Selection::FIGURE, -1);
    bestScore_ = score;
  }
}

void SelectionFinder::testPolyline(QPolygonF polyline, Figure* figure)
{
  double score = computeScore(pointToPolylineDistance(cursorPos_, polyline), polylineActivationRadius);
  if (score > bestScore_) {
    bestSelection_.assign(figure, Selection::FIGURE, -1);
    bestScore_ = score;
  }
}

void SelectionFinder::testVertex(QPointF vertex, Figure* figure, int iVertex)
{
  double score = computeScore(pointToPointDistance(cursorPos_, vertex), vertexActivationRadius);
  if (score > bestScore_) {
    bestSelection_.assign(figure, Selection::VERTEX, iVertex);
    bestScore_ = score;
  }
}

void SelectionFinder::testInscription(QRectF boundingRect, Figure* figure)
{
  double score = computeScore(pointToPolygonDistance(cursorPos_, QPolygonF(boundingRect)), inscriptionActivationRadius);
  if (score > bestScore_) {
    bestSelection_.assign(figure, Selection::INSCRIPTION, -1);
    bestScore_ = score;
  }
}
