project(Util)

set(JoynrLib_EXPORT_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/include/joynr/JoynrExport.h"
)

set(SOURCES
    BootClock.cpp
    Future.cpp
    ObjectWithDecayTime.cpp
    Settings.cpp
    StatusCode.cpp
    SystemServicesSettings.cpp
    TimePoint.cpp
    Url.cpp
    Util.cpp
)

set(PUBLIC_HEADERS
    "${CMAKE_CURRENT_BINARY_DIR}/include/joynr/JoynrVersion.h"
    "${JoynrLib_EXPORT_HEADER}"

    include/joynr/BootClock.h
    include/joynr/BoostIoserviceForwardDecl.h
    include/joynr/Cache.h
    include/joynr/CachedValue.h
    include/joynr/ContentWithDecayTime.h
    include/joynr/Future.h
    include/joynr/TaskSequencer.h
    include/joynr/HashUtil.h
    include/joynr/ObjectWithDecayTime.h
    include/joynr/PrivateCopyAssign.h
    include/joynr/ReadWriteLock.h
    include/joynr/Settings.h
    include/joynr/StatusCode.h
    include/joynr/SystemServicesSettings.h
    include/joynr/ThreadSafeMap.h
    include/joynr/TimePoint.h
    include/joynr/Url.h
    include/joynr/Util.h
)

add_library(${PROJECT_NAME} OBJECT ${PUBLIC_HEADERS} ${SOURCES})
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME}
    PRIVATE CMAKE_JOYNR_SETTINGS_INSTALL_DIR="${JOYNR_INSTALL_FULL_SYSCONFDIR}"
)
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PUBLIC Boost::thread
    PUBLIC Boost::filesystem
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::Logger
)

configure_file(include/joynr/JoynrVersion.h.in include/joynr/JoynrVersion.h @ONLY)

generate_export_header(
    ${PROJECT_NAME}
    EXPORT_FILE_NAME "${JoynrLib_EXPORT_HEADER}"
    BASE_NAME "Joynr"
)

install(
    DIRECTORY include/ "${CMAKE_CURRENT_BINARY_DIR}/include/"
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

AddClangFormat(${PROJECT_NAME} ${JoynrExportHeader})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
