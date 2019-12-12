project(JoynrWsRuntimeWithoutJoynrRuntime)

add_library(${PROJECT_NAME} OBJECT "")
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

add_subdirectory(websocket)

target_sources(${PROJECT_NAME} PRIVATE
    LibJoynrRuntime.cpp
    LibJoynrRuntime.h
)

target_include_directories(${PROJECT_NAME}
    # for includes like #include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::RuntimeCommon
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})

# -------------------------------------------------------------------------------------------------

project(JoynrWsRuntime)

add_library(${PROJECT_NAME} SHARED "")
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    websocket/JoynrRuntime.cpp
)

target_include_directories(${PROJECT_NAME}
    # for includes like #include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::JoynrWsRuntimeWithoutJoynrRuntime
    PUBLIC Joynr::RuntimeCommon
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
