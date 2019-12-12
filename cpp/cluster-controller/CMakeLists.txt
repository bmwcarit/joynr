project(cluster-controller)

add_executable(${PROJECT_NAME} "")

target_sources(${PROJECT_NAME} PRIVATE
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Joynr::JoynrClusterControllerRuntime
)

target_include_directories(${PROJECT_NAME}
    # for includes like #include "runtimes/cluster-controller-runtime/signal-handler/PosixSignalHandler.h"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

if(JOYNR_ENABLE_DLT_LOGGING)
    set(JOYNR_CLUSTER_CONTROLLER_DLT_APP_ID "JOCC"
        CACHE STRING
        "DLT application id for cluster-controller"
    )
    set(JOYNR_CLUSTER_CONTROLLER_DLT_DESCRIPTION "joynr cluster controller"
        CACHE STRING
        "DLT application description for cluster-controller"
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        JOYNR_CLUSTER_CONTROLLER_DLT_APP_ID=${JOYNR_CLUSTER_CONTROLLER_DLT_APP_ID}
        JOYNR_CLUSTER_CONTROLLER_DLT_DESCRIPTION=${JOYNR_CLUSTER_CONTROLLER_DLT_DESCRIPTION}
    )
endif()

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${JOYNR_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${JOYNR_INSTALL_LIBDIR}"
)
