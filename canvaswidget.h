#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>

#include "modes.h"

class MainWindow;
class QLabel;
class QScrollArea;

// in all variables ``original'' prefix means ``in original scale''

class CanvasWidget : public QWidget
{
public:
  CanvasWidget(const QPixmap* image, MainWindow* mainWindow, QScrollArea* scrollArea, QLabel* scaleLabel, QLabel* statusLabel, QWidget* parent = 0);
  ~CanvasWidget();

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
  QLabel* scaleLabel_;
  QLabel* statusLabel_;
  const QPixmap* originalImage_;  // Owner
  QPixmap image_;
  Mode mode_;
  QList<double> acceptableScales_;
  int iScale_;
  double scale_;

  int nEthalonPointsSet_;
  QLine originalEtalon_;
  double etalonMetersLength_;
  double originalMetersPerPixel_;
  double metersPerPixel_;

  QPoint originalPointUnderMouse_;
  QPolygon originalPolygon_;
  bool polygonFinished_;

  QPoint scrollStartPoint_;
  int scrollStartHValue_;
  int scrollStartVValue_;

private:
  virtual void paintEvent(QPaintEvent* event);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);
  virtual bool eventFilter(QObject* object, QEvent* event);

  QPolygon getActivePolygon(bool scaled) const;

  void drawFramed(QPainter& painter, const QList<QRect>& objects, int frameThickness, const QColor& objectsColor, const QColor& frameColor);
  void drawRuler(QPainter& painter, const QRect& rect);

  void resetEtalon();
  void resetPolygon();
  void scaleChanged();

  void updateStatusText();
  void updateAll();
};

#endif // CANVASWIDGET_H
