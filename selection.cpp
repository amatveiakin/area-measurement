#include "polygon_math.h"
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

void Selection::assign(const Figure* figure__, Type type__, int iVertex__)
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


bool Selection::isDraggable() const
{
  switch (type) {
    case FIGURE:
      return false;
    case VERTEX:
    case INSCRIPTION:
      return true;
  }
  abort();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SelectionFinder

SelectionFinder::SelectionFinder(QPointF cursorPos) :
  cursorPos_(cursorPos),
  bestSelection_(),
  bestScore_(0.)
{
}

void SelectionFinder::testPolygon(QPolygonF polygon, const Figure* figure)
{
  double score = computeScore(pointToPolygonDistance(cursorPos_, polygon), polygonActivationRadius);
  if (score > bestScore_) {
    bestSelection_.assign(figure, Selection::FIGURE, -1);
    bestScore_ = score;
  }
}

void SelectionFinder::testPolyline(QPolygonF polyline, const Figure* figure)
{
  double score = computeScore(pointToPolylineDistance(cursorPos_, polyline), polylineActivationRadius);
  if (score > bestScore_) {
    bestSelection_.assign(figure, Selection::FIGURE, -1);
    bestScore_ = score;
  }
}

void SelectionFinder::testVertex(QPointF vertex, const Figure* figure, int iVertex)
{
  double score = computeScore(pointToPointDistance(cursorPos_, vertex), vertexActivationRadius);
  if (score > bestScore_) {
    bestSelection_.assign(figure, Selection::VERTEX, iVertex);
    bestScore_ = score;
  }
}

void SelectionFinder::testInscription(QRectF boundingRect, const Figure* figure)
{
  double score = computeScore(pointToPolygonDistance(cursorPos_, QPolygonF(boundingRect)), inscriptionActivationRadius);
  if (score > bestScore_) {
    bestSelection_.assign(figure, Selection::INSCRIPTION, -1);
    bestScore_ = score;
  }
}
