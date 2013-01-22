#ifndef POLYGON_MATH_H
#define POLYGON_MATH_H

#include <QLineF>
#include <QPolygonF>

#include "defines.h"

class QPolygonF;

extern const double eps;
extern const double positiveInf;
extern const double negativeInf;

static inline double sqr(double x) { return x * x; }

double segmentLenght(QPointF a, QPointF b);
double polylineLength(QPolygonF polyline);

bool isSelfintersectingPolygon(QPolygonF polygon);
double polygonArea(QPolygonF polygon);

bool addPointToPolygon(QPolygonF& polygon, QPointF newPoint, FigureType figureType);
void finishPolygon(QPolygonF& polygon, FigureType figureType);
PolygonCorrectness polygonCorrectness(QPolygonF polygon, FigureType figureType);

double pointToPointDistance(QPointF point1, QPointF point2);
double pointToSegmentDistance(QPointF point, QLineF line);
double pointToPolylineDistance(QPointF point, QPolygonF polyline);
double pointToPolygonDistance(QPointF point, QPolygonF polygon);

#endif // POLYGON_MATH_H
