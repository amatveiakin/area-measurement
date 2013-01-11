#include <cassert>

#include <QFileDialog>
#include <QImageReader>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

#include "canvaswidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"


const int maxRecentDocuments = 10;


MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setAttribute(Qt::WA_QuitOnClose);
  setWindowState(windowState() | Qt::WindowMaximized);
  setWindowTitle(appName());

  modeActionGroup = new QActionGroup(this);
  openFileAction                    = new QAction(style()->standardIcon (QStyle::SP_DirOpenIcon), QString::fromUtf8("Открыть файл"),                              this);
  toggleEtalonModeAction            = new QAction(QIcon(":/pictures/etalon.png"),                 QString::fromUtf8("Включить/выключить режим задания эталона"),  this);
  measureSegmentLengthAction        = new QAction(QIcon(":/pictures/segment_length.png"),         QString::fromUtf8("Измерение длин отрезков"),                   modeActionGroup);
  measurePolylineLengthAction       = new QAction(QIcon(":/pictures/polyline_length.png"),        QString::fromUtf8("Измерение длин кривых"),                     modeActionGroup);
  measureClosedPolylineLengthAction = new QAction(QIcon(":/pictures/closed_polyline_length.png"), QString::fromUtf8("Измерение длин замкнутых кривых"),           modeActionGroup);
  measureRectangleAreaAction        = new QAction(QIcon(":/pictures/rectangle_area.png"),         QString::fromUtf8("Измерение площадей прямоугольников"),        modeActionGroup);
  measurePolygonAreaAction          = new QAction(QIcon(":/pictures/polygon_area.png"),           QString::fromUtf8("Измерение площадей многоугольников"),        modeActionGroup);
  toggleRulerAction                 = new QAction(QIcon(":/pictures/toggle_ruler.png"),           QString::fromUtf8("Показать/скрыть масштабную линейку"),        this);
  aboutAction                       = new QAction(QIcon(":/pictures/about.png"),                  QString::fromUtf8("О программе"),                               this);

  openRecentMenu = new QMenu(this);
  openFileAction->setMenu(openRecentMenu);

  toggleEtalonModeAction->setCheckable(true);
  toggleEtalonModeAction->setChecked(true);

  foreach (QAction* action, modeActionGroup->actions())
    action->setCheckable(true);
  measureSegmentLengthAction->setChecked(true);

  toggleRulerAction->setCheckable(true);
  toggleRulerAction->setChecked(true);

  ui->mainToolBar->addAction(openFileAction);
  ui->mainToolBar->addSeparator();
  ui->mainToolBar->addAction(toggleEtalonModeAction);
  ui->mainToolBar->addSeparator();
  ui->mainToolBar->addActions(modeActionGroup->actions());
  ui->mainToolBar->addSeparator();
  ui->mainToolBar->addAction(toggleRulerAction);
  ui->mainToolBar->addSeparator();
  ui->mainToolBar->addAction(aboutAction);
  ui->mainToolBar->setIconSize(QSize(32, 32));
  ui->mainToolBar->setContextMenuPolicy(Qt::PreventContextMenu);

  QFont statusBarFont = ui->statusBar->font();
  int newSize = QFontInfo(statusBarFont).pointSize() * 1.5;  // In contrast to ``labelFont.pointSize()'' it always works
  statusBarFont.setPointSize(newSize);
  ui->statusBar->setFont(statusBarFont);
  scaleLabel  = new QLabel(this);
  statusLabel = new QLabel(this);
  ui->statusBar->addPermanentWidget(scaleLabel);
  ui->statusBar->addWidget(statusLabel);

  canvasWidget = 0;

  connect(openFileAction, SIGNAL(triggered()), this, SLOT(openNewFile()));
  connect(aboutAction,    SIGNAL(triggered()), this, SLOT(showAbout()));

  connect(toggleEtalonModeAction, SIGNAL(toggled(bool)),       this, SLOT(toggleEtalonDefinition(bool)));
  connect(modeActionGroup,        SIGNAL(triggered(QAction*)), this, SLOT(updateMode(QAction*)));

  setDrawOptionsEnabled(false);
  loadAndApplySettings();
}

MainWindow::~MainWindow()
{
  saveSettings();
  delete ui;
}


QString MainWindow::companyName() const
{
  return "AMM Soft";
}

QString MainWindow::appName() const
{
  return "AreaMeasurement";
}

QList<int> MainWindow::appVersion() const
{
  // TODO: Don't forget to increment it!
  return QList<int>() << 0 << 5;
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
  if (canvasWidget)
    canvasWidget->setMode(newMode);
}


