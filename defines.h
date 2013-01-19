#ifndef DEFINES_H
#define DEFINES_H

enum FigureType
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
  FIGURE_1D,
  FIGURE_2D
};

static inline Dimensionality getDimensionality(FigureType figureType)
{
  switch (figureType) {
    case SEGMENT:
    case POLYLINE:
    case CLOSED_POLYLINE:
      return FIGURE_1D;
    case POLYGON:
    case RECTANGLE:
      return FIGURE_2D;
  }
  abort();
}


enum PolygonCorrectness
{
  VALID_POLYGON,
  SELF_INTERSECTING_POLYGON
};

#endif // DEFINES_H
