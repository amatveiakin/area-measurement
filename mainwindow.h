#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui { class MainWindow; }
class CanvasWidget;

enum Mode
{
  SET_ETALON,
  MEASURE_SEGMENT_LENGTH,
  MEASURE_POLYLINE_LENGTH,
  MEASURE_CLOSED_POLYLINE_LENGTH,
  MEASURE_RECTANGLE_AREA,
  MEASURE_POLYGON_AREA
};

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = 0);
  ~MainWindow();

private:
  Ui::MainWindow* ui;

  CanvasWidget* canvasWidget;

  QAction* setEtalonAction;
  QAction* measureSegmentLengthAction;
  QAction* measurePolylineLengthAction;
  QAction* measureClosedPolylineLengthAction;
  QAction* measureRectangleAreaAction;
  QAction* measurePolygonAreaAction;

  void setMode(Mode newMode);

private slots:
  void updateMode(QAction* modeAction);
};

#endif // MAINWINDOW_H