void MainWindow::doOpenFile(const QString& imageFile)
{
  QPixmap* image = new QPixmap;
  if (!image->load(imageFile)) {
    QMessageBox::warning(this, appName(), QString::fromUtf8("Не могу открыть изображение \"%1\".").arg(imageFile));
    delete image;
    return;
  }

  recentFiles.removeAll(imageFile);
  recentFiles.prepend(imageFile);
  if (recentFiles.size() > maxRecentDocuments)
    recentFiles.erase(recentFiles.begin() + maxRecentDocuments, recentFiles.end());
  updateOpenRecentMenu();

  toggleEtalonModeAction->setChecked(true);
  measureSegmentLengthAction->setChecked(true);

  delete canvasWidget;
  canvasWidget = new CanvasWidget(image, this, ui->containingScrollArea, scaleLabel, statusLabel, this);
  ui->containingScrollArea->setWidget(canvasWidget);

  connect(toggleRulerAction, SIGNAL(toggled(bool)), canvasWidget, SLOT(toggleRuler(bool)));
  canvasWidget->toggleRuler(toggleRulerAction->isChecked());

  saveSettings();
  setDrawOptionsEnabled(true);
}

void MainWindow::loadSettings()
{
  QSettings settings(companyName(), appName ());
  recentFiles.clear();
  int nRecent = settings.beginReadArray("Recent Documents");
  for (int i = 0; i < nRecent; ++i) {
    settings.setArrayIndex(i);
    recentFiles.append(settings.value("Filename").toString());
  }
  settings.endArray();
}

void MainWindow::loadAndApplySettings()
{
  loadSettings();
  updateOpenRecentMenu();
}

void MainWindow::saveSettings() const
{
  QSettings settings(companyName(), appName ());
  settings.clear();
  settings.beginWriteArray("Recent Documents");
  for (int i = 0; i < recentFiles.size(); ++i) {
    settings.setArrayIndex(i);
    settings.setValue("Filename", recentFiles[i]);
  }
  settings.endArray();
}

void MainWindow::updateOpenRecentMenu()
{
  openRecentMenu->clear();
  foreach (const QString& file, recentFiles)
    openRecentMenu->addAction(file, this, SLOT(openRecentFile()));
  if (openRecentMenu->isEmpty()) {
    QAction* infoAction = new QAction(QString::fromUtf8("Список недавно открытых документов пуст"), openRecentMenu);
    infoAction->setEnabled(false);
    openRecentMenu->addAction(infoAction);
  }
}


void MainWindow::openNewFile()
{
  QList<QByteArray> supportedFormatsList__ = QImageReader::supportedImageFormats();
  QSet<QString> supportedFormatsSet;
  foreach (const QByteArray& format, supportedFormatsList__)
    supportedFormatsSet.insert(QString(format).toLower());
  QStringList supportedFormatsList(supportedFormatsSet.toList());
  qSort(supportedFormatsList);
  QString allFormatsString;
  QStringList singleFormatsList;
  foreach (const QString& lowerFormat, supportedFormatsList) {
    QString upperFormat = lowerFormat.toUpper();
    allFormatsString += QString("*.%1 ").arg(lowerFormat);
    if (upperFormat == "JPEG")
      singleFormatsList += QString::fromUtf8("Изображения JPEG (*.jpeg *.jpg)");
    else if (upperFormat != "JPG")
      singleFormatsList += QString::fromUtf8("Изображения %1 (*.%2)").arg(upperFormat).arg(lowerFormat);
  }
  allFormatsString = allFormatsString.trimmed();
  QString formatsList = QString::fromUtf8("Все изображения (%1);;").arg(allFormatsString) + singleFormatsList.join(";;");

  QString imageFile = QFileDialog::getOpenFileName (this, QString::fromUtf8("Укажите путь к изображению — ") + appName(),
                                                    QString(), formatsList, 0);
  if (!imageFile.isEmpty())
    doOpenFile(imageFile);
}

void MainWindow::openRecentFile()
{
  QAction* triggeredAction = qobject_cast<QAction*>(sender());
  assert(triggeredAction);
  doOpenFile(triggeredAction->text());
}

void MainWindow::setDrawOptionsEnabled(bool enabled)
{
  toggleEtalonModeAction->setEnabled(enabled);
  modeActionGroup       ->setEnabled(enabled);
  toggleRulerAction     ->setEnabled(enabled);
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
  if (!canvasWidget)
    return;

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
