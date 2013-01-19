#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>

#include "defines.h"
#include "figure.h"

class MainWindow;
class QLabel;
class QScrollArea;

// in all variables ``original'' prefix means ``in original scale''
//TODO: change naming, it's counterintuitive

class CanvasWidget : public QWidget
{
  Q_OBJECT

public:
  CanvasWidget(const QPixmap* image, MainWindow* mainWindow, QScrollArea* scrollArea,
               QLabel* scaleLabel, QLabel* statusLabel, QWidget* parent = 0);
  ~CanvasWidget();

  void setMode(FigureType newMode);
  bool isEtalonCorrect() const;
  QPixmap getModifiedImage();

public slots:
  void toggleEtalonDefinition(bool isDefiningEtalon);
  void toggleRuler(bool showRuler);

private:
  // Global
  MainWindow* mainWindow_;
  QScrollArea* scrollArea_;
  QLabel* scaleLabel_;
  QLabel* statusLabel_;
  const QPixmap* originalImage_;  // Owner
  QPixmap image_;

  // Current state
  FigureType figureType_;
  bool isDefiningEtalon_;
  bool showRuler_;

  // Scale
  QList<double> acceptableScales_;
  int iScale_;
  double scale_;

  // Length etalon
  double originalMetersPerPixel_;
  double metersPerPixel_;

  // Drawings
  QPointF originalPointUnderMouse_;
  QList<Figure> figures_;

  // Scroll
  QPoint scrollStartPoint_;
  int scrollStartHValue_;
  int scrollStartVValue_;

private:
  virtual void paintEvent(QPaintEvent* event);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);
  virtual void mouseDoubleClickEvent(QMouseEvent* event);
  virtual bool eventFilter(QObject* object, QEvent* event);

  Figure& activeFigure();

  void drawRuler(QPainter& painter, const QRect& rect);

  void finishPlotting();
  Figure newFigure(bool isEtalon);
  void resetAll();
  void scaleChanged();
  void updateAll();
};

#endif // CANVASWIDGET_H
