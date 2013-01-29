#ifndef SHAPE_H
#define SHAPE_H

#include <QPolygonF>

#include "defines.h"

class Shape
{
public:
  Shape(ShapeType shapeType);

  bool addPoint(QPointF newPoint);  // returns whether polygon is finished
  void finish();
  void scale(double factor);
  void dragVertex(int iVertex, QPointF newPos);

  ShapeType type() const                { return type_; }
  Dimensionality dimensionality() const { return getDimensionality(type_); }
  bool isEmpty() const                  { return vertices_.isEmpty(); }
  bool isFinished() const               { return isFinished_; }
  bool isValid() const                  { return correctness() == VALID_SHAPE; }
  int nVertices() const                 { return vertices().size(); }
  QPolygonF vertices() const;
  QPolygonF polygon() const;
  ShapeCorrectness correctness() const;
  double length() const;
  double area() const;

private:
  QPolygonF vertices_;  // never closed
  ShapeType type_;
  bool      isFinished_;
};

#endif // SHAPE_H
