#include <QFileDialog>
#include <QImageReader>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>

#include "canvaswidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setAttribute(Qt::WA_QuitOnClose);
  setWindowState(windowState() | Qt::WindowMaximized);
  setWindowTitle(appName());

  QActionGroup* modeActionGroup = new QActionGroup(this);
  toggleEtalonModeAction            = new QAction(QIcon(":/pictures/etalon.png"),                 QString::fromUtf8("Включить/выключить режим задания эталона"),  this);
  measureSegmentLengthAction        = new QAction(QIcon(":/pictures/segment_length.png"),         QString::fromUtf8("Измерение длин отрезков"),                   modeActionGroup);
  measurePolylineLengthAction       = new QAction(QIcon(":/pictures/polyline_length.png"),        QString::fromUtf8("Измерение длин кривых"),                     modeActionGroup);
  measureClosedPolylineLengthAction = new QAction(QIcon(":/pictures/closed_polyline_length.png"), QString::fromUtf8("Измерение длин замкнутых кривых"),           modeActionGroup);
  measureRectangleAreaAction        = new QAction(QIcon(":/pictures/rectangle_area.png"),         QString::fromUtf8("Измерение площадей прямоугольников"),        modeActionGroup);
  measurePolygonAreaAction          = new QAction(QIcon(":/pictures/polygon_area.png"),           QString::fromUtf8("Измерение площадей многоугольников"),        modeActionGroup);
  toggleRulerAction                 = new QAction(QIcon(":/pictures/toggle_ruler.png"),           QString::fromUtf8("Показать/скрыть масштабную линейку"),        this);
  aboutAction                       = new QAction(QIcon(":/pictures/about.png"),                  QString::fromUtf8("О программе"),                               this);
  toggleEtalonModeAction->setCheckable(true);
  toggleEtalonModeAction->setChecked(true);
  foreach (QAction* action, modeActionGroup->actions())
    action->setCheckable(true);
  measureSegmentLengthAction->setChecked(true);
  toggleRulerAction->setCheckable(true);
  toggleRulerAction->setChecked(true);
  ui->mainToolBar->addAction(toggleEtalonModeAction);
  ui->mainToolBar->addSeparator();
  ui->mainToolBar->addActions(modeActionGroup->actions());
  ui->mainToolBar->addSeparator();
  ui->mainToolBar->addAction(toggleRulerAction);
  ui->mainToolBar->addSeparator();
  ui->mainToolBar->addAction(aboutAction);
  ui->mainToolBar->setIconSize(QSize(32, 32));
  ui->mainToolBar->setContextMenuPolicy(Qt::PreventContextMenu);

  QList<QByteArray> supportedFormatsList = QImageReader::supportedImageFormats();
  QString supportedFormatsString;
  foreach (const QByteArray& format, supportedFormatsList)
     supportedFormatsString += "*." + QString(format).toLower() + " ";

  QString imageFile = QFileDialog::getOpenFileName (this, QString::fromUtf8("Укажите путь к изображению — ") + appName(),
                                                    QString(), QString::fromUtf8("Все изображения (%1)").arg(supportedFormatsString));
  if (imageFile.isEmpty()) {
    QTimer::singleShot(0, this, SLOT(close()));
    return;
  }

  QPixmap* image = new QPixmap;
  if (!image->load(imageFile)) {
    QMessageBox::warning(this, appName(), QString::fromUtf8("Не могу открыть изображение \"%1\".").arg(imageFile));
    delete image;
    QTimer::singleShot(0, this, SLOT(close()));
    return;
  }

  QFont statusBarFont = ui->statusBar->font();
  int newSize = QFontInfo(statusBarFont).pointSize() * 1.5;  // In contrast to ``labelFont.pointSize()'' it always works
  statusBarFont.setPointSize(newSize);
  ui->statusBar->setFont(statusBarFont);
  QLabel* scaleLabel  = new QLabel(this);
  QLabel* statusLabel = new QLabel(this);
  ui->statusBar->addPermanentWidget(scaleLabel);
  ui->statusBar->addWidget(statusLabel);

  canvasWidget = new CanvasWidget(image, this, ui->containingScrollArea, scaleLabel, statusLabel, this);
  ui->containingScrollArea->setBackgroundRole(QPalette::Dark);
  ui->containingScrollArea->setWidget(canvasWidget);

  connect(toggleEtalonModeAction, SIGNAL(toggled(bool)),       this,         SLOT(toggleEtalonDefinition(bool)));
  connect(modeActionGroup,        SIGNAL(triggered(QAction*)), this,         SLOT(updateMode(QAction*)));
  connect(toggleRulerAction,      SIGNAL(toggled(bool)),       canvasWidget, SLOT(toggleRuler(bool)));
  connect(aboutAction,            SIGNAL(triggered()),         this,         SLOT(showAbout()));
  canvasWidget->toggleRuler(toggleRulerAction->isChecked());
}

MainWindow::~MainWindow()
{
  delete ui;
}


QString MainWindow::appName() const
{
  return "AreaMeasurement";
}

QList<int> MainWindow::appVersion() const
{
  // TODO: Don't forget to increment it!
  return QList<int>() << 0 << 4;
}

QString MainWindow::appVersionString() const
{
  QString versionString;
  foreach (int x, appVersion())
    versionString += QString::number(x) + '.';
  return versionString.left(versionString.length() - 1);
}


void MainWindow::setMode(Mode newMode)
{
  canvasWidget->setMode(newMode);
}

void MainWindow::updateMode(QAction* modeAction)
{
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

void MainWindow::toggleEtalonDefinition(bool isDefiningEtalon)
{
  if (canvasWidget->isEtalonCorrect()) {
    toggleEtalonModeAction->setChecked(isDefiningEtalon);
    canvasWidget->toggleEtalonDefinition(isDefiningEtalon);
  }
  else
    toggleEtalonModeAction->setChecked(true);
}

void MainWindow::showAbout()
{
  QString aboutText = QString::fromUtf8("Программа %1, версия %2.\n\n"
                                        "Приложение предназначено для измерения длин и площадей объектов на чертежах, картах и т.д.\n\n"
                                        "Автор — Матвеякин Андрей.\n\n"
                                        "Программа распространяется бесплатно по принципу «как есть»: автор не несёт ответственности за возможный ущерб, нанесённый в результате работы приложения."
                                        ).arg(appName()).arg(appVersionString());
  QMessageBox::about(this, QString::fromUtf8("О программе %1").arg(appName()), aboutText);
}
