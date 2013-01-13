#include <cassert>

#include <QFileDialog>
#include <QFontDialog>
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
  openFileAction                    = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), QString::fromUtf8("Открыть файл"),                              this);
  saveFileAction                    = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), QString::fromUtf8("Сохранить файл"),                            this);
  toggleEtalonModeAction            = new QAction(QIcon(":/pictures/etalon.png"),                     QString::fromUtf8("Включить/выключить режим задания эталона"),  this);
  measureSegmentLengthAction        = new QAction(QIcon(":/pictures/segment_length.png"),             QString::fromUtf8("Измерение длин отрезков"),                   modeActionGroup);
  measurePolylineLengthAction       = new QAction(QIcon(":/pictures/polyline_length.png"),            QString::fromUtf8("Измерение длин кривых"),                     modeActionGroup);
  measureClosedPolylineLengthAction = new QAction(QIcon(":/pictures/closed_polyline_length.png"),     QString::fromUtf8("Измерение длин замкнутых кривых"),           modeActionGroup);
  measureRectangleAreaAction        = new QAction(QIcon(":/pictures/rectangle_area.png"),             QString::fromUtf8("Измерение площадей прямоугольников"),        modeActionGroup);
  measurePolygonAreaAction          = new QAction(QIcon(":/pictures/polygon_area.png"),               QString::fromUtf8("Измерение площадей многоугольников"),        modeActionGroup);
  toggleRulerAction                 = new QAction(QIcon(":/pictures/toggle_ruler.png"),               QString::fromUtf8("Показать/скрыть масштабную линейку"),        this);
  customizeInscriptionFontAction    = new QAction(QIcon(":/pictures/font.png"),                       QString::fromUtf8("Настроить шрифт подписей"),                  this);
  aboutAction                       = new QAction(QIcon(":/pictures/about.png"),                      QString::fromUtf8("О программе"),                               this);

  openRecentMenu = new QMenu(this);
  openFileAction->setMenu(openRecentMenu);
  saveFileAction->setEnabled(false);

  toggleEtalonModeAction->setCheckable(true);
  toggleEtalonModeAction->setChecked(true);

  foreach (QAction* action, modeActionGroup->actions())
    action->setCheckable(true);
  measureSegmentLengthAction->setChecked(true);

  toggleRulerAction->setCheckable(true);
  toggleRulerAction->setChecked(true);

  ui->mainToolBar->addAction(openFileAction);
  ui->mainToolBar->addAction(saveFileAction);
  ui->mainToolBar->addSeparator();
  ui->mainToolBar->addAction(toggleEtalonModeAction);
  ui->mainToolBar->addSeparator();
  ui->mainToolBar->addActions(modeActionGroup->actions());
  ui->mainToolBar->addSeparator();
  ui->mainToolBar->addAction(toggleRulerAction);
  ui->mainToolBar->addAction(customizeInscriptionFontAction);
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

  connect(openFileAction,                 SIGNAL(triggered()), this, SLOT(openFile()));
  connect(saveFileAction,                 SIGNAL(triggered()), this, SLOT(saveFile()));
  connect(customizeInscriptionFontAction, SIGNAL(triggered()), this, SLOT(customizeInscriptionFont()));
  connect(aboutAction,                    SIGNAL(triggered()), this, SLOT(showAbout()));

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
  return QList<int>() << 0 << 6;
}

QString MainWindow::appVersionString() const
{
  QString versionString;
  foreach (int x, appVersion())
    versionString += QString::number(x) + '.';
  return versionString.left(versionString.length() - 1);
}


QFont MainWindow::getInscriptionFont() const
{
  return inscriptionFont;
}

void MainWindow::setMode(FigureType newMode)
{
  if (canvasWidget)
    canvasWidget->setMode(newMode);
}


QString MainWindow::getImageFormatsFilter() const
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
  return QString::fromUtf8("Все изображения (%1);;").arg(allFormatsString) + singleFormatsList.join(";;");
}

void MainWindow::doOpenFile(const QString& filename)
{
  QPixmap* image = new QPixmap;
  if (!image->load(filename)) {
    QMessageBox::warning(this, appName(), QString::fromUtf8("Не могу открыть изображение «%1».").arg(filename));
    delete image;
    return;
  }

  recentFiles.removeAll(filename);
  recentFiles.prepend(filename);
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

  saveFileAction->setEnabled(true);
  saveSettings();
  setDrawOptionsEnabled(true);
}

void MainWindow::doSaveFile(const QString& filename)
{
  assert(canvasWidget);
  bool ok = canvasWidget->getModifiedImage().save(filename);
  if (ok)
    ui->statusBar->showMessage(QString::fromUtf8("Файл успешно сохранён"), 5000);
  else
    QMessageBox::warning(this, appName(), QString::fromUtf8("Не удалось записать файл «%1»!").arg(filename));
}

void MainWindow::loadSettings()
{
  QSettings settings(companyName(), appName());
  inscriptionFont = settings.value("Inscription Font", QFont()).value<QFont>();
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
  QSettings settings(companyName(), appName());
  settings.clear();
  settings.setValue("Inscription Font", inscriptionFont);
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


void MainWindow::openFile()
{
  QString filename = QFileDialog::getOpenFileName(this, QString::fromUtf8("Открыть изображение — ") + appName(),
                                                  QString(), getImageFormatsFilter(), 0);
  if (!filename.isEmpty())
    doOpenFile(filename);
}

void MainWindow::openRecentFile()
{
  QAction* triggeredAction = qobject_cast<QAction*>(sender());
  assert(triggeredAction);
  doOpenFile(triggeredAction->text());
}

void MainWindow::saveFile()
{
  QString filename = QFileDialog::getSaveFileName(this, QString::fromUtf8("Сохранить изображение — ") + appName(),
                                                  QString(), getImageFormatsFilter(), 0);
  if (!filename.isEmpty())
    doSaveFile(filename);
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
    return setMode(SEGMENT);
  if (modeAction == measurePolylineLengthAction)
    return setMode(POLYLINE);
  if (modeAction == measureClosedPolylineLengthAction)
    return setMode(CLOSED_POLYLINE);
  if (modeAction == measureRectangleAreaAction)
    return setMode(RECTANGLE);
  if (modeAction == measurePolygonAreaAction)
    return setMode(POLYGON);
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

void MainWindow::customizeInscriptionFont()
{
  // TODO: Why does the dialog show wrong font for the first time?
  inscriptionFont = QFontDialog::getFont(0, inscriptionFont, this);
  if (canvasWidget)
    canvasWidget->update();
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
