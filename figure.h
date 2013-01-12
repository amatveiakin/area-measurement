#ifndef FIGURE_H
#define FIGURE_H

#include <QColor>
#include <QPolygon>

#include "defines.h"

class QPainter;

extern const QString linearUnitSuffix;
extern const QString squareUnitSuffix;

class Figure
{
public:
  Figure(FigureType figureType, bool isEtalon, const double* originalMetersPerPixel,
         const double* scale, QPoint* originalPointUnderMouse);

  FigureType figureType() const           { return figureType_; }
  bool isEtalon() const                   { return isEtalon_; }
  bool isFinished() const                 { return isFinished_; }
  const QPolygon& originalPolygon() const { return originalPolygon_; }

  bool addPoint(QPoint originalNewPoint);
  void finish();
  void draw(QPainter& painter) const;
  QString statusString() const;

private:
  static QColor etalonStaticPen_;
  static QColor etalonActivePen_;
  static QColor staticPen_;
  static QColor activePen_;
  static QColor errorPen_;

  FigureType figureType_;
  bool isEtalon_;
  bool isFinished_;
  QPolygon originalPolygon_;
  QPoint originalInscriptionPos_;  // TODO: Use it
  const double* originalMetersPerPixel_;
  const double* scale_;
  QPoint* originalPointUnderMouse_;
  double size_;   // length or area
  QColor penColor_;

  QPolygon getActiveOriginalPolygon(PolygonCorrectness& correctness) const;
  void scalePolygon(QPolygon& polygon) const;
  QString getSizeString(PolygonCorrectness& correctness) const;
  QString getInscription() const;
};

#endif // FIGURE_H
