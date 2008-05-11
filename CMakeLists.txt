
PROJECT(skype-call-recorder)

SET(TARGET skype-call-recorder)
SET(CMAKE_BUILD_TYPE RELEASE)

SET(CMAKE_CXX_FLAGS "-Wall -W -pipe -fno-rtti -fno-exceptions")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2")

SET(SOURCES call.cpp callgui.cpp common.cpp mp3writer.cpp preferences.cpp
	recorder.cpp skype.cpp trayicon.cpp wavewriter.cpp writer.cpp)
SET(MOC_HEADERS call.h callgui.h preferences.h recorder.h skype.h
	smartwidgets.h trayicon.h)
SET(RESOURCES resources.qrc)

SET(QT_USE_QTDBUS TRUE)
SET(QT_USE_QTNETWORK TRUE)
SET(LIBRARIES mp3lame)

# versions before 2.4.8 do not seem to know about Qt's DBus
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.8 FATAL_ERROR)

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(MOC_SOURCES ${MOC_HEADERS})
QT4_ADD_RESOURCES(RESOURCES_SOURCES ${RESOURCES})

ADD_EXECUTABLE(${TARGET} ${SOURCES} ${MOC_SOURCES} ${RESOURCES_SOURCES})
TARGET_LINK_LIBRARIES(${TARGET} ${QT_LIBRARIES} ${LIBRARIES})

