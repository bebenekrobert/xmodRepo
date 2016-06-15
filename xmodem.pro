#-------------------------------------------------
#
# Project created by QtCreator 2015-05-21T18:43:36
#
#-------------------------------------------------

QT       += core
QT       += serialport
QT       -= gui

TARGET = xmodem
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH = ./

SOURCES += main.cpp \
    xmodemsender.cpp \
    xmodemreceiver.cpp


HEADERS += \
    xmodemsender.h \
    common.h \
    xmodemreceiver.h
