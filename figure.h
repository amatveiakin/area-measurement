#ifndef FIGURE_H
#define FIGURE_H

#include <QColor>
#include <QPolygonF>

#include "defines.h"

class QPainter;

extern const QString linearUnitSuffix;
extern const QString squareUnitSuffix;

class Figure
{
public:
  Figure(FigureType figureType, bool isEtalon, const double* originalMetersPerPixel,
         const double* scale, QPointF* originalPointUnderMouse);

  FigureType figureType() const            { return figureType_; }
  bool isEtalon() const                    { return isEtalon_; }
  bool isFinished() const                  { return isFinished_; }
  const QPolygonF& originalPolygon() const { return originalPolygon_; }

  bool addPoint(QPointF originalNewPoint);
  void finish();
  void draw(QPainter& painter) const;
  QString statusString() const;

private:
  FigureType figureType_;
  bool isEtalon_;
  bool isFinished_;
  QPolygonF originalPolygon_;
  QPointF originalInscriptionPos_;  // TODO: Use it
  const double* originalMetersPerPixel_;
  const double* scale_;
  QPointF* originalPointUnderMouse_;
  double size_;   // length or area  // TODO: Use it or delete it
  QColor penColor_;

  QPolygonF getActiveOriginalPolygon(PolygonCorrectness& correctness) const;
  void scalePolygon(QPolygonF& polygon) const;
  QString getSizeString(PolygonCorrectness& correctness) const;
  QString getInscription() const;
};

#endif // FIGURE_H
