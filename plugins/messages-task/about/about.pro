QT       += widgets dbus

TEMPLATE = lib
CONFIG += plugin

TARGET = $$qtLibraryTarget(about)
DESTDIR = ../../../pluginlibs

include(../../../env.pri)

INCLUDEPATH   +=  \
                 $$PROJECT_ROOTDIR \




#DEFINES += QT_DEPRECATED_WARNINGS

#target.path = $$[QT_INSTALL_PREFIX]/lib/control-center/plugins/
#INSTALLS += target

HEADERS += \
    about.h

SOURCES += \
    about.cpp

FORMS += \
    about.ui

RESOURCES += \
    res/img.qrc
