#include <limits>

#include "defines.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Misc

const double eps = 1e-6;
const double positiveInf = std::numeric_limits<double>::max();
const double negativeInf = std::numeric_limits<double>::min();


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shapes

Dimensionality getDimensionality(ShapeType shapeType)
{
  switch (shapeType) {
    case SEGMENT:
    case POLYLINE:
    case CLOSED_POLYLINE:
      return SHAPE_1D;
    case POLYGON:
    case RECTANGLE:
      return SHAPE_2D;
  }
  ERROR_RETURN_V(SHAPE_1D);
}
