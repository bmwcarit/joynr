# Parts of this file are inspired by a CMake tutorial (http://www.cmake.org/Wiki/CMake/)
# that is available under Creative Commons Attribution 2.5 Generic (CC BY 2.5, http://creativecommons.org/licenses/by/2.5/).
# There are no attributions specified by the author.

cmake_minimum_required(VERSION 3.10)

project(Joynr VERSION 1.19.4 LANGUAGES CXX)
message(STATUS "-----------------------------------------------------")
message(STATUS "- PROJECT: Joynr ${Joynr_VERSION}")
message(STATUS "-----------------------------------------------------")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(FindPkgConfig)
include(GenerateExportHeader)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

include(AddClangFormat)
include(AddClangTidy)
include(AddIncludeWhatYouUse)
include(LocateProgram)
include(Sanitizers)
include(ObjectLibraryTargetUtils)
include(OutofdirTargetSources)

######## Project settings ########
set(Joynr_SOVERSION ${Joynr_VERSION_MAJOR}.${Joynr_VERSION_MINOR})

if(NOT JOYNR_GIT_VERSION)
  find_package(Git REQUIRED)
  execute_process(COMMAND ${GIT_EXECUTABLE} describe --long --tags --dirty --always
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                  OUTPUT_VARIABLE JOYNR_GIT_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_QUIET)
endif()
string(TIMESTAMP JOYNR_BUILD_TIME)

