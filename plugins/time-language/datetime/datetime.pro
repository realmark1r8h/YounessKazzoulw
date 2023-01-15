#-------------------------------------------------
#
# Project created by QtCreator 2019-06-29T15:08:41
#
#-------------------------------------------------

QT       += widgets dbus x11extras

TEMPLATE = lib
CONFIG += plugin

TARGET = $$qtLibraryTarget(datetime)
DESTDIR = ../../../pluginlibs


include(../../../env.pri)
include($$PROJECT_COMPONENTSOURCE/switchbutton.pri)


INCLUDEPATH   +=  \
                 $$PROJECT_COMPONENTSOURCE \
                 $$PROJECT_ROOTDIR \
                 /usr/include/kylinsdk \


LIBS += -L/usr/lib/ -lgsettings-qt

##加载gio库和gio-unix库，用于处理时间
CONFIG        += link_pkgconfig \
                 C++11
PKGCONFIG     += gio-2.0 \
                 gio-unix-2.0

#DEFINES += QT_DEPRECATED_WARNINGS

target.path = $$[QT_INSTALL_PREFIX]/lib/control-center/plugins/
INSTALLS += target

SOURCES += \
        datetime.cpp \
    changtime.cpp \

HEADERS += \
        datetime.h \
    changtime.h \

FORMS += \
        datetime.ui \
    changtime.ui

RESOURCES += \
    tz.qrc

DISTFILES +=
