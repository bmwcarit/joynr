project(JoynrLibJoynrRuntime)

add_library(${PROJECT_NAME} STATIC ./LibJoynrRuntime.cpp ./LibJoynrRuntime.h)
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::RuntimeCommon
)

target_include_directories(${PROJECT_NAME}
    # for includes like #include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})

install(
    TARGETS ${PROJECT_NAME}
    EXPORT JoynrTargets
    RUNTIME DESTINATION "${JOYNR_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${JOYNR_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${JOYNR_INSTALL_LIBDIR}"
)

function(create_joynr_runtime TYPE SUBDIR)
    project("Joynr${TYPE}RuntimeWithoutJoynrRuntime")

    add_library(${PROJECT_NAME} OBJECT "")
    add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

    add_subdirectory(${SUBDIR})

    target_include_directories(${PROJECT_NAME}
        # for includes like #include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
        PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
    )

    target_link_libraries(${PROJECT_NAME}
        PUBLIC Joynr::JoynrLibJoynrRuntime
    )

    AddClangFormat(${PROJECT_NAME})

    AddIncludeWhatYouUse(${PROJECT_NAME})

    AddClangTidy(${PROJECT_NAME})

    # -------------------------------------------------------------------------------------------------

    project("Joynr${TYPE}Runtime")

    add_library(${PROJECT_NAME} SHARED "")
    add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

    target_sources(${PROJECT_NAME} PRIVATE
        ${SUBDIR}/JoynrRuntime.cpp
    )

    target_include_directories(${PROJECT_NAME}
        # for includes like #include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
        PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
    )
    target_link_objlibraries(${PROJECT_NAME}
        PUBLIC "Joynr::Joynr${TYPE}RuntimeWithoutJoynrRuntime"
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

endfunction()

create_joynr_runtime(Ws websocket)
create_joynr_runtime(Uds uds)