set(JOYNR_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/joynr-${Joynr_SOVERSION}")
set(JOYNR_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
set(JOYNR_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}")
set(JOYNR_INSTALL_DOCDIR "${CMAKE_INSTALL_DOCDIR}joynr-${Joynr_SOVERSION}")
set(JOYNR_INSTALL_CMAKECONFDIR "${CMAKE_INSTALL_LIBDIR}/cmake/joynr-${Joynr_SOVERSION}")
set(JOYNR_INSTALL_FULL_SYSCONFDIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/joynr")

if(NOT JOYNR_INSTALL_TEST_DIR)
    set(JOYNR_INSTALL_TEST_DIR "${JOYNR_INSTALL_BINDIR}")
endif(NOT JOYNR_INSTALL_TEST_DIR)
set(JOYNR_INSTALL_RESOURCESDIR "${JOYNR_INSTALL_TEST_DIR}/resources")

message(STATUS "----------------- INSTALL LOCATIONS -----------------")
message(STATUS "CMAKE_INSTALL_PREFIX          = ${CMAKE_INSTALL_PREFIX}")
message(STATUS "JOYNR_INSTALL_INCLUDEDIR      = ${JOYNR_INSTALL_INCLUDEDIR}")
message(STATUS "JOYNR_INSTALL_LIBDIR          = ${JOYNR_INSTALL_LIBDIR}")
message(STATUS "JOYNR_INSTALL_BINDIR          = ${JOYNR_INSTALL_BINDIR}")
message(STATUS "JOYNR_INSTALL_DOCDIR          = ${JOYNR_INSTALL_DOCDIR}")
message(STATUS "JOYNR_INSTALL_TEST_DIR        = ${JOYNR_INSTALL_TEST_DIR}")
message(STATUS "JOYNR_INSTALL_CMAKECONFDIR    = ${JOYNR_INSTALL_CMAKECONFDIR}")
message(STATUS "JOYNR_INSTALL_FULL_SYSCONFDIR = ${JOYNR_INSTALL_FULL_SYSCONFDIR}")
message(STATUS "-----------------------------------------------------")

option(JOYNR_ENABLE_DLT_LOGGING "Use DLT logger?" OFF)
option(JOYNR_SUPPORT_WEBSOCKET "Support WebSocket interface" ON)
option(JOYNR_SUPPORT_UDS "Support Unix Domain Socket (UDS) interface" ON)

option(GENERATE_SOURCES "Generate source files from Franca model?" OFF)
if(${GENERATE_SOURCES})
    find_package(JoynrGenerator ${Joynr_VERSION} EXACT REQUIRED)
endif(${GENERATE_SOURCES})

#### BEGIN Globally required dependencies ####
if(JOYNR_SUPPORT_UDS)
# As we use some features of Boost 1.66 in UDS, it is required for any build that compiles it.
    set(JOYNR_BOOST_REQUIRED_VERSION 1.66.0)
else()
# The setting for 1.65 is necessary to stay compatible with older build systems, where its is the
# maximum available boost version. These systems will not be able to use UDS.
    set(JOYNR_BOOST_REQUIRED_VERSION 1.65.0)
endif(JOYNR_SUPPORT_UDS)
set(JOYNR_BOOST_REQUIRED_COMPONENTS system filesystem thread)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost ${JOYNR_BOOST_REQUIRED_VERSION} REQUIRED
    COMPONENTS ${JOYNR_BOOST_REQUIRED_COMPONENTS}
)

set(JOYNR_DLT_REQUIRED_VERSION 2.15.0)
if(JOYNR_ENABLE_DLT_LOGGING)
    pkg_check_modules(DLT REQUIRED IMPORTED_TARGET automotive-dlt>=${JOYNR_DLT_REQUIRED_VERSION})
endif()

set(JOYNR_SPDLOG_REQUIRED_VERSION 1.4.2)
find_package(spdlog ${JOYNR_SPDLOG_REQUIRED_VERSION} REQUIRED)

set(JOYNR_WEBSOCKETPP_REQUIRED_VERSION 0.8.0)
# websocketpp-config.cmake iw not guarding target addition; workaround: check it here.
if(NOT TARGET websocketpp::websocketpp)
    find_package(websocketpp ${JOYNR_WEBSOCKETPP_REQUIRED_VERSION} REQUIRED)
endif()

set(JOYNR_MOCOCRW_REQUIRED_VERSION 0.1.0)
find_package(MoCOCrW ${JOYNR_MOCOCRW_REQUIRED_VERSION} REQUIRED CONFIG)

set(JOYNR_MUESLI_REQUIRED_VERSION 1.0.2)
find_package(muesli ${JOYNR_MUESLI_REQUIRED_VERSION} REQUIRED CONFIG)

set(JOYNR_SMRF_REQUIRED_VERSION 0.3.4)
find_package(smrf ${JOYNR_SMRF_REQUIRED_VERSION} REQUIRED CONFIG)

set(JOYNR_MOSQUITTO_REQUIRED_VERSION 1.6.8)
find_package(mosquitto ${JOYNR_MOSQUITTO_REQUIRED_VERSION} REQUIRED)
include(CheckMosquittoImportTargets)

#### END Globally required dependencies ####

option(
    ENABLE_GCOV
    "Use GCOV to meassure test code coverage?"
    OFF
)
if(${ENABLE_GCOV})
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/code-coverage")
    include(CodeCoverage)
    setup_target_for_coverage("UnitCoverageHtml" g_UnitTests "coverage")
    setup_target_for_coverage_cobertura("UnitCoverageTarget" g_UnitTests "coverage" "--gtest_output=xml:g_UnitTests.junit.xml" "'.*/libs/.*|.*/build/.*|.*/tests/.*|.*/generated/.*'")
    # copy the resource folder to the build folder, because some files are required for the tests
    add_custom_target(CopyResourceFolderToBuildDir
        cp -r bin/resources . && cp -r bin/test-resources .

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Copy resources to build directory."
    )
    add_dependencies(UnitCoverageTarget CopyResourceFolderToBuildDir)
    #setup_target_for_coverage_cobertura("IntegrationCoverageTarget" g_IntegrationTests "integrationCoverage.xml")
    message(STATUS "setting flags to measure test coverage.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    #set(LDFLAGS "{$LDFLAGS} -fprofile-arcs -ftest-coverage")
endif(${ENABLE_GCOV})

# Doxygen documentation build
option(
    ENABLE_DOXYGEN
    "Use DOXYGEN to build documentation?"
    ON
)
if(ENABLE_DOXYGEN)
    find_package(Doxygen REQUIRED)
    add_custom_target(doxygen
        COMMAND /bin/bash "${CMAKE_SOURCE_DIR}/run_doxygen.sh" "${DOXYGEN_EXECUTABLE}" "${CMAKE_SOURCE_DIR}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Creating doxygen documentation"
        VERBATIM
    )
endif(ENABLE_DOXYGEN)

#### BEGIN variables for resource settings ####

if(NOT DEFINED JOYNR_DEFAULT_GBID)
    set(JOYNR_DEFAULT_GBID "joynrdefaultgbid")
endif(NOT DEFINED JOYNR_DEFAULT_GBID)
message(STATUS "variable JOYNR_DEFAULT_GBID=" ${JOYNR_DEFAULT_GBID})

if(NOT DEFINED JOYNR_SERVER_HOST)
    set(JOYNR_SERVER_HOST "localhost")
endif(NOT DEFINED JOYNR_SERVER_HOST)
message(STATUS "variable JOYNR_SERVER_HOST=" ${JOYNR_SERVER_HOST})

if(NOT DEFINED JOYNR_SERVER_MQTT_PORT)
    set(JOYNR_SERVER_MQTT_PORT "1883")
endif(NOT DEFINED JOYNR_SERVER_MQTT_PORT)
message(STATUS "variable JOYNR_SERVER_MQTT_PORT=" ${JOYNR_SERVER_MQTT_PORT})

if(NOT DEFINED JOYNR_SERVER_MQTT_TLS_PORT)
    set(JOYNR_SERVER_MQTT_TLS_PORT "8883")
endif(NOT DEFINED JOYNR_SERVER_MQTT_TLS_PORT)
message(STATUS "variable JOYNR_SERVER_MQTT_TLS_PORT=" ${JOYNR_SERVER_MQTT_TLS_PORT})

#### END variables for resource settings ####

# Set the binary directory
set(JOYNR_BINARY_DIR "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${JOYNR_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${JOYNR_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${JOYNR_BINARY_DIR})

######## DEFAULT COMPILER FLAGS #############

set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(
    -Wall -Wextra -pedantic
    "$<$<CONFIG:Debug>:-ggdb;-O0>"
    "$<IF:$<CXX_COMPILER_ID:GNU>,,-Wno-unknown-warning-option>" # For: #pragma GCC diagnostic ignored "-Wunsafe-loop-optimizations"
)

if(NOT CMAKE_BUILD_TYPE MATCHES "(Debug)|(Coverage)")
    add_definitions(-D_FORTIFY_SOURCE=2)
endif()

add_compile_options(
    -Walloc-zero
    -Walloca
    -Wcast-align
    -Wcast-qual
    -Wduplicated-branches
    -Wduplicated-cond
    -Werror=all
    -Werror=alloc-zero
    -Werror=alloca
    -Werror=cast-align
    -Werror=cast-qual
    -Werror=duplicated-branches
    -Werror=duplicated-cond
    -Werror=extra
    -Werror=float-equal
    -Werror=format
    -Werror=format-security
    -Werror=format-security
    -Werror=format-signedness
    -Werror=implicit-fallthrough
    -Werror=logical-op
    -Werror=missing-include-dirs
    -Werror=non-virtual-dtor
    -Werror=redundant-decls
    -Werror=shadow
    -Werror=sign-conversion
    -Werror=uninitialized
    -Werror=unused-parameter
    -Wfloat-equal
    -Wformat-security
    -Wformat-signedness
    -Wformat=2
    -Wimplicit-fallthrough=5
    -Wlogical-op
    -Wmissing-include-dirs
    -Wredundant-decls
    -Wshadow
    -Wsign-conversion
    -Wuninitialized
    -Wunused-parameter
    -fstack-protector-strong
    -fwrapv
)

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-D_GLIBCXX_ASSERTIONS>)

add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wdeclaration-after-statement>)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Werror=declaration-after-statement>)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Werror=missing-prototypes>)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Werror=nested-externs>)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Werror=old-style-definition>)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wmissing-prototypes>)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wnested-externs>)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wold-style-definition>)

