# *****************************************************************************
# * Copyright (C) 2011, BMW Car IT GmbH 
# *****************************************************************************

set(LOG4QT_MAJOR_VERSION 0)
set(LOG4QT_MINOR_VERSION 3)
set(LOG4QT_PATCH_VERSION Joynr)
set(
    LOG4QT_VERSION
    ${LOG4QT_MAJOR_VERSION}.${LOG4QT_MINOR_VERSION}.${LOG4QT_PATCH_VERSION}
)

# The source code needs to know which DLL is being built when compiling on Windows
if(${WIN32})
    if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
        add_definitions(/DBUILDING_LOG4QT)
    else()
        add_definitions(-DBUILDING_LOG4QT)
    endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")
endif(${WIN32})

# Suppress "... may be used uninitialized" warnings from GNU C++ compiler
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-Wno-maybe-uninitialized")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(log4qt_HEADERS
    log4qt/appender.h
    log4qt/appenderskeleton.h
    log4qt/consoleappender.h
    log4qt/dailyrollingfileappender.h
    log4qt/fileappender.h
    log4qt/helpers/configuratorhelper.h
    log4qt/helpers/logobject.h
    log4qt/layout.h
    log4qt/logger.h
    log4qt/log4qt_api.h
    log4qt/patternlayout.h
    log4qt/simplelayout.h
    log4qt/spi/filter.h
    log4qt/ttcclayout.h
    log4qt/writerappender.h
    log4qt/varia/debugappender.h
    log4qt/varia/denyallfilter.h
    log4qt/varia/nullappender.h
    log4qt/varia/levelmatchfilter.h
    log4qt/varia/levelrangefilter.h
    log4qt/varia/listappender.h
    log4qt/varia/stringmatchfilter.h
    log4qt/rollingfileappender.h
    log4qt/basicconfigurator.h
    log4qt/helpers/classlogger.h 
    log4qt/helpers/datetime.h 
    log4qt/helpers/factory.h 
    log4qt/helpers/initialisationhelper.h 
    log4qt/helpers/logerror.h 
    log4qt/helpers/logobjectptr.h 
    log4qt/helpers/optionconverter.h 
    log4qt/helpers/patternformatter.h 
    log4qt/helpers/properties.h 
    log4qt/hierarchy.h 
    log4qt/level.h 
    log4qt/log4qt.h 
    log4qt/loggerrepository.h 
    log4qt/loggingevent.h 
    log4qt/logmanager.h 
    log4qt/mdc.h 
    log4qt/ndc.h 
    log4qt/propertyconfigurator.h 
    log4qt/log4qt_api.h
)

set(log4qt_SOURCES
    log4qt/appenderskeleton.cpp 
    log4qt/basicconfigurator.cpp 
    log4qt/consoleappender.cpp 
    log4qt/dailyrollingfileappender.cpp 
    log4qt/fileappender.cpp 
    log4qt/helpers/classlogger.cpp 
    log4qt/helpers/configuratorhelper.cpp 
    log4qt/helpers/datetime.cpp 
    log4qt/helpers/factory.cpp 
    log4qt/helpers/initialisationhelper.cpp 
    log4qt/helpers/logerror.cpp 
    log4qt/helpers/logobject.cpp 
    log4qt/helpers/logobjectptr.cpp 
    log4qt/helpers/optionconverter.cpp 
    log4qt/helpers/patternformatter.cpp 
    log4qt/helpers/properties.cpp 
    log4qt/hierarchy.cpp  
    log4qt/layout.cpp 
    log4qt/level.cpp 
    log4qt/log4qt.cpp 
    log4qt/logger.cpp 
    log4qt/loggerrepository.cpp 
    log4qt/loggingevent.cpp 
    log4qt/logmanager.cpp 
    log4qt/mdc.cpp 
    log4qt/ndc.cpp 
    log4qt/patternlayout.cpp 
    log4qt/propertyconfigurator.cpp 
    log4qt/rollingfileappender.cpp 
    log4qt/simplelayout.cpp 
    log4qt/spi/filter.cpp 
    log4qt/ttcclayout.cpp
    log4qt/writerappender.cpp 
    log4qt/varia/debugappender.cpp 
    log4qt/varia/denyallfilter.cpp 
    log4qt/varia/nullappender.cpp 
    log4qt/varia/levelmatchfilter.cpp 
    log4qt/varia/levelrangefilter.cpp 
    log4qt/varia/listappender.cpp 
    log4qt/varia/stringmatchfilter.cpp
)

add_library(
    log4qt SHARED
    ${log4qt_HEADERS}
    ${log4qt_SOURCES}
)
target_link_libraries(log4qt Qt5::Core)

set_target_properties(
    log4qt
    PROPERTIES
    VERSION ${LOG4QT_VERSION}
    SOVERSION ${LOG4QT_VERSION}
    PUBLIC_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

export(
    TARGETS log4qt
    FILE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/log4qt.cmake"
)

install(
    TARGETS log4qt EXPORT JoynrTargets
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT runtime
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT library
    # this will flatten directory hierarchy of headder files
    # PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT development
)
# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY .
    DESTINATION "${INSTALL_INCLUDE_DIR}"
    FILES_MATCHING PATTERN "*.h"
)

#install(
#    EXPORT log4qt_EXPORT
#    FILE "log4qt.cmake"
#    DESTINATION "lib"
#)
