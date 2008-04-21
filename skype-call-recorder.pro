
TEMPLATE = app
TARGET = skype-call-recorder
DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES = common.cpp recorder.cpp trayicon.cpp skype.cpp preferences.cpp call.cpp writer.cpp wavewriter.cpp
HEADERS = common.h   recorder.h   trayicon.h   skype.h   preferences.h   call.h   writer.h   wavewriter.h

HEADERS += smartwidgets.h

RESOURCES = resources.qrc

CONFIG += qdbus
QT += network

