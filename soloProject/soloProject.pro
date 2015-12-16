#-------------------------------------------------
#
# Project created by QtCreator 2015-10-12T20:06:11
#
#-------------------------------------------------

QT       += core gui
QT       += sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = soloProject
TEMPLATE = app


SOURCES += main.cpp \
    gamewindow.cpp \
    deck.cpp \
    players.cpp

HEADERS  += \
    gamewindow.h \
    deck.h \
    players.h

RESOURCES += \
    image.qrc
