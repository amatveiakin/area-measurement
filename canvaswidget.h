#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QLinkedList>
#include <QWidget>

#include "defines.h"
#include "figure.h"
#include "selection.h"

class MainWindow;
class QLabel;
class QScrollArea;

// in all variables ``original'' prefix means ``in original scale''
//TODO: change naming, it's counterintuitive

class CanvasWidget : public QWidget
{
  Q_OBJECT

public:
  CanvasWidget(const QPixmap& image, MainWindow* mainWindow, QScrollArea* scrollArea,
               QLabel* scaleLabel, QLabel* statusLabel, QWidget* parent = 0);
  ~CanvasWidget();

  void setMode(ShapeType newMode);
  bool hasEtalon() const;
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
  QPixmap originalImage_;
  QPixmap image_;

  // Current state
  ShapeType shapeType_;
  bool isDefiningEtalon_;
  bool showRuler_;

  // Scale
  QList<double> acceptableScales_;
  int iScale_;
  double scale_;

  // Length etalon
  double etalonMetersSize_;
  double originalMetersPerPixel_;
  double metersPerPixel_;

  // Drawings
  QPointF pointUnderMouse_;
  QPointF originalPointUnderMouse_;
  QLinkedList<Figure> figures_;  // We want pointers not to be invalidated after insertions

  // Current state
  Figure* etalonFigure_;
  Figure* activeFigure_;
  Selection selection_;
  Selection hover_;

  // Scroll
  QPoint scrollStartPoint_;
  int scrollStartHValue_;
  int scrollStartVValue_;

private:
  virtual void paintEvent(QPaintEvent* event);
  virtual void keyPressEvent(QKeyEvent* event);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);
  virtual void mouseDoubleClickEvent(QMouseEvent* event);
  virtual bool eventFilter(QObject* object, QEvent* event);

  void addActiveFigure();
  void removeFigure(const Figure* figure);

  void drawRuler(QPainter& painter, const QRect& rect);

  void updateMousePos(QPoint mousePos);
  void updateHover();
  void updateStatus();
  void defineEtalon(Figure* etalonFigure);
  void clearEtalon(bool invalidateOnly = false);
  void finishDrawing();
  void resetAll();
  void scaleChanged();
  void updateAll();

  friend class Figure;
};

#endif // CANVASWIDGET_H
