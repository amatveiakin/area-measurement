#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>

#include "defines.h"

class MainWindow;
class QLabel;
class QScrollArea;

// in all variables ``original'' prefix means ``in original scale''

class CanvasWidget : public QWidget
{
  Q_OBJECT

public:
  CanvasWidget(const QPixmap* image, MainWindow* mainWindow, QScrollArea* scrollArea, QLabel* scaleLabel, QLabel* statusLabel, QWidget* parent = 0);
  ~CanvasWidget();

  void setMode(Mode newMode);
  bool isEtalonCorrect() const;

public slots:
  void toggleEtalonDefinition(bool etalonMode);
  void toggleRuler(bool showRuler);

private:
  QColor etalonStaticPen_;
  QColor etalonActivePen_;
  QColor staticPen_;
  QColor activePen_;
  QColor errorPen_;

  MainWindow* mainWindow_;
  QScrollArea* scrollArea_;
  QLabel* scaleLabel_;
  QLabel* statusLabel_;
  const QPixmap* originalImage_;  // Owner
  QPixmap image_;

  Mode mode_;
  bool etalonDefinition_;
  bool etalonDefinedRecently_;
  bool showRuler_;

  QList<double> acceptableScales_;
  int iScale_;
  double scale_;

  double originalMetersPerPixel_;
  double metersPerPixel_;

  QPoint originalPointUnderMouse_;
  QPolygon originalPolygon_;
  bool polygonFinished_;
  QPolygon etalonPolygon_;
  Mode etalonPolygonMode_;

  QPoint scrollStartPoint_;
  int scrollStartHValue_;
  int scrollStartVValue_;

private:
  virtual void paintEvent(QPaintEvent* event);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);
  virtual void mouseDoubleClickEvent(QMouseEvent* event);
  virtual bool eventFilter(QObject* object, QEvent* event);

  void getActivePolygon(bool scaled, QPolygon& polygon, Mode& mode, bool& isEtalon, PolygonCorrectness& correctness) const;

  void drawFramed(QPainter& painter, const QList<QRect>& objects, int frameThickness, const QColor& objectsColor, const QColor& frameColor);
  void drawRuler(QPainter& painter, const QRect& rect);

  void finishPlotting();
  void saveEtalonPolygon();
  void resetPolygon();
  void resetAll();
  void scaleChanged();

  void updateStatusText();
  void updateAll();
};

#endif // CANVASWIDGET_H
