#-------------------------------------------------
#
# Project created by QtCreator 2012-11-04T20:22:51
#
#-------------------------------------------------

QT       += core gui

TARGET = AreaMeasurement
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++0x


SOURCES += main.cpp\
        mainwindow.cpp \
    canvaswidget.cpp \
    defines.cpp \
    figure.cpp \
    paint_utils.cpp \
    selection.cpp \
    shape.cpp

HEADERS  += mainwindow.h \
    canvaswidget.h \
    defines.h \
    figure.h \
    paint_utils.h \
    selection.h \
    shape.h \
    debug_utils.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
