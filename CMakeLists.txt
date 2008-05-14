
PROJECT(skype-call-recorder)

SET(TARGET skype-call-recorder)
SET(CMAKE_BUILD_TYPE RELEASE)

SET(CMAKE_CXX_FLAGS "-Wall -W -pipe -fno-rtti -fno-exceptions")
SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2")

SET(SOURCES about.cpp call.cpp callgui.cpp common.cpp mp3writer.cpp
	preferences.cpp recorder.cpp skype.cpp trayicon.cpp version.cpp
	wavewriter.cpp writer.cpp)
SET(MOC_HEADERS call.h callgui.h preferences.h recorder.h skype.h
	smartwidgets.h trayicon.h)
SET(RESOURCES resources.qrc)

ADD_CUSTOM_TARGET(Version ${CMAKE_CURRENT_SOURCE_DIR}/makeversion ${CMAKE_CURRENT_SOURCE_DIR}
	COMMENT "Checking version.cpp" VERBATIM)
SET_SOURCE_FILES_PROPERTIES(version.cpp PROPERTIES GENERATED TRUE)
SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES version.cpp)

SET(QT_USE_QTDBUS TRUE)
SET(QT_USE_QTNETWORK TRUE)
SET(LIBRARIES mp3lame)

# when building out of source, version.cpp needs this
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# versions before 2.4.8 do not seem to know about Qt's DBus
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.8 FATAL_ERROR)

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(MOC_SOURCES ${MOC_HEADERS})
QT4_ADD_RESOURCES(RESOURCES_SOURCES ${RESOURCES})

ADD_EXECUTABLE(${TARGET} ${SOURCES} ${MOC_SOURCES} ${RESOURCES_SOURCES})
TARGET_LINK_LIBRARIES(${TARGET} ${QT_LIBRARIES} ${LIBRARIES})
ADD_DEPENDENCIES(${TARGET} Version)

