#include <QFileDialog>
#include <QImageReader>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPaintEvent>

// include
#include <QLabel>

#include "mainwindow.h"
#include "ui_mainwindow.h"


const QString appName = "AreaMeasurement";

const QString linearUnit = QString::fromUtf8("м");
const QString squareUnit = linearUnit + QString::fromUtf8("²");

enum Mode
{
  SET_ETALON,
  MEASURE_SEGMENT_LENGTH,
  MEASURE_POLYLINE_LENGTH,
  MEASURE_RECTANGLE_AREA,
  MEASURE_POLYGON_AREA
};

class CanvasWidget : public QWidget
{
public:
  CanvasWidget(const QPixmap* image, QWidget* parent = 0) :
    QWidget(parent),
    image_(image)
  {
    setFixedSize(image_->size());
    setMouseTracking(true);
    mode_ = SET_ETALON;
    nPointsSet_ = 0;
  }

  ~CanvasWidget()
  {
    delete image_;
  }

  virtual void paintEvent(QPaintEvent* event)
  {
    QPainter painter(this);
    painter.drawPixmap(event->rect().topLeft() , *image_, event->rect());
    switch (mode_) {
      case SET_ETALON:
        if (nPointsSet_ == 1) {
          painter.setPen(Qt::gray);
          painter.drawLine(etalon_.p1(), pointUnderMouse_);
        }
        else if (nPointsSet_ == 2) {
          painter.setPen(Qt::blue);
          painter.drawLine(etalon_);
        }
      case MEASURE_SEGMENT_LENGTH:
      case MEASURE_POLYLINE_LENGTH:
      case MEASURE_RECTANGLE_AREA:
      case MEASURE_POLYGON_AREA:
        break;
    }
  }

  virtual void mousePressEvent(QMouseEvent* event)
  {
    if (event->buttons() != Qt::LeftButton)
      return;
    switch (mode_) {
      case SET_ETALON:
        if (nPointsSet_ >= 2)
          nPointsSet_ = 0;
        if (nPointsSet_ == 0)
          etalon_.setP1(event->pos());
        else if (nPointsSet_ == 1) {
          etalon_.setP2(event->pos());
          QInputDialog::getInt(this, appName, QString::fromUtf8("Укажите длину эталона (%1): ").arg(linearUnit), 0, 0);
        }
        nPointsSet_++;
        break;
      case MEASURE_SEGMENT_LENGTH:
      case MEASURE_POLYLINE_LENGTH:
      case MEASURE_RECTANGLE_AREA:
      case MEASURE_POLYGON_AREA:
        break;
    }
  }

  virtual void mouseMoveEvent(QMouseEvent* event)
  {
    pointUnderMouse_ = event->pos();
    update();
  }

private:
  const QPixmap* image_;
  Mode mode_;
  int nPointsSet_;
  QLine etalon_;
  QPoint pointUnderMouse_;
  double etalonLength_;
};


MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setAttribute(Qt::WA_QuitOnClose);
  setWindowTitle(appName);

  QActionGroup* modeActionGroup = new QActionGroup(this);
  QAction* setEtalonAction             = new QAction(QString::fromUtf8("Задание эталона"),                    modeActionGroup);
  QAction* measureSegmentLengthAction  = new QAction(QString::fromUtf8("Измерение длин отрезков"),            modeActionGroup);
  QAction* measurePolylineLengthAction = new QAction(QString::fromUtf8("Измерение длин кривых"),              modeActionGroup);
  QAction* measureRectangleAreaAction  = new QAction(QString::fromUtf8("Измерение площадей прямоугольников"), modeActionGroup);
  QAction* measurePolygonAreaAction    = new QAction(QString::fromUtf8("Измерение площадей многоугольников"), modeActionGroup);
  setEtalonAction            ->setCheckable(true);
  measureSegmentLengthAction ->setCheckable(true);
  measurePolylineLengthAction->setCheckable(true);
  measureRectangleAreaAction ->setCheckable(true);
  measurePolygonAreaAction   ->setCheckable(true);
  setEtalonAction->setChecked(true);
  ui->mainToolBar->addActions(modeActionGroup->actions());

  QList<QByteArray> supportedFormatsList = QImageReader::supportedImageFormats();
  QString supportedFormatsString;
  foreach (const QByteArray& format, supportedFormatsList)
     supportedFormatsString += "*." + QString(format).toLower() + " ";

  QString imageFile = QFileDialog::getOpenFileName (this, QString::fromUtf8("Укажите путь к изображению — ") + appName,
                                                    QString(), QString::fromUtf8("Все изображения (%1)").arg(supportedFormatsString));
  if (imageFile.isEmpty ())
    close();

  QPixmap* image = new QPixmap;
  if (!image->load(imageFile)) {
    QMessageBox::warning(this, appName, QString::fromUtf8("Не могу открыть изображение \"%1\".").arg(imageFile));
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
