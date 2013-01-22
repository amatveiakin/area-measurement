#ifndef SELECTION_H
#define SELECTION_H

#include <QLineF>
#include <QPolygonF>

class Figure;

struct Selection
{
  enum Type
  {
    FIGURE,
    VERTEX,
    INSCRIPTION
  };

  const Figure* figure;
  Type          type;
  int           iVertex;

  Selection();

  void reset();
  void assign(const Figure* figure__, Type type__, int iVertex__);

  bool operator==(const Selection& other);
  bool operator!=(const Selection& other);

  bool isEmpty()  { return !figure; }
  bool isDraggable() const;  // TODO: use
};

class SelectionFinder
{
public:
  SelectionFinder(QPointF cursorPos);

  void testPolygon(QPolygonF polygon, const Figure* figure);
  void testPolyline(QPolygonF polyline, const Figure* figure);
  void testVertex(QPointF vertex, const Figure* figure, int iVertex);
  void testInscription(QRectF boundingRect, const Figure* figure);

  //bool hasSelection() const               { return bestScore_ > 0.; }
  const Selection& bestSelection() const  { return bestSelection_; }

private:
  QPointF   cursorPos_;
  Selection bestSelection_;
  double    bestScore_;
};

#endif // SELECTION_H
