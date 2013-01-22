#-------------------------------------------------
#
# Project created by QtCreator 2012-11-04T20:22:51
#
#-------------------------------------------------

QT       += core gui

TARGET = AreaMeasurement
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    polygon_math.cpp \
    canvaswidget.cpp \
    figure.cpp \
    paint_utils.cpp \
    selection.cpp

HEADERS  += mainwindow.h \
    polygon_math.h \
    canvaswidget.h \
    defines.h \
    figure.h \
    paint_utils.h \
    selection.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
