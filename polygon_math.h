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

bool addPointToPolygon(QPolygon& polygon, QPoint newPoint, FigureType figureType);
void finishPolygon(QPolygon& polygon, FigureType figureType);
PolygonCorrectness polygonCorrectness(const QPolygon& polygon, FigureType figureType);


#endif // POLYGON_MATH_H
