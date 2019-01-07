#-------------------------------------------------
#
# Project created by QtCreator 2018-11-06T11:56:30
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT += qml quick network xml
QT += quickwidgets
CONFIG += c++11

TARGET = Robot_Plugins
TEMPLATE = lib
CONFIG += plugin

#DESTDIR = $$[QT_INSTALL_PLUGINS]/generic

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        robot_plugins.cpp \
    turing.cpp

HEADERS += \
        robot_plugins.h \
    turing.h
DISTFILES += Robot_Plugins.json 

unix {
    target.path = /usr/lib
    INSTALLS += target
}

RESOURCES += \
    qml.qrc