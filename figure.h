#ifndef FIGURE_H
#define FIGURE_H

#include <QColor>
#include <QPolygonF>

#include "defines.h"
#include "shape.h"

class QPainter;
class CanvasWidget;
class Selection;
class SelectionFinder;

extern const QString linearUnitSuffix;
extern const QString squareUnitSuffix;

class Figure
{
public:
  Figure(ShapeType shapeType, bool isEtalon, const CanvasWidget* canvas);

  bool isEtalon() const             { return isEtalon_; }
  bool isFinished() const           { return originalShape_.isFinished(); }
  ShapeType shapeType() const       { return originalShape_.type(); }
  Shape originalShape() const       { return originalShape_; }

  bool addPoint(QPointF originalNewPoint);
  void finish();

  void testSelection(SelectionFinder& selectionFinder);  // for a closed polygon return first (not last) vertex
  void dragTo(const Selection& selection, QPointF newPos);
  void draw(QPainter& painter) const;
  QString statusString() const;

private:
  Shape originalShape_;
  bool isEtalon_;
  QPointF originalInscriptionPos_;  // TODO: Use it
  const CanvasWidget* canvas_;
  double size_;   // length or area  // TODO: Use it or delete it
  QColor penColor_;

  Shape getActiveOriginalShape() const;
  void snapPolygonToPixelGrid(QPolygonF& polygon) const;
  QString getSizeString(ShapeCorrectness& correctness) const;
  QString getInscription() const;
  bool isSelected() const;
  bool isHovered() const;
  int hoveredVertex() const;  // -1 if not hovered
};

#endif // FIGURE_H
