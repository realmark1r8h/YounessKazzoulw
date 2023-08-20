#-------------------------------------------------
#
# Project created by QtCreator 2019-06-29T13:45:31
#
#-------------------------------------------------

QT       += widgets network dbus gui core
TEMPLATE = lib
CONFIG += plugin

TARGET = $$qtLibraryTarget(netconnect)
DESTDIR = ../../../pluginlibs

include(../../../env.pri)
include($$PROJECT_COMPONENTSOURCE/switchbutton.pri)

INCLUDEPATH   +=  \
                 $$PROJECT_COMPONENTSOURCE \
                 $$PROJECT_ROOTDIR \

LIBS          +=  -L/usr/lib/ -lgsettings-qt
#DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        kylin_network_interface.c \
        netconnect.cpp

HEADERS += \
        kylin_network_interface.h \
        netconnect.h

FORMS += \
        netconnect.ui
