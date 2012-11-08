#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "modes.h"

namespace Ui { class MainWindow; }
class CanvasWidget;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = 0);
  ~MainWindow();

  QString appName() const;

  void setMode(Mode newMode);
  void setMeasurementEnabled(bool state);

private:
  Ui::MainWindow* ui;

  CanvasWidget* canvasWidget;

  QAction* setEtalonAction;
  QAction* measureSegmentLengthAction;
  QAction* measurePolylineLengthAction;
  QAction* measureClosedPolylineLengthAction;
  QAction* measureRectangleAreaAction;
  QAction* measurePolygonAreaAction;
  QAction* toggleRulerAction;

private slots:
  void updateMode(QAction* modeAction);
};

#endif // MAINWINDOW_H
