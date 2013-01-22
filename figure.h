#ifndef FIGURE_H
#define FIGURE_H

#include <QColor>
#include <QPolygonF>

#include "defines.h"

class QPainter;
class CanvasWidget;
class SelectionFinder;

extern const QString linearUnitSuffix;
extern const QString squareUnitSuffix;

class Figure
{
public:
  Figure(FigureType figureType, bool isEtalon, const CanvasWidget* canvas);

  FigureType figureType() const     { return figureType_; }
  bool isEtalon() const             { return isEtalon_; }
  bool isFinished() const           { return isFinished_; }
  QPolygonF originalPolygon() const { return originalPolygon_; }

  bool addPoint(QPointF originalNewPoint);
  void finish();

  void testSelection(SelectionFinder& selectionFinder) const;
  void draw(QPainter& painter) const;
  QString statusString() const;

private:
  FigureType figureType_;
  bool isEtalon_;
  bool isFinished_;
  QPolygonF originalPolygon_;
  QPointF originalInscriptionPos_;  // TODO: Use it
  const CanvasWidget* canvas_;
  double size_;   // length or area  // TODO: Use it or delete it
  QColor penColor_;

  QPolygonF getActiveOriginalPolygon(PolygonCorrectness *correctness = 0) const;
  void scalePolygon(QPolygonF& polygon) const;
  void snapPolygonToPixelGrid(QPolygonF& polygon) const;
  QString getSizeString(PolygonCorrectness& correctness) const;
  QString getInscription() const;
  bool isSelected() const;
  bool isHovered() const;
};

#endif // FIGURE_H
