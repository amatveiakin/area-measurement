#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "defines.h"

namespace Ui { class MainWindow; }
class CanvasWidget;
class QActionGroup;
class QLabel;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = 0);
  ~MainWindow();

  QString companyName() const;
  QString appName() const;
  QList<int> appVersion() const;
  QString appVersionString() const;

  QFont getInscriptionFont() const;
  void setMode(FigureType newMode);

public slots:
  void toggleEtalonDefinition(bool isDefiningEtalon);

private:
  QStringList recentFiles;
  QFont inscriptionFont;

  Ui::MainWindow* ui;

  QMenu* openRecentMenu;
  QLabel* scaleLabel;
  QLabel* statusLabel;
  CanvasWidget* canvasWidget;

  QActionGroup* modeActionGroup;
  QAction* openFileAction;
  QAction* toggleEtalonModeAction;
  QAction* measureSegmentLengthAction;
  QAction* measurePolylineLengthAction;
  QAction* measureClosedPolylineLengthAction;
  QAction* measureRectangleAreaAction;
  QAction* measurePolygonAreaAction;
  QAction* toggleRulerAction;
  QAction* customizeInscriptionFontAction;
  QAction* aboutAction;

  void doOpenFile(const QString& filename);
  void loadSettings();
  void loadAndApplySettings();
  void saveSettings() const;
  void updateOpenRecentMenu();

private slots:
  void openNewFile();
  void openRecentFile();
  void setDrawOptionsEnabled(bool enabled);
  void updateMode(QAction* modeAction);
  void customizeInscriptionFont();
  void showAbout();
};

#endif // MAINWINDOW_H
