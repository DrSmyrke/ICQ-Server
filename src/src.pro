QT += core network
QT -= gui

TARGET = src
CONFIG += console
CONFIG -= app_bundle

QMAKE_CXXFLAGS += "-std=c++11"

TEMPLATE = app

SOURCES += main.cpp \
    icqserver.cpp \
    global.cpp \
    icqclient.cpp

HEADERS += \
    icqserver.h \
    global.h \
    icqclient.h

