#-------------------------------------------------
#
# Project created by QtCreator 2016-09-02T10:33:07
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = EliteLog
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    fileops.cpp

HEADERS  += mainwindow.h \
    fileops.h

FORMS    += mainwindow.ui

QMAKE_CXXFLAGS_WARN_OFF -= -Wunused-parameter
