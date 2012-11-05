#ifndef MODES_H
#define MODES_H

enum Mode
{
  SET_ETALON,
  MEASURE_SEGMENT_LENGTH,
  MEASURE_POLYLINE_LENGTH,
  MEASURE_CLOSED_POLYLINE_LENGTH,
  MEASURE_RECTANGLE_AREA,
  MEASURE_POLYGON_AREA
};

enum ModeKind
{
  ETALON,
  LENGTH,
  AREA
};

static inline ModeKind getModeKind(Mode mode)
{
  switch (mode) {
    case SET_ETALON:
      return ETALON;
    case MEASURE_SEGMENT_LENGTH:
    case MEASURE_POLYLINE_LENGTH:
    case MEASURE_CLOSED_POLYLINE_LENGTH:
      return LENGTH;
    case MEASURE_POLYGON_AREA:
    case MEASURE_RECTANGLE_AREA:
      return AREA;
  }
  abort();
}

#endif // MODES_H
