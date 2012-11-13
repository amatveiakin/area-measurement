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
  QList<int> appVersion() const;
  QString appVersionString() const;

  void setMode(Mode newMode);

public slots:
  void toggleEtalonDefinition(bool isDefiningEtalon);

private:
  Ui::MainWindow* ui;

  CanvasWidget* canvasWidget;

  QAction* toggleEtalonModeAction;
  QAction* measureSegmentLengthAction;
  QAction* measurePolylineLengthAction;
  QAction* measureClosedPolylineLengthAction;
  QAction* measureRectangleAreaAction;
  QAction* measurePolygonAreaAction;
  QAction* toggleRulerAction;
  QAction* aboutAction;

private slots:
  void updateMode(QAction* modeAction);
  void showAbout();
};

#endif // MAINWINDOW_H
