#include <cmath>

#include <QFileDialog>
#include <QImageReader>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPaintEvent>
#include <QScrollBar>

#include "mainwindow.h"
#include "ui_mainwindow.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Misc

const double eps = 1e-6;


const QString appName = "AreaMeasurement";

const QString linearUnit = QString::fromUtf8("м");
const QString squareUnit = linearUnit + QString::fromUtf8("²");


static inline ModeKind getModeKind(Mode mode)
{
  switch (mode) {
    case SET_ETALON:
      return ETALON;
    case MEASURE_SEGMENT_LENGTH:
    case MEASURE_POLYLINE_LENGTH:
    case MEASURE_CLOSED_POLYLINE_LENGTH:
      return LENGTH;
    case MEASURE_POLYGON_AREA:
    case MEASURE_RECTANGLE_AREA:
      return AREA;
  }
  abort();
}


static inline double sqr(double x)
{
  return x * x;
}


static inline bool fuzzyEq(QPoint a, QPoint b)
{
  return (a - b).manhattanLength() <= eps;
}


QColor getFillColor(QColor penColor)
{
  QColor fillColor = penColor;
  fillColor.setAlpha(100);
  return fillColor;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Length

static inline double segmentLenght(QPoint a, QPoint b)
{
  return std::sqrt(sqr(a.x() - b.x()) + sqr(a.y() - b.y()));
}

double polylineLenght(const QPolygon& polyline)
{
  double length = 0;
  for (int i = 0; i < polyline.size() - 1; i++)
    length += segmentLenght(polyline[i], polyline[i + 1]);
  return length;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Area

void assertPolygonIsClosed(const QPolygon& polygon)
{
  if (!polygon.isEmpty() && !fuzzyEq(polygon.first(), polygon.last()))
    abort();
}

static inline bool testSegmentsCross(QPoint a, QPoint b, QPoint c, QPoint d)
{
  return QLineF(a, b).intersect(QLineF(c, d), 0) == QLineF::BoundedIntersection;
}

bool isSelfintersectingPolygon(const QPolygon& polygon)
{
  assertPolygonIsClosed(polygon);
  int n = polygon.size() - 1;  // cut off last vertex
  for (int i1 = 0; i1 < n; i1++) {
    int i2 = (i1 + 1) % n;
    for (int j1 = 0; j1 < n; j1++) {
      int j2 = (j1 + 1) % n;
      if (i1 != j1 && i1 != j2 && i2 != j1
          && testSegmentsCross(polygon[i1], polygon[i2], polygon[j1], polygon[j2]))
        return true;
    }
  }
  return false;
}

static inline double triangleSignedArea(QPoint a, QPoint b, QPoint c)
{
  QPoint p = b - a;
  QPoint q = c - a;
  return (p.x() * q.y() - p.y() * q.x()) / 2.0;
}

double polygonArea(const QPolygon& polygon)
{
  assertPolygonIsClosed(polygon);
  double area = 0;
  for (int i = 1; i < polygon.size() - 2; i++)
    area += triangleSignedArea(polygon[0], polygon[i], polygon[i + 1]);
  return qAbs(area);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Polygon

bool addPoint(QPolygon& polygon, QPoint newPoint, Mode mode)
{
  switch (mode) {
    case SET_ETALON: {
      abort();
    }

    case MEASURE_SEGMENT_LENGTH: {
      polygon.append(newPoint);
      return polygon.size() >= 2;
    }

    case MEASURE_POLYLINE_LENGTH:
    case MEASURE_CLOSED_POLYLINE_LENGTH:
    case MEASURE_POLYGON_AREA: {
      bool finish = (!polygon.isEmpty() && polygon.back() == newPoint);
      if (!finish)
        polygon.append(newPoint);
      return finish;
    }

    case MEASURE_RECTANGLE_AREA: {
      if (polygon.empty()) {
        polygon.append(newPoint);
        return false;
      }
      else {
        polygon = QPolygon(QRect(polygon.first(), newPoint), true);
        return true;
      }
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
      return;

    case MEASURE_CLOSED_POLYLINE_LENGTH:
    case MEASURE_POLYGON_AREA:
      polygon.append(polygon.first());
      return;
  }
  abort();
}

bool isValidPolygon(const QPolygon& polygon, Mode mode)
{
  switch (mode) {
    case SET_ETALON:
      abort();

    case MEASURE_SEGMENT_LENGTH:
    case MEASURE_POLYLINE_LENGTH:
    case MEASURE_CLOSED_POLYLINE_LENGTH:
    case MEASURE_RECTANGLE_AREA:
      return true;

    case MEASURE_POLYGON_AREA:
      return !isSelfintersectingPolygon(polygon);
  }
  abort();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CanvasWidget

class CanvasWidget : public QWidget
{
public:
  CanvasWidget(const QPixmap* image, MainWindow* mainWindow, QScrollArea* scrollArea, QLabel* statusLabel, QWidget* parent = 0) :
    QWidget(parent),
    mainWindow_(mainWindow),
    scrollArea_(scrollArea),
    statusLabel_(statusLabel),
    image_(image)
  {
    etalonStaticPen_ = QColor(0, 150, 0);
    etalonActivePen_ = QColor(0, 200, 0);
    staticPen_  = QColor(  0,  50, 240);
    activePen_  = QColor(  0, 100, 240);
    errorPen_   = QColor(255,   0,   0);
    staticFill_ = getFillColor(staticPen_);
    activeFill_ = getFillColor(activePen_);
    errorFill_  = getFillColor(errorPen_);

    setFixedSize(image_->size());
    setMouseTracking(true);
    mode_ = SET_ETALON;
    resetEtalon();
    resetPolygon();
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
      QPolygon activePolygon = getActivePolygon();
      if (!isValidPolygon(activePolygon, mode_)) {
        painter.setPen(errorPen_);
        painter.setBrush(errorFill_);
      }
      else if (polygonFinished_) {
        painter.setPen(staticPen_);
        painter.setBrush(staticFill_);
      }
      else {
        painter.setPen(activePen_);
        painter.setBrush(activeFill_);
      }
      if (getModeKind(mode_) == LENGTH)
        painter.drawPolyline(activePolygon);
      else
        painter.drawPolygon(activePolygon);
    }
  }

  virtual void mousePressEvent(QMouseEvent* event)
  {
    if (event->buttons() == Qt::LeftButton) {
      QPoint newPoint = event->pos();

      if (polygonFinished_)
        resetPolygon();

      if (mode_ == SET_ETALON) {
        if (nEthalonPointsSet_ >= 2)
          resetEtalon();

        if (nEthalonPointsSet_ == 0) {
          etalon_.setP1(newPoint);
          nEthalonPointsSet_ = 1;
        }
        else if (nEthalonPointsSet_ == 1) {
          bool ok;
          etalon_.setP2(newPoint);
          etalonLength_ = QInputDialog::getDouble(this, appName, QString::fromUtf8("Укажите длину эталона (%1): ").arg(linearUnit), 1., 0.001, 1e9, 3, &ok);
          double etalonPixelLength_ = segmentLenght(etalon_.p1(), etalon_.p2());
          if (ok && etalonPixelLength_ > eps) {
            metersPerPixel_ = etalonLength_ / etalonPixelLength_;
            nEthalonPointsSet_ = 2;
            mainWindow_->setMeasurementEnabled(true);
          }
          else
            resetEtalon();
        }
      }
      else {
        polygonFinished_ = addPoint(polygon_, newPoint, mode_);
        if (polygonFinished_)
          finishPolygon(polygon_, mode_);
      }
      updateAll();
    }
    else if (event->buttons() == Qt::RightButton) {
      scrollStartPoint_ = event->globalPos();
      scrollStartHValue_ = scrollArea_->horizontalScrollBar()->value();
      scrollStartVValue_ = scrollArea_->verticalScrollBar()  ->value();
    }
  }

  virtual void mouseMoveEvent(QMouseEvent* event)
  {
    if (event->buttons() == Qt::NoButton) {
      pointUnderMouse_ = event->pos();
      updateAll();
    }
    else if (event->buttons() == Qt::RightButton) {
      QPoint scrollBy = event->globalPos() - scrollStartPoint_;
      scrollArea_->horizontalScrollBar()->setValue(scrollStartHValue_ - scrollBy.x());
      scrollArea_->verticalScrollBar()  ->setValue(scrollStartVValue_ - scrollBy.y());
    }
  }

//  virtual void wheelEvent(QWheelEvent* event)
//  {
//    // TODO: Wheel zoom
//  }

  QPolygon getActivePolygon()
  {
    QPolygon activePolygon = polygon_;
    if (!polygonFinished_) {
      addPoint(activePolygon, pointUnderMouse_, mode_);
      finishPolygon(activePolygon, mode_);
    }
    return activePolygon;
  }

  void resetEtalon()
  {
    nEthalonPointsSet_ = 0;
    etalon_ = QLine();
    etalonLength_ = 0.;
    metersPerPixel_ = 0.;
    mainWindow_->setMeasurementEnabled(false);
  }

  void resetPolygon()
  {
    polygon_.clear();
    polygonFinished_ = false;
  }

  void setMode(Mode newMode)
  {
    mode_ = newMode;
    polygon_.clear();
    updateAll();
  }

  void updateStatusText()
  {
    switch (getModeKind(mode_)) {
      case ETALON: {
        statusLabel_->setText(etalonLength_ > eps ? QString::fromUtf8("Длина эталона: %1 %2").arg(etalonLength_).arg(linearUnit) : QString());
        break;
      }
      case LENGTH: {
        double length = polylineLenght(getActivePolygon()) * metersPerPixel_;
        statusLabel_->setText(length > eps ? QString::fromUtf8("Длина: %1 %2").arg(length).arg(linearUnit) : QString());
        break;
      }
      case AREA: {
        QPolygon activePolygon = getActivePolygon();
        if (isValidPolygon(activePolygon, mode_)) {
          double area = polygonArea(activePolygon) * sqr(metersPerPixel_);
          statusLabel_->setText(area > eps ? QString::fromUtf8("Площадь: %1 %2").arg(area).arg(squareUnit) : QString());
        }
        else {
          statusLabel_->setText(QString::fromUtf8("Многоугольник не должен самопересекаться!"));
        }
        break;
      }
    }
  }

  void updateAll()
  {
    updateStatusText();
    update();
  }

private:
  QColor etalonStaticPen_;
  QColor etalonActivePen_;
  QColor staticPen_;
  QColor staticFill_;
  QColor activePen_;
  QColor activeFill_;
  QColor errorPen_;
  QColor errorFill_;

  MainWindow* mainWindow_;
  QScrollArea* scrollArea_;
  QLabel* statusLabel_;
  const QPixmap* image_;
  Mode mode_;

  int nEthalonPointsSet_;
  QLine etalon_;
  double etalonLength_;
  double metersPerPixel_;

  QPoint pointUnderMouse_;
  QPolygon polygon_;
  bool polygonFinished_;

  QPoint scrollStartPoint_;
  int scrollStartHValue_;
  int scrollStartVValue_;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MainWindow

MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setAttribute(Qt::WA_QuitOnClose);
  setWindowTitle(appName);

  QActionGroup* modeActionGroup = new QActionGroup(this);
  setEtalonAction                   = new QAction(QIcon(":/pictures/etalon.png"),                 QString::fromUtf8("Задание эталона"),                    modeActionGroup);
  measureSegmentLengthAction        = new QAction(QIcon(":/pictures/segment_length.svg"),         QString::fromUtf8("Измерение длин отрезков"),            modeActionGroup);
  measurePolylineLengthAction       = new QAction(QIcon(":/pictures/polyline_length.svg"),        QString::fromUtf8("Измерение длин кривых"),              modeActionGroup);
  measureClosedPolylineLengthAction = new QAction(QIcon(":/pictures/closed_polyline_length.svg"), QString::fromUtf8("Измерение длин замкнутых кривых"),    modeActionGroup);
  measureRectangleAreaAction        = new QAction(QIcon(":/pictures/rectangle_area.svg"),         QString::fromUtf8("Измерение площадей прямоугольников"), modeActionGroup);
  measurePolygonAreaAction          = new QAction(QIcon(":/pictures/polygon_area.svg"),           QString::fromUtf8("Измерение площадей многоугольников"), modeActionGroup);
  foreach (QAction* action, modeActionGroup->actions())
    action->setCheckable(true);
  setEtalonAction->setChecked(true);
  ui->mainToolBar->addActions(modeActionGroup->actions());
  ui->mainToolBar->setIconSize(QSize(32, 32));
  ui->mainToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
  setMeasurementEnabled(false);
  connect(modeActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(updateMode(QAction*)));

  QList<QByteArray> supportedFormatsList = QImageReader::supportedImageFormats();
  QString supportedFormatsString;
  foreach (const QByteArray& format, supportedFormatsList)
     supportedFormatsString += "*." + QString(format).toLower() + " ";

  QString imageFile = QFileDialog::getOpenFileName (this, QString::fromUtf8("Укажите путь к изображению — ") + appName,
                                                    QString(), QString::fromUtf8("Все изображения (%1)").arg(supportedFormatsString));
  if (imageFile.isEmpty()) {
    exit(EXIT_SUCCESS);
    return;
  }

  QPixmap* image = new QPixmap;
  if (!image->load(imageFile)) {
    QMessageBox::warning(this, appName, QString::fromUtf8("Не могу открыть изображение \"%1\".").arg(imageFile));
    delete image;
    exit(EXIT_SUCCESS);
    return;
  }

  QLabel* statusLabel = new QLabel(this);
  ui->statusBar->addWidget(statusLabel);
  canvasWidget = new CanvasWidget(image, this, ui->containingScrollArea, statusLabel, this);
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

void MainWindow::setMeasurementEnabled(bool state)
{
  measureSegmentLengthAction       ->setEnabled(state);
  measurePolylineLengthAction      ->setEnabled(state);
  measureClosedPolylineLengthAction->setEnabled(state);
  measureRectangleAreaAction       ->setEnabled(state);
  measurePolygonAreaAction         ->setEnabled(state);
}

void MainWindow::updateMode(QAction* modeAction)
{
  if (modeAction == setEtalonAction)
    return setMode(SET_ETALON);
  if (modeAction == measureSegmentLengthAction)
    return setMode(MEASURE_SEGMENT_LENGTH);
  if (modeAction == measurePolylineLengthAction)
    return setMode(MEASURE_POLYLINE_LENGTH);
  if (modeAction == measureClosedPolylineLengthAction)
    return setMode(MEASURE_CLOSED_POLYLINE_LENGTH);
  if (modeAction == measureRectangleAreaAction)
    return setMode(MEASURE_RECTANGLE_AREA);
  if (modeAction == measurePolygonAreaAction)
    return setMode(MEASURE_POLYGON_AREA);
  abort();
}
