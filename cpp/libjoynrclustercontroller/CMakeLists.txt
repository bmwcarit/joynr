message(STATUS "#############################################")
message(STATUS "Configuring JoynrClusterController...")
message(STATUS "#############################################\n")

### Add mosquitto mqtt ###########################################################
set(JOYNR_REQUIRED_MOSQUITTO_VERSION 1.6.8)
if(USE_PLATFORM_MOSQUITTO)
    find_package(mosquitto ${JOYNR_REQUIRED_MOSQUITTO_VERSION} REQUIRED)

    message(STATUS "variable MOSQUITTO_INCLUDE_DIRS=${MOSQUITTO_INCLUDE_DIRS}")
    set(MOSQUITTO_INCLUDE_DIRS ${MOSQUITTO_INCLUDE_DIRS} PARENT_SCOPE)
    get_filename_component(MOSQUITTO_BINARY_DIR ${MOSQUITTO_LIBRARIES} DIRECTORY)
    message(STATUS "variable MOSQUITTO_BINARY_DIR=${MOSQUITTO_BINARY_DIR}")
    message(STATUS "variable MOSQUITTO_LIBRARIES=${MOSQUITTO_LIBRARIES}")
else(USE_PLATFORM_MOSQUITTO)
    message(STATUS "############ Download: mosquitto ############")
    set(MOSQUITTO_SOURCE_URL "https://github.com/eclipse/mosquitto")
    set(MOSQUITTO_SOURCE_TYPE "GIT_REPOSITORY")
    set(MOSQUITTO_LIBRARY_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}mosquitto${CMAKE_SHARED_LIBRARY_SUFFIX})

    AddExternalProject(
        mosquitto
        ${MOSQUITTO_SOURCE_TYPE} "${MOSQUITTO_SOURCE_URL}"
        GIT_TAG v${JOYNR_REQUIRED_MOSQUITTO_VERSION}
        PATCH_COMMAND git apply ${CMAKE_SOURCE_DIR}/../docker/joynr-base/0001-Introduce-mosquitto_connect_bind_async_v5-API.patch
        TIMEOUT 10
        BUILD_BYPRODUCTS("${EXTERNAL_PROJECTS_ROOT}/src/mosquitto-build/lib/${MOSQUITTO_LIBRARY_NAME}")
        NO_PROPAGATE_CXX_FLAGS
    )

    ExternalProject_Get_Property(mosquitto SOURCE_DIR)

    # The directory does not exist when the import targets are checked. As a workaround
    # we need to create it here. Otherwise the initial cmake call will fail.
    file(MAKE_DIRECTORY ${SOURCE_DIR}/lib)

    set(MOSQUITTO_INCLUDE_DIRS ${SOURCE_DIR}/lib)
    message(STATUS "variable MOSQUITTO_INCLUDE_DIRS=${MOSQUITTO_INCLUDE_DIRS}")

    ExternalProject_Get_Property(mosquitto BINARY_DIR)
    set(MOSQUITTO_BINARY_DIR ${BINARY_DIR}/lib)
    message(STATUS "variable MOSQUITTO_BINARY_DIR=${MOSQUITTO_BINARY_DIR}")
    set(MOSQUITTO_LIBRARIES ${MOSQUITTO_BINARY_DIR}/${MOSQUITTO_LIBRARY_NAME})
    message(STATUS "variable MOSQUITTO_LIBRARIES=${MOSQUITTO_LIBRARIES}")
endif(USE_PLATFORM_MOSQUITTO)
include(CheckMosquittoImportTargets)

set(
    JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set(
    JoynrClusterController_INCLUDE_DIRECTORIES
    ${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}
    "../"
)

set(
    JoynrClusterController_SYS_INCLUDE_DIRECTORIES
    ${CURL_INCLUDE_DIRS}
    ${MOSQUITTO_INCLUDE_DIRS}
)

set(
    JoynrClusterController_TARGET_LIBRARIES
    Joynr
    cURL::curl
)

set(
    JoynrClusterController_EXPORT_HEADER
    "include/joynr/JoynrClusterControllerExport.h"
)


file(
    GLOB JoynrClusterController_PUBLIC_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "include/joynr/*.h"
)

file(
    GLOB_RECURSE JoynrClusterController_PRIVATE_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "access-control/*.h"
    "capabilities-client/*.h"
    "http-communication-manager/*.h"
    "httpnetworking/*.h"
    "ClusterControllerDirectories.h"
    "ICommunicationManager.h"
    "messaging/*.h"
    "messaging/in-process/*.h"
    "messaging/joynr-messaging/*.h"
    "mqtt/*.h"
    "websocket/*.h"
)

set(
    JoynrClusterController_HEADERS
    ${JoynrClusterController_PUBLIC_HEADERS}
    ${JoynrClusterController_PRIVATE_HEADERS}
)


file(
    GLOB JoynrClusterController_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "access-control/*.cpp"
    "capabilities-client/*.cpp"
    "http-communication-manager/*.cpp"
    "httpnetworking/*.cpp"
    "messaging/*.cpp"
    "messaging/in-process/*.cpp"
    "messaging/joynr-messaging/*.cpp"
    "mqtt/*.cpp"
    "websocket/*.cpp"
    "ClusterControllerSettings.cpp"
    "ClusterControllerCallContext.cpp"
)

add_library(
    JoynrClusterController
    # library type is set via BUILD_SHARED_LIBS
    ${JoynrClusterController_HEADERS}
    ${JoynrClusterController_SOURCES}
)

target_include_directories(
    JoynrClusterController
    PRIVATE ${JoynrClusterController_INCLUDE_DIRECTORIES}
)

target_include_directories(
    JoynrClusterController
    SYSTEM
    PRIVATE ${JoynrClusterController_SYS_INCLUDE_DIRECTORIES}
)

if(NOT USE_PLATFORM_MOSQUITTO)
    # make sure the external project mosquitto is built before this
    add_dependencies(JoynrClusterController mosquitto::mosquitto)
endif(NOT USE_PLATFORM_MOSQUITTO)

# manually add dependency to target
list(
    APPEND JoynrClusterController_TARGET_LIBRARIES
    mosquitto::mosquitto
    MoCOCrW::mococrw
)


AddClangFormat(JoynrClusterController ${JoynrClusterController_EXPORT_HEADER})

AddIncludeWhatYouUse(JoynrClusterController)

AddClangTidy(JoynrClusterController)

set_target_properties(
    JoynrClusterController
    PROPERTIES
    SOVERSION "${JOYNR_SOVERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}"
)

include(GenerateJoynrExports)
GenerateJoynrExports(
    JoynrClusterController
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${JoynrClusterController_EXPORT_HEADER}"
)

target_link_libraries(JoynrClusterController ${JoynrClusterController_TARGET_LIBRARIES})

install(
    TARGETS JoynrClusterController
    EXPORT JoynrTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    # this will flatten directory hierarchy of headder files
    # PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

# Visual Studio compiles to Debug and Release directories
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-messaging.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-messaging.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-clustercontroller.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-clustercontroller.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/resources/default-websocket.settings
        @ONLY
    )
else()
    # copy resources for build tree
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-messaging.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-messaging.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-websocket.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-websocket.settings
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/default-clustercontroller.settings
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-clustercontroller.settings
        @ONLY
    )
    # ...and for install tree
    install(
        FILES
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-messaging.settings
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-websocket.settings
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-clustercontroller.settings
        DESTINATION ${JOYNR_INSTALL_FULL_SYSCONFDIR}
    )
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")
