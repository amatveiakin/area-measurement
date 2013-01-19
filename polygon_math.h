#ifndef POLYGON_MATH_H
#define POLYGON_MATH_H

#include <QPolygonF>

#include "defines.h"

extern const double eps;

static inline double sqr(double x) { return x * x; }

double segmentLenght(QPointF a, QPointF b);
double polylineLength(const QPolygonF& polyline);

bool isSelfintersectingPolygon(const QPolygonF& polygon);
double polygonArea(const QPolygonF& polygon);

bool addPointToPolygon(QPolygonF& polygon, QPointF newPoint, FigureType figureType);
void finishPolygon(QPolygonF& polygon, FigureType figureType);
PolygonCorrectness polygonCorrectness(const QPolygonF& polygon, FigureType figureType);


#endif // POLYGON_MATH_H
