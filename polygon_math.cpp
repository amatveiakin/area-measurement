#include <cassert>
#include <cmath>

#include <QLineF>

#include "polygon_math.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Misc

const double eps = 1e-6;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Length

double segmentLenght(QPointF a, QPointF b)
{
  return std::sqrt(sqr(a.x() - b.x()) + sqr(a.y() - b.y()));
}

double polylineLength(const QPolygonF& polyline)
{
  double length = 0;
  for (int i = 0; i < polyline.size() - 1; i++)
    length += segmentLenght(polyline[i], polyline[i + 1]);
  return length;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Area

void assertPolygonIsClosed(const QPolygonF& polygon)
{
  assert(polygon.isEmpty() || polygon.first() == polygon.last());
}

bool testSegmentsCross(QPointF a, QPointF b, QPointF c, QPointF d)
{
  return QLineF(a, b).intersect(QLineF(c, d), 0) == QLineF::BoundedIntersection;
}

bool isSelfintersectingPolygon(const QPolygonF& polygon)
{
  assertPolygonIsClosed(polygon);
  int n = polygon.size() - 1;  // cut off last vertex
  for (int i1 = 0; i1 < n; i1++) {
    int i2 = (i1 + 1) % n;
    for (int j1 = 0; j1 < n; j1++) {
      int j2 = (j1 + 1) % n;
      if (i1 != j1 && i1 != j2 && i2 != j1
          && testSegmentsCross(polygon[i1], polygon[i2], polygon[j1], polygon[j2]))
        return true;
    }
  }
  return false;
}

double triangleSignedArea(QPointF a, QPointF b, QPointF c)
{
  QPointF p = b - a;
  QPointF q = c - a;
  return (p.x() * q.y() - p.y() * q.x()) / 2.0;
}

double polygonArea(const QPolygonF& polygon)
{
  assertPolygonIsClosed(polygon);
  double area = 0;
  for (int i = 1; i < polygon.size() - 2; i++)
    area += triangleSignedArea(polygon[0], polygon[i], polygon[i + 1]);
  return qAbs(area);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Polygon

bool addPointToPolygon(QPolygonF& polygon, QPointF newPoint, FigureType figureType)
{
  switch (figureType) {
    case SEGMENT: {
      polygon.append(newPoint);
      return polygon.size() >= 2;
    }

    case POLYLINE:
    case CLOSED_POLYLINE:
    case POLYGON: {
      if (polygon.isEmpty() || newPoint != polygon.back())
        polygon.append(newPoint);
      return false;
    }

    case RECTANGLE: {
      if (polygon.isEmpty()) {
        if (polygon.isEmpty() || newPoint != polygon.back())
          polygon.append(newPoint);
        return false;
      }
      else {
        polygon = QPolygonF(QRectF(polygon.first(), newPoint));
        return true;
      }
    }
  }
  abort();
}

void finishPolygon(QPolygonF& polygon, FigureType figureType)
{
  if (polygon.isEmpty())
    return;

  switch (figureType) {
    case SEGMENT:
    case POLYLINE:
    case RECTANGLE:
      return;

    case CLOSED_POLYLINE:
    case POLYGON:
      polygon.append(polygon.first());
      return;
  }
  abort();
}

PolygonCorrectness polygonCorrectness(const QPolygonF& polygon, FigureType figureType)
{
  switch (figureType) {
    case SEGMENT:
    case POLYLINE:
    case CLOSED_POLYLINE:
    case RECTANGLE:
      return VALID_POLYGON;

    case POLYGON:
      return isSelfintersectingPolygon(polygon) ? SELF_INTERSECTING_POLYGON : VALID_POLYGON;
  }
  abort();
}
