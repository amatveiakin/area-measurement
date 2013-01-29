#include <QLineF>

#include "shape.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers

double segmentLenght(QPointF a, QPointF b)
{
  return QLineF(a, b).length();
}

void assertPolygonIsClosed(QPolygonF polygon)
{
  ASSERT_RETURN(polygon.isEmpty() || polygon.first() == polygon.last());
}

bool testSegmentsCross(QPointF a, QPointF b, QPointF c, QPointF d)
{
  return QLineF(a, b).intersect(QLineF(c, d), 0) == QLineF::BoundedIntersection;
}

bool isSelfintersectingPolygon(QPolygonF polygon)
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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shape

Shape::Shape(ShapeType shapeType) :
  vertices_(),
  type_(shapeType),
  isFinished_(false)
{
}


bool Shape::addPoint(QPointF newPoint)
{
  ASSERT_RETURN_V(!isFinished_, true);
  if (!vertices_.isEmpty() && newPoint == vertices_.back())
    return false;
  vertices_.append(newPoint);

  switch (type_) {
    case SEGMENT:
    case RECTANGLE:
      ASSERT_RETURN_V(vertices_.size() <= 2, true);
      isFinished_ = (vertices_.size() == 2);
      return isFinished_;

    case POLYLINE:
    case CLOSED_POLYLINE:
    case POLYGON:
      return false;
  }
  ERROR_RETURN_V(true);
}

void Shape::finish()
{
  ASSERT_RETURN(!vertices_.empty());
  isFinished_ = true;
}

void Shape::scale(double factor)
{
  for (int i = 0; i < vertices_.size(); ++i)
    vertices_[i] *= factor;
}

void Shape::dragVertex(int iVertex, QPointF newPos)
{
  switch (type_) {
    case SEGMENT:
    case POLYLINE:
    case CLOSED_POLYLINE:
    case POLYGON:
      vertices_[iVertex] = newPos;
      break;

    case RECTANGLE:
      ASSERT_RETURN(vertices_.size() == 2);
      switch (iVertex) {
        case 0: vertices_[0]      = newPos;                                     break;
        case 1: vertices_[0].ry() = newPos.y(); vertices_[1].rx() = newPos.x(); break;
        case 2:                                 vertices_[1]      = newPos;     break;
        case 3: vertices_[0].rx() = newPos.x(); vertices_[1].ry() = newPos.y(); break;
        default: ERROR_RETURN();
      }
      break;
  }
}


QPolygonF Shape::vertices() const
{
  QPolygonF result = vertices_;

  switch (type_) {
    case SEGMENT:
    case POLYLINE:
    case CLOSED_POLYLINE:
    case POLYGON:
      break;

    case RECTANGLE:
      if (result.size() == 2) {
        result = QPolygonF(QRectF(result[0], result[1]));
        result.pop_back();
      }
      break;
  }
  return result;
}

QPolygonF Shape::polygon() const
{
  QPolygonF result = vertices_;

  switch (type_) {
    case SEGMENT:
    case POLYLINE:
      break;

    case CLOSED_POLYLINE:
    case POLYGON:
      result.append(result.first());
      break;

    case RECTANGLE:
      if (result.size() == 2)
        result = QPolygonF(QRectF(result[0], result[1]));
      break;
  }
  return result;
}

ShapeCorrectness Shape::correctness() const
{
  switch (type_) {
    case SEGMENT:
    case POLYLINE:
    case CLOSED_POLYLINE:
    case RECTANGLE:
      return VALID_SHAPE;

    case POLYGON:
      return isSelfintersectingPolygon(polygon()) ? SELF_INTERSECTING_POLYGON : VALID_SHAPE;
  }
  ERROR_RETURN_V(VALID_SHAPE);
}

double Shape::length() const
{
  ASSERT_RETURN_V(dimensionality() == SHAPE_1D, 0.);
  QPolygonF p = polygon();
  double result = 0.;
  for (int i = 0; i < p.size() - 1; i++)
    result += segmentLenght(p[i], p[i + 1]);
  return result;
}

double Shape::area() const
{
  ASSERT_RETURN_V(dimensionality() == SHAPE_2D, 0.);
  QPolygonF p = polygon();
  assertPolygonIsClosed(p);
  double result = 0.;
  for (int i = 1; i < p.size() - 2; i++)
    result += triangleSignedArea(p[0], p[i], p[i + 1]);
  return qAbs(result);
}
