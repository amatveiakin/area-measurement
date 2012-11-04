#include <cmath>

#include <QFileDialog>
#include <QImageReader>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPaintEvent>

#include "mainwindow.h"
#include "ui_mainwindow.h"


const QString appName = "AreaMeasurement";

const QString linearUnit = QString::fromUtf8("м");
const QString squareUnit = linearUnit + QString::fromUtf8("²");

static inline double sqr(double x)
{
  return x * x;
}

static inline double segmentLenght(QPoint a, QPoint b)
{
  return std::sqrt(sqr(a.x() - b.x()) + sqr(a.y() - b.y()));
}

static inline double polylineLenght(const QPolygon& polyline)
{
  double length = 0;
  for (int i = 0; i < polyline.size() - 1; i++)
    length += segmentLenght(polyline[i], polyline[i + 1]);
  return length;
}

bool addPoint(QPolygon& polygon, QPoint newPoint, Mode mode, bool userFinishes)
{
  switch (mode) {
    case SET_ETALON:
      abort();

    case MEASURE_SEGMENT_LENGTH:
      polygon.append(newPoint);
      return polygon.size() >= 2;

    case MEASURE_POLYLINE_LENGTH:
    case MEASURE_CLOSED_POLYLINE_LENGTH:
    case MEASURE_POLYGON_AREA:
      polygon.append(newPoint);
      return userFinishes;

    case MEASURE_RECTANGLE_AREA:
      if (polygon.empty()) {
        polygon.append(newPoint);
        return false;
      }
      else {
        polygon = QPolygon(QRect(polygon.first(), newPoint), true);
        return true;
      }
  }
  abort();
}

void finishPolygon(QPolygon& polygon, Mode mode)
{
  if (polygon.isEmpty())
    return;

  switch (mode) {
    case SET_ETALON:
      abort();

    case MEASURE_SEGMENT_LENGTH:
    case MEASURE_POLYLINE_LENGTH:
    case MEASURE_RECTANGLE_AREA:
    case MEASURE_POLYGON_AREA:
      return;

    case MEASURE_CLOSED_POLYLINE_LENGTH:
      polygon.append(polygon.first());
      return;
  }
  abort();
}

class CanvasWidget : public QWidget
{
public:
  CanvasWidget(const QPixmap* image, QWidget* parent = 0) :
    QWidget(parent),
    image_(image)
  {
    etalonStaticPen_ = QColor(0, 150, 0);
    etalonActivePen_ = QColor(0, 200, 0);
    staticPen_  = QColor(0,  50, 240);
    activePen_  = QColor(0, 100, 240);
    staticFill_ = staticPen_;
    staticFill_.setAlpha(100);
    activeFill_ = activePen_;
    activeFill_.setAlpha(100);

    setFixedSize(image_->size());
    setMouseTracking(true);
    mode_ = SET_ETALON;
    nEthalonPointsSet_ = 0;
    polygonFinished_ = true;
  }

  ~CanvasWidget()
  {
    delete image_;
  }

  virtual void paintEvent(QPaintEvent* event)
  {
    QPainter painter(this);
    painter.drawPixmap(event->rect().topLeft() , *image_, event->rect());

    if (mode_ == SET_ETALON) {
      if (nEthalonPointsSet_ == 2) {
        painter.setPen(etalonStaticPen_);
        painter.drawLine(etalon_);
      }
      else if (nEthalonPointsSet_ == 1) {
        painter.setPen(etalonActivePen_);
        painter.drawLine(etalon_.p1(), pointUnderMouse_);
      }
    }
    else {
      QPolygon drawPolygon = polygon_;
      if (polygonFinished_) {
        painter.setPen(staticPen_);
        painter.setBrush(staticFill_);
      }
      else {
        painter.setPen(activePen_);
        painter.setBrush(activeFill_);
        addPoint(drawPolygon, pointUnderMouse_, mode_, false);
        finishPolygon(drawPolygon, mode_);
      }

      switch (mode_) {
        case SET_ETALON:
          abort();

        case MEASURE_SEGMENT_LENGTH:
        case MEASURE_POLYLINE_LENGTH:
        case MEASURE_CLOSED_POLYLINE_LENGTH:
          painter.drawPolyline(drawPolygon);
          break;

        case MEASURE_RECTANGLE_AREA:
        case MEASURE_POLYGON_AREA:
          painter.drawPolygon(drawPolygon);
          break;
      }
    }
  }

