#ifndef POLYGON_MATH_H
#define POLYGON_MATH_H

#include <QPolygon>

#include "defines.h"

extern const double eps;

static inline double sqr(double x) { return x * x; }

double segmentLenght(QPoint a, QPoint b);
double polylineLength(const QPolygon& polyline);

bool isSelfintersectingPolygon(const QPolygon& polygon);
double polygonArea(const QPolygon& polygon);

bool addPoint(QPolygon& polygon, QPoint newPoint, Mode mode);
void finishPolygon(QPolygon& polygon, Mode mode);
PolygonCorrectness polygonCorrectness(const QPolygon& polygon, Mode mode);


#endif // POLYGON_MATH_H
