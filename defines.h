#ifndef DEFINES_H
#define DEFINES_H

#include "debug_utils.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Misc

extern const double eps;
extern const double positiveInf;
extern const double negativeInf;

static inline double sqr(double x) { return x * x; }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shapes

enum ShapeType
{
  SEGMENT,
  POLYLINE,
  CLOSED_POLYLINE,
  RECTANGLE,
  POLYGON,

  DEFAULT_TYPE = SEGMENT
};

enum Dimensionality
{
  SHAPE_1D,
  SHAPE_2D
};

enum ShapeCorrectness
{
  VALID_SHAPE,
  SELF_INTERSECTING_POLYGON
};

Dimensionality getDimensionality(ShapeType shapeType);

#endif // DEFINES_H