option(ENABLE_CCACHE "Should ccache be used?" OFF)
if(ENABLE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_FOUND})
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_FOUND})
    else(CCACHE_FOUND)
        message(WARNING "ccache was not found. Fall-back to default compiler.")
    endif(CCACHE_FOUND)
endif(ENABLE_CCACHE)

option(USE_LD_GOLD "Use GNU gold linker" ON)
if(USE_LD_GOLD)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -fuse-ld=gold -Wl,--version OUTPUT_VARIABLE stdout ERROR_QUIET)
    if("${stdout}" MATCHES "GNU gold")
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
            add_link_options("-fuse-ld=gold")
        else()
            list(APPEND CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=gold")
        endif()
    else()
        message(WARNING "GNU gold linker isn't available, using the default system linker.")
    endif()
endif()

######## Add submodules ########

add_subdirectory(libjoynr)
add_subdirectory(libjoynrclustercontroller)
add_subdirectory(runtimes)

option(BUILD_CLUSTER_CONTROLLER "Build cluster-controller executable?" ON)
if(${BUILD_CLUSTER_CONTROLLER})
    add_subdirectory(cluster-controller)
endif(${BUILD_CLUSTER_CONTROLLER})

option(BUILD_TESTS "Build test binaries?" ON)
if(${BUILD_TESTS})
    add_subdirectory(tests)
endif(${BUILD_TESTS})

### Add Config files to project so that can be shown in the editor
file(GLOB_RECURSE ADD_ALL_CMAKE_FILE_TO_PROJECT
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "CMake/*"
)
add_custom_target(NOOP_SHOW_CMAKE_FILES SOURCES ${ADD_ALL_CMAKE_FILE_TO_PROJECT})

######## Install targets ########

# export all targets from the build tree to use by outside projects
export(
    EXPORT JoynrTargets
    NAMESPACE Joynr::
    FILE "${JOYNR_BINARY_DIR}/JoynrTargets.cmake"
)

set(JoynrConfig_RESOURCES_DIR "${JOYNR_INSTALL_RESOURCESDIR}")
configure_package_config_file(
    JoynrConfig.cmake.in
    "${JOYNR_BINARY_DIR}/JoynrConfig.cmake"
    INSTALL_PREFIX "${CMAKE_BINARY_DIR}"
    INSTALL_DESTINATION "${JOYNR_BINARY_DIR}"
    PATH_VARS JOYNR_INSTALL_BINDIR JoynrConfig_RESOURCES_DIR
)
write_basic_package_version_file(
    "${JOYNR_BINARY_DIR}/JoynrConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# ... for the install tree

# install the export targets for use with the install-tree
install(
    EXPORT JoynrTargets
    DESTINATION "${JOYNR_INSTALL_CMAKECONFDIR}"
    NAMESPACE Joynr::
    FILE "JoynrTargets.cmake"
)

set(JoynrConfig_RESOURCES_DIR "${JOYNR_INSTALL_FULL_SYSCONFDIR}")
configure_package_config_file(
    JoynrConfig.cmake.in
    "JoynrConfig.cmake.install-tree"
    INSTALL_DESTINATION "${JOYNR_INSTALL_CMAKECONFDIR}"
    PATH_VARS JOYNR_INSTALL_BINDIR JoynrConfig_RESOURCES_DIR
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/JoynrConfig.cmake.install-tree"
    DESTINATION "${JOYNR_INSTALL_CMAKECONFDIR}"
    RENAME "JoynrConfig.cmake"
)

install(
    FILES "${JOYNR_BINARY_DIR}/JoynrConfigVersion.cmake"
    DESTINATION "${JOYNR_INSTALL_CMAKECONFDIR}"
)

# unset variable to avoid confusion
unset(JoynrConfig_RESOURCES_DIR)