  virtual void mousePressEvent(QMouseEvent* event)
  {
    QPoint newPoint = event->pos();

    if (polygonFinished_)
      polygon_.clear();

    if (mode_ == SET_ETALON) {
      if (nEthalonPointsSet_ >= 2)
        nEthalonPointsSet_ = 0;
      if (nEthalonPointsSet_ == 0)
        etalon_.setP1(newPoint);
      else if (nEthalonPointsSet_ == 1) {
        bool ok;
        etalon_.setP2(newPoint);
        etalonLength_ = QInputDialog::getDouble(this, appName, QString::fromUtf8("Укажите длину эталона (%1): ").arg(linearUnit), 0, 0, 1e9, 3, &ok);
        if (!ok)
          nEthalonPointsSet_ = -1;
        else
          metersPerPixel_ = segmentLenght(etalon_.p1(), etalon_.p2());
      }
      nEthalonPointsSet_++;
    }
    else {
      polygonFinished_ = addPoint(polygon_, newPoint, mode_, event->buttons() == Qt::RightButton);
      if (polygonFinished_)
        finishPolygon(polygon_, mode_);
    }
  }

  virtual void mouseMoveEvent(QMouseEvent* event)
  {
    pointUnderMouse_ = event->pos();
    update();
  }

  void setMode(Mode newMode)
  {
    mode_ = newMode;
    polygon_.clear();
  }

private:
  QColor etalonStaticPen_;
  QColor etalonActivePen_;
  QColor staticPen_;
  QColor staticFill_;
  QColor activePen_;
  QColor activeFill_;

  const QPixmap* image_;
  Mode mode_;

  int nEthalonPointsSet_;
  QLine etalon_;
  double etalonLength_;
  double metersPerPixel_;

  QPoint pointUnderMouse_;
  QPolygon polygon_;
  bool polygonFinished_;
};


MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setAttribute(Qt::WA_QuitOnClose);
  setWindowTitle(appName);

  QActionGroup* modeActionGroup = new QActionGroup(this);
  setEtalonAction                   = new QAction(QString::fromUtf8("Задание эталона"),                    modeActionGroup);
  measureSegmentLengthAction        = new QAction(QString::fromUtf8("Измерение длин отрезков"),            modeActionGroup);
  measurePolylineLengthAction       = new QAction(QString::fromUtf8("Измерение длин кривых"),              modeActionGroup);
  measureClosedPolylineLengthAction = new QAction(QString::fromUtf8("Измерение длин замкнутых кривых"),    modeActionGroup);
  measureRectangleAreaAction        = new QAction(QString::fromUtf8("Измерение площадей прямоугольников"), modeActionGroup);
  measurePolygonAreaAction          = new QAction(QString::fromUtf8("Измерение площадей многоугольников"), modeActionGroup);
  foreach (QAction* action, modeActionGroup->actions())
    action->setCheckable(true);
  setEtalonAction->setChecked(true);
  ui->mainToolBar->addActions(modeActionGroup->actions());
  connect (modeActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(updateMode(QAction*)));

  QList<QByteArray> supportedFormatsList = QImageReader::supportedImageFormats();
  QString supportedFormatsString;
  foreach (const QByteArray& format, supportedFormatsList)
     supportedFormatsString += "*." + QString(format).toLower() + " ";

  QString imageFile = QFileDialog::getOpenFileName (this, QString::fromUtf8("Укажите путь к изображению — ") + appName,
                                                    QString(), QString::fromUtf8("Все изображения (%1)").arg(supportedFormatsString));
  if (imageFile.isEmpty ()) {
    close(); // ???
    return;
  }

  QPixmap* image = new QPixmap;
  if (!image->load(imageFile)) {
    QMessageBox::warning(this, appName, QString::fromUtf8("Не могу открыть изображение \"%1\".").arg(imageFile));
    delete image;
    close(); // ???
    return;
  }

  canvasWidget = new CanvasWidget(image, this);
  ui->containingScrollArea->setBackgroundRole(QPalette::Dark);
  ui->containingScrollArea->setWidget(canvasWidget);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::setMode(Mode newMode)
{
  canvasWidget->setMode(newMode);
}

void MainWindow::updateMode(QAction* modeAction)
{
  if (modeAction == setEtalonAction)
    return setMode (SET_ETALON);
  if (modeAction == measureSegmentLengthAction)
    return setMode (MEASURE_SEGMENT_LENGTH);
  if (modeAction == measurePolylineLengthAction)
    return setMode (MEASURE_POLYLINE_LENGTH);
  if (modeAction == measureClosedPolylineLengthAction)
    return setMode (MEASURE_CLOSED_POLYLINE_LENGTH);
  if (modeAction == measureRectangleAreaAction)
    return setMode (MEASURE_RECTANGLE_AREA);
  if (modeAction == measurePolygonAreaAction)
    return setMode (MEASURE_POLYGON_AREA);
  abort();
}
