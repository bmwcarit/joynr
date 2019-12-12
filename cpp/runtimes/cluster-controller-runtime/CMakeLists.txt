project(JoynrClusterControllerRuntime)

add_library(${PROJECT_NAME} SHARED "")
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# It seems unused, but included from cluster-controller like
# #include "runtimes/cluster-controller-runtime/signal-handler/PosixSignalHandler.h"
# TODO: Should it be part of cluster-controller?
add_subdirectory(signal-handler)

set(SOURCES
    JoynrClusterControllerMqttConnectionData.cpp
    JoynrClusterControllerRuntime.cpp
    JoynrRuntime.cpp
)

set(PUBLIC_HEADERS
    include/joynr/IClusterControllerSignalHandler.h
    include/joynr/JoynrClusterControllerMqttConnectionData.h
    include/joynr/JoynrClusterControllerRuntime.h
)

target_sources(${PROJECT_NAME} PRIVATE
    ${PUBLIC_HEADERS}
    ${SOURCES}
)

target_include_directories(${PROJECT_NAME}
    # for includes like #include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(${PROJECT_NAME}
    PUBLIC Joynr::JoynrClusterControllerLib
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::RuntimeCommon
)

install(
    DIRECTORY include/ "${CMAKE_CURRENT_BINARY_DIR}/include/"
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    SOVERSION "${Joynr_SOVERSION}"
    VERSION "${Joynr_VERSION}"
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT JoynrTargets
    RUNTIME DESTINATION "${JOYNR_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${JOYNR_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${JOYNR_INSTALL_LIBDIR}"
)
