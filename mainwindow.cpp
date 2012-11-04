#include <QFileDialog>
#include <QImageReader>
#include <QMessageBox>
#include <QPainter>
#include <QPaintEvent>

// include
#include <QLabel>

#include "mainwindow.h"
#include "ui_mainwindow.h"


const QString appName = "AreaMeasurement";

class CanvasWidget : public QWidget
{
public:
  CanvasWidget(const QPixmap* image, QWidget* parent = 0) :
    QWidget(parent),
    image_(image)
  {
    setFixedSize(image_->size());
  }

  ~CanvasWidget()
  {
    delete image_;
  }

  virtual void paintEvent(QPaintEvent* event)
  {
    QPainter painter(this);
    painter.drawPixmap(event->rect().topLeft() , *image_, event->rect());
  }

private:
  const QPixmap* image_;
};


MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setAttribute(Qt::WA_QuitOnClose);

  QList<QByteArray> supportedFormatsList = QImageReader::supportedImageFormats();
  QString supportedFormatsString;
  foreach (const QByteArray& format, supportedFormatsList)
     supportedFormatsString += "*." + QString(format).toLower() + " ";

  QString imageFile = QFileDialog::getOpenFileName (this, QString::fromUtf8("Укажите путь к изображению — ") + appName,
                                                    QString(), QString::fromUtf8("Все изображения (") + supportedFormatsString + ")");
  if (imageFile.isEmpty ())
    close();

  QPixmap* image = new QPixmap;
  if (!image->load(imageFile)) {
    QMessageBox::warning(this, appName, QString::fromUtf8("Не могу открыть изображение \"") + imageFile + "\".");
    delete image;
    close();
  }

  CanvasWidget* canvasWidget = new CanvasWidget(image, this);
  ui->containingScrollArea->setBackgroundRole(QPalette::Dark);
  ui->containingScrollArea->setWidget(canvasWidget);
}

MainWindow::~MainWindow()
{
  delete ui;
}
