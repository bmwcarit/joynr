project(Logger)

option(JOYNR_ENABLE_STDOUT_LOGGING "Use stdout logger?" ON)
option(JOYNR_DISABLE_DEBUG_TO_INFO_LOGGING "Don't map debug loglevel to info loglevel?" OFF)

# for every other build type than debug we limit log messages to level DEBUG
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_TOLOWER)
if(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug)
    set(JOYNR_MAX_LOG_LEVEL "TRACE" CACHE STRING "joynr max log level")
    set(JOYNR_DEFAULT_RUNTIME_LOG_LEVEL "TRACE" CACHE STRING "joynr max log level evaluated at runtime")
else()
    set(JOYNR_MAX_LOG_LEVEL "TRACE" CACHE STRING "joynr max log level")
    set(JOYNR_DEFAULT_RUNTIME_LOG_LEVEL "DEBUG" CACHE STRING "joynr max log level evaluated at runtime")
endif()

macro(CREATE_LOG_LEVEL_PROPERTY name)
    set_property(CACHE ${name} PROPERTY STRINGS
                 FATAL ERROR WARN INFO DEBUG TRACE
    )
endmacro()

CREATE_LOG_LEVEL_PROPERTY(JOYNR_MAX_LOG_LEVEL)
CREATE_LOG_LEVEL_PROPERTY(JOYNR_DEFAULT_RUNTIME_LOG_LEVEL)

set(SOURCES
    Logger.cpp
)

set(PUBLIC_HEADERS
    include/joynr/Logger.h
    include/joynr/DltSink.h
)

add_library(${PROJECT_NAME} OBJECT ${PUBLIC_HEADERS} ${SOURCES})
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PUBLIC spdlog::spdlog_header_only
    PUBLIC $<$<BOOL:${JOYNR_ENABLE_DLT_LOGGING}>:DLT::DLT>
)
target_compile_definitions(${PROJECT_NAME}
    PUBLIC "$<$<BOOL:${JOYNR_ENABLE_STDOUT_LOGGING}>:JOYNR_ENABLE_STDOUT_LOGGING>"
    PUBLIC "$<$<BOOL:${JOYNR_ENABLE_DLT_LOGGING}>:JOYNR_ENABLE_DLT_LOGGING>"
    PUBLIC "$<$<BOOL:${JOYNR_DISABLE_DEBUG_TO_INFO_LOGGING}>:JOYNR_DISABLE_DEBUG_TO_INFO_LOGGING>"
    PUBLIC "JOYNR_MAX_LOG_LEVEL_${JOYNR_MAX_LOG_LEVEL}"
    PUBLIC "JOYNR_DEFAULT_RUNTIME_LOG_LEVEL_${JOYNR_DEFAULT_RUNTIME_LOG_LEVEL}"
)

install(
    DIRECTORY include/
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
