message(STATUS "#############################################")
message(STATUS "Configuring cluster controller executable...")
message(STATUS "#############################################\n")

get_target_property(JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY JoynrClusterController PUBLIC_INCLUDE_DIRECTORY)
get_target_property(runtime_PUBLIC_INCLUDE_DIRECTORY JoynrClusterControllerRuntime PUBLIC_INCLUDE_DIRECTORY)

set(
    ClusterControllerExecutable_INCLUDE_DIRECTORIES
    ${runtime_PUBLIC_INCLUDE_DIRECTORY}
    ${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}
    "../"
)

file(
    GLOB_RECURSE cluster-controller_PUBLIC_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.h"
)
file(
    GLOB_RECURSE cluster-controller_PUBLIC_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp"
)

set(
    cluster-controller_HEADERS
    ${cluster-controller_PUBLIC_HEADERS}

)

set(
    cluster-controller_SOURCES
    ${cluster-controller_PUBLIC_SOURCES}
)

add_executable(
    cluster-controller
    ${cluster-controller_HEADERS}
    ${cluster-controller_SOURCES}
)

AddClangFormat(cluster-controller)

AddClangTidy(cluster-controller)

target_link_libraries(cluster-controller
    JoynrClusterControllerRuntime
)

target_include_directories(
    cluster-controller
    PRIVATE ${ClusterControllerExecutable_INCLUDE_DIRECTORIES}
)

if(JOYNR_ENABLE_DLT_LOGGING)
    set(
      JOYNR_CLUSTER_CONTROLLER_DLT_APP_ID
      "JOCC"
      CACHE
      STRING
      "DLT application id for cluster-controller"
    )
    set(
      JOYNR_CLUSTER_CONTROLLER_DLT_DESCRIPTION
      "joynr cluster controller"
      CACHE
      STRING
      "DLT application description for cluster-controller"
    )
    target_compile_definitions(
        cluster-controller
        PRIVATE
        JOYNR_CLUSTER_CONTROLLER_DLT_APP_ID=${JOYNR_CLUSTER_CONTROLLER_DLT_APP_ID}
        JOYNR_CLUSTER_CONTROLLER_DLT_DESCRIPTION=${JOYNR_CLUSTER_CONTROLLER_DLT_DESCRIPTION}
    )
endif()

install(
    TARGETS cluster-controller
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
