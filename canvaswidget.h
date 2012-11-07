#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>

#include "modes.h"

class MainWindow;
class QLabel;
class QScrollArea;

class CanvasWidget : public QWidget
{
public:
  CanvasWidget(const QPixmap* image, MainWindow* mainWindow, QScrollArea* scrollArea, QLabel* statusLabel, QWidget* parent = 0);
  ~CanvasWidget();

  virtual void paintEvent(QPaintEvent* event);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);
//  virtual void wheelEvent(QWheelEvent* event)   // TODO: Wheel zoom

  void setMode(Mode newMode);

private:
  QColor etalonStaticPen_;
  QColor etalonActivePen_;
  QColor staticPen_;
  QColor staticFill_;
  QColor activePen_;
  QColor activeFill_;
  QColor errorPen_;
  QColor errorFill_;

  MainWindow* mainWindow_;
  QScrollArea* scrollArea_;
  QLabel* statusLabel_;
  const QPixmap* image_;
  Mode mode_;

  int nEthalonPointsSet_;
  QLine etalon_;
  double etalonLength_;
  double metersPerPixel_;

  QPoint pointUnderMouse_;
  QPolygon polygon_;
  bool polygonFinished_;

  QPoint scrollStartPoint_;
  int scrollStartHValue_;
  int scrollStartVValue_;

private:
  QPolygon getActivePolygon() const;

  void drawFramed(QPainter& painter, const QList<QRect>& objects, int frameThickness, const QColor& objectsColor, const QColor& frameColor);
  void drawRuler(QPainter& painter, const QRect& rect);

  void resetEtalon();
  void resetPolygon();

  void updateStatusText();
  void updateAll();
};

#endif // CANVASWIDGET_H
