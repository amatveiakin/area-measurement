// TODO: Use Q_ASSERT instead of assert (?)

#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(":/pictures/polygon_area.png"));
  MainWindow window;
  window.show();

  return app.exec();
}
