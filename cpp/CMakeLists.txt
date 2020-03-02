# Parts of this file are inspired by a CMake tutorial (http://www.cmake.org/Wiki/CMake/)
# that is available under Creative Commons Attribution 2.5 Generic (CC BY 2.5, http://creativecommons.org/licenses/by/2.5/).
# There are no attributions specified by the author.

cmake_minimum_required(VERSION 2.8.11)

######## Project settings ########
project(joynr-1.14.0-SNAPSHOT)
set(JOYNR_MAJOR_VERSION 1)
set(JOYNR_MINOR_VERSION 14)
set(JOYNR_PATCH_VERSION 0)
set(
    JOYNR_VERSION
    ${JOYNR_MAJOR_VERSION}.${JOYNR_MINOR_VERSION}.${JOYNR_PATCH_VERSION}
)

set(
    JOYNR_SOVERSION
    ${JOYNR_MAJOR_VERSION}.${JOYNR_MINOR_VERSION}
)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "setting default value 'Debug' for CMAKE_BUILD_TYPE")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

set_property(
    CACHE
    CMAKE_BUILD_TYPE
    PROPERTY STRINGS
    "Debug"
    "Release"
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
include(Sanitizers)

message(STATUS "variable CMAKE_MODULE_PATH=" ${CMAKE_MODULE_PATH})
message(STATUS "variable CMAKE_PREFIX_PATH=" ${CMAKE_PREFIX_PATH})
message(STATUS "variable CMAKE_VERSION=" ${CMAKE_VERSION})
message(STATUS "variable CMAKE_ROOT=" ${CMAKE_ROOT})
message(STATUS "variable CMAKE_SOURCE_DIR=" ${CMAKE_SOURCE_DIR})
message(STATUS "variable CMAKE_CURRENT_SOURCE_DIR=" ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "variable BUILD_PACKAGE_SHLIB=" ${BUILD_PACKAGE_SHLIB})
message(STATUS "variable BUILD_PACKAGE_BIN=" ${BUILD_PACKAGE_BIN})
message(STATUS "variable BUILD_PACKAGE_DEV=" ${BUILD_PACKAGE_DEV})
message(STATUS "variable BUILD_PACKAGE_LIBRARY=" ${BUILD_PACKAGE_LIBRARY})
message(STATUS "variable BUILD_PACKAGE_TESTS=" ${BUILD_PACKAGE_TESTS})

# defines GNU standard installation directories
include(GNUInstallDirs)
set(INSTALL_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/joynr CACHE PATH "Installation sub directory for CMake files")
set(JOYNR_INSTALL_FULL_SYSCONFDIR ${CMAKE_INSTALL_FULL_SYSCONFDIR}/joynr CACHE PATH "Installation directory (absolute path) for resources and settings files")

if(NOT INSTALL_TEST_DIR)
    set(INSTALL_TEST_DIR "${CMAKE_INSTALL_BINDIR}" CACHE PATH "Installation sub directory for tests")
endif(NOT INSTALL_TEST_DIR)

message(STATUS "----------------- INSTALL LOCATIONS -----------------")
message(STATUS "CMAKE_INSTALL_PREFIX          = ${CMAKE_INSTALL_PREFIX}")
message(STATUS "CMAKE_INSTALL_BINDIR          = ${CMAKE_INSTALL_BINDIR}")
message(STATUS "INSTALL_TEST_DIR              = ${INSTALL_TEST_DIR}")
message(STATUS "CMAKE_INSTALL_LIBDIR          = ${CMAKE_INSTALL_LIBDIR}")
message(STATUS "INSTALL_CMAKE_DIR             = ${INSTALL_CMAKE_DIR}")
message(STATUS "CMAKE_INSTALL_INCLUDEDIR      = ${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "CMAKE_INSTALL_SYSCONFDIR      = ${CMAKE_INSTALL_SYSCONFDIR}")
message(STATUS "CMAKE_INSTALL_FULL_SYSCONFDIR = ${CMAKE_INSTALL_FULL_SYSCONFDIR}")
message(STATUS "JOYNR_INSTALL_FULL_SYSCONFDIR = ${JOYNR_INSTALL_FULL_SYSCONFDIR}")
message(STATUS "-----------------------------------------------------")

option(
    ENABLE_CCACHE
    "Should ccache be used?"
    OFF
)
message(STATUS "option ENABLE_CCACHE=" ${ENABLE_CCACHE})

if(ENABLE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_FOUND})
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_FOUND})
    else(CCACHE_FOUND)
        message(WARNING "ccache was not found. Fall-back to default compiler.")
    endif(CCACHE_FOUND)
endif(ENABLE_CCACHE)

option(
    USE_LD_GOLD
    "Use GNU gold linker"
    ON
)

if(USE_LD_GOLD AND "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
  execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version OUTPUT_VARIABLE stdout ERROR_QUIET)
  if("${stdout}" MATCHES "GNU gold")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fuse-ld=gold")
  else()
    message(WARNING "GNU gold linker isn't available, using the default system linker.")
  endif()
endif()

option(
    GENERATE_SOURCES
    "Generate source files from Franca model?"
    OFF
)
message(STATUS "option GENERATE_SOURCES=" ${GENERATE_SOURCES})
if(${GENERATE_SOURCES})
    find_package(JoynrGenerator ${JOYNR_VERSION} EXACT REQUIRED)
    message(STATUS "variable JoynrGenerator_FOUND=${JoynrGenerator_FOUND}")
    message(STATUS "variable JoynrGenerator_JAR=${JoynrGenerator_JAR}")
endif(${GENERATE_SOURCES})

option(
    USE_PLATFORM_SPDLOG
    "Resolve dependency to spdlog from the system?"
    ON
)
message(STATUS "option USE_PLATFORM_SPDLOG=" ${USE_PLATFORM_SPDLOG})

option(
    USE_PLATFORM_MUESLI
    "Resolve dependency to muesli from the system?"
    ON
)
message(STATUS "option USE_PLATFORM_MUESLI=" ${USE_PLATFORM_MUESLI})

option(
    USE_PLATFORM_MOSQUITTO
    "Resolve dependency to mosquitto from the system?"
    ON
)
message(STATUS "option USE_PLATFORM_MOSQUITTO=" ${USE_PLATFORM_MOSQUITTO})

option(
    USE_PLATFORM_GTEST_GMOCK
    "Resolve dependency to gtest and gmock from the system?"
    ON
)
message(STATUS "option USE_PLATFORM_GTEST_GMOCK=" ${USE_PLATFORM_GTEST_GMOCK})

option(
    USE_PLATFORM_WEBSOCKETPP
    "Resolve dependency to WebSocket++ from the system?"
    ON
)
message(STATUS "option USE_PLATFORM_WEBSOCKETPP=" ${USE_PLATFORM_WEBSOCKETPP})

option(
    JOYNR_ENABLE_DLT_LOGGING
    "Use DLT logger?"
    OFF
)
message(STATUS "option JOYNR_ENABLE_DLT_LOGGING=" ${JOYNR_ENABLE_DLT_LOGGING})

if(JOYNR_ENABLE_DLT_LOGGING)
option(
    USE_PLATFORM_DLT
    "Resolve dependency to DLT from the system?"
    ON
)
message(STATUS "option USE_PLATFORM_DLT=" ${USE_PLATFORM_DLT})
endif(JOYNR_ENABLE_DLT_LOGGING)

option(
    JOYNR_ENABLE_STDOUT_LOGGING
    "Use stdout logger?"
    ON
)
message(STATUS "option JOYNR_ENABLE_STDOUT_LOGGING=" ${JOYNR_ENABLE_STDOUT_LOGGING})

option(
    JOYNR_DISABLE_DEBUG_TO_INFO_LOGGING
    "Don't map debug loglevel to info loglevel?"
    OFF
)
message(STATUS "option JOYNR_DISABLE_DEBUG_TO_INFO_LOGGING=" ${JOYNR_DISABLE_DEBUG_TO_INFO_LOGGING})

option(
    USE_PLATFORM_SMRF
    "Resolve dependency to SMRF from the system?"
    ON
)
message(STATUS "option USE_PLATFORM_SMRF=" ${USE_PLATFORM_SMRF})

option(
    USE_PLATFORM_MOCOCRW
    "Resolve dependency to MOCOCRW from the system?"
    ON
)
message(STATUS "option USE_PLATFORM_MOCOCRW=" ${USE_PLATFORM_MOCOCRW})

option(
    BUILD_TESTS
    "Build test binaries?"
    ON
)
message(STATUS "option BUILD_TESTS=" ${BUILD_TESTS})

option(
    ENABLE_GCOV
    "Use GCOV to meassure test code coverage?"
    OFF
)
message(STATUS "option ENABLE_GCOV=" ${ENABLE_GCOV})

option(
    ENABLE_DOXYGEN
    "Use DOXYGEN to build documentation?"
    ON
)
message(STATUS "option ENABLE_DOXYGEN=" ${ENABLE_DOXYGEN})

option(
    ENABLE_MQTT_OCSP
    "Enable OCSP in mosquitto?"
    OFF
)
message(STATUS "option ENABLE_MQTT_OCSP=" ${ENABLE_MQTT_OCSP})

option(
    BUILD_CLUSTER_CONTROLLER
    "Build cluster-controller executable?"
    ON
)
message(STATUS "option BUILD_CLUSTER_CONTROLLER=" ${BUILD_CLUSTER_CONTROLLER})

if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
endif()

message(STATUS "option BUILD_SHARED_LIBS=" ${BUILD_SHARED_LIBS})

include(LocateProgram)
include(AddClangFormat)

#### BEGIN: Prevent install-target execution if platform dependencies are not used ####

# Following dependencies must be installed by the user beforehand because they either
# use a shared object or are included through joynr header files into the user's code.
set (CHECK_DLT (JOYNR_ENABLE_DLT_LOGGING AND USE_PLATFORM_DLT OR NOT JOYNR_ENABLE_DLT_LOGGING))
if(NOT (USE_PLATFORM_SPDLOG AND CHECK_DLT AND USE_PLATFORM_MOSQUITTO AND USE_PLATFORM_SMRF AND USE_PLATFORM_MOCOCRW))
    set(codeMsg "message(FATAL_ERROR \"Install target is not available as long as external platform")
    set(codeMsg "${codeMsg} dependencies are used. For more information see USE_PLATFORM_SPDLOG,")
    set(codeMsg "${codeMsg} USE_PLATFORM_DLT, USE_PLATFORM_MOSQUITTO and USE_PLATFORM_SMRF and USE_PLATFORM_MOCOCRW.\")")
    install(CODE "${codeMsg}")
endif()

#### END: Prevent install-target execution if platform dependencies are not used #####

#### BEGIN include-what-you-use ####

# include-what-you-use can be used with CMake 3.3.0 and higher
if("${CMAKE_VERSION}" VERSION_GREATER 3.3.0)

    option(
        ENABLE_INCLUDE_WHAT_YOU_USE
        "Use include-what-you-use to identify missing/superfluous includes?"
        OFF
    )

    message(STATUS "option ENABLE_INCLUDE_WHAT_YOU_USE=" ${ENABLE_INCLUDE_WHAT_YOU_USE})

    if(${ENABLE_INCLUDE_WHAT_YOU_USE})
        LocateProgram(include-what-you-use INCLUDE_WHAT_YOU_USE_PATH)
    endif(${ENABLE_INCLUDE_WHAT_YOU_USE})

    set(INCLUDE_WHAT_YOU_USE_OPTIONS "" CACHE STRING "additional options for include-what-you-use")

    set(INCLUDE_WHAT_YOU_USE_PATH_AND_OPTIONS
        ${INCLUDE_WHAT_YOU_USE_PATH}
        ${INCLUDE_WHAT_YOU_USE_OPTIONS}
    )

endif("${CMAKE_VERSION}" VERSION_GREATER 3.3.0)

include(AddIncludeWhatYouUse)

#### END include-what-you-use ####


#### BEGIN clang-tidy ####

option(
    ENABLE_CLANG_TIDY
    "Use clang-tidy for code analysis/cleanup?"
    OFF
)

message(STATUS "option ENABLE_CLANG_TIDY=" ${ENABLE_CLANG_TIDY})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(${ENABLE_CLANG_TIDY})
    LocateProgram(clang-tidy CLANG_TIDY_PATH)
    add_custom_target(tidy)

    set(
        CLANG_TIDY_OPTIONS
        -checks=*
        -header-filter=.*joynr.*
        CACHE LIST "additional options for clang-tidy")

    set(CLANG_TIDY_TARGET_FILES "" CACHE LIST "files which will be cleaned up by clang-tidy")
    option(CLANG_TIDY_APPLY_FIXES "files which will be cleaned up by clang-tidy" ON)
    option(CLANG_TIDY_FORMAT "format files after applying fixes" ON)

    set(CLANG_TIDY_FIXES_PATH "${CMAKE_BINARY_DIR}/tidy-fixes/" CACHE FILEPATH "location of exported fixes from 'clang-tidy'")
    file(MAKE_DIRECTORY ${CLANG_TIDY_FIXES_PATH})

    if(${CLANG_TIDY_APPLY_FIXES})
        LocateProgram(clang-apply-replacements CLANG_APPLY_REPLACEMENTS_PATH)

        set(CLANG_APPLY_REPLACEMENTS_OPTIONS "-remove-change-desc-files")
        if(${CLANG_TIDY_FORMAT})
            set(CLANG_APPLY_REPLACEMENTS_OPTIONS ${CLANG_APPLY_REPLACEMENTS_OPTIONS} "-format")
        endif(${CLANG_TIDY_FORMAT})

        add_custom_command(TARGET tidy
                           POST_BUILD
                           COMMAND ${CLANG_APPLY_REPLACEMENTS_PATH} ${CLANG_APPLY_REPLACEMENTS_OPTIONS} ${CLANG_TIDY_FIXES_PATH}
                           VERBATIM
        )
    endif(${CLANG_TIDY_APPLY_FIXES})

endif(${ENABLE_CLANG_TIDY})

include(AddClangTidy)

#### END clang-tidy ####

if(NOT DEFINED JOYNR_DEFAULT_GBID)
    set(JOYNR_DEFAULT_GBID "joynrdefaultgbid")
endif(NOT DEFINED JOYNR_DEFAULT_GBID)
message(STATUS "variable JOYNR_DEFAULT_GBID=" ${JOYNR_DEFAULT_GBID})

if(NOT DEFINED JOYNR_SERVER_HOST)
    set(JOYNR_SERVER_HOST "localhost")
endif(NOT DEFINED JOYNR_SERVER_HOST)
message(STATUS "variable JOYNR_SERVER_HOST=" ${JOYNR_SERVER_HOST})

message(STATUS "variable CMAKE_BUILD_TYPE=" ${CMAKE_BUILD_TYPE})

if(NOT DEFINED JOYNR_SERVER_HTTP_PORT)
    set(JOYNR_SERVER_HTTP_PORT "8080")
endif(NOT DEFINED JOYNR_SERVER_HTTP_PORT)
message(STATUS "variable JOYNR_SERVER_HTTP_PORT=" ${JOYNR_SERVER_HTTP_PORT})

if(NOT DEFINED JOYNR_SERVER_MQTT_PORT)
    set(JOYNR_SERVER_MQTT_PORT "1883")
endif(NOT DEFINED JOYNR_SERVER_MQTT_PORT)
message(STATUS "variable JOYNR_SERVER_MQTT_PORT=" ${JOYNR_SERVER_MQTT_PORT})

if(NOT DEFINED JOYNR_SERVER_MQTT_TLS_PORT)
    set(JOYNR_SERVER_MQTT_TLS_PORT "8883")
endif(NOT DEFINED JOYNR_SERVER_MQTT_TLS_PORT)
message(STATUS "variable JOYNR_SERVER_MQTT_TLS_PORT=" ${JOYNR_SERVER_MQTT_TLS_PORT})

# Set the binary directory
set(JOYNR_BINARY_DIR ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
message(STATUS "variable JOYNR_BINARY_DIR=" ${JOYNR_BINARY_DIR})

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
if(ENABLE_DOXYGEN)
    find_package(Doxygen REQUIRED)
    message(STATUS "doxygen DOXYGEN_FOUND=${DOXYGEN_FOUND}")
    add_custom_target(doxygen
        COMMAND /bin/bash ${CMAKE_SOURCE_DIR}/run_doxygen.sh ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Creating doxygen documentation"
        VERBATIM
    )
endif(ENABLE_DOXYGEN)

######## Add Boost ########

set(JOYNR_BOOST_REQUIRED_COMPONENTS
    system
    filesystem
    thread)
set(JOYNR_BOOST_REQUIRED_VERSION 1.58.0)
find_package(Boost ${JOYNR_BOOST_REQUIRED_VERSION} REQUIRED
    COMPONENTS ${JOYNR_BOOST_REQUIRED_COMPONENTS}
)
include(CheckBoostImportTargets)

include_directories(${Boost_INCLUDE_DIRS})

message(STATUS "############ Import: cURL ############")
if(${WIN32})
    if(NOT CURL_HOME)
        if(DEFINED ENV{CURL_HOME})
            message(STATUS "setting CURL_HOME from environment variable")
            message(STATUS "environment variable CURL_HOME=" $ENV{CURL_HOME})
            set(CURL_HOME $ENV{CURL_HOME})
        else()
            message(FATAL_ERROR "CURL_HOME variable not set")
        endif()
    endif(NOT CURL_HOME)
    message(STATUS "variable CURL_HOME=" ${CURL_HOME})

    find_path(CURL_INCLUDE_DIRS
        NAMES curl/curl.h
        PATHS ${CURL_HOME}
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
    )

    set(curl_version_header ${CURL_INCLUDE_DIRS}/curl/curlver.h)
    if(EXISTS ${curl_version_header})
        file(STRINGS ${curl_version_header} curl_version_define REGEX "^#define[\t ]+LIBCURL_VERSION[\t ]+\".*\"")
        string(REGEX REPLACE "^#define[\t ]+LIBCURL_VERSION[\t ]+\"([^\"]*)\".*" "\\1" CURL_VERSION_STRING "${curl_version_define}")
        unset(curl_version_define)
    endif()
    unset(curl_version_header)

    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        message(AUTHOR_WARNING "When using the Visual Studio makefile generator, no prefixes are used to find libraries. Therefore, we add additional prefixes here to CMAKE_FIND_LIBRARY_PREFIXES.")
        message(AUTHOR_WARNING "When using the Visual Studio makefile generator, only .lib suffixes are used to find libraries. Therefore, we add additional suffixes here to CMAKE_FIND_LIBRARY_SUFFIXES.")
        # saving the old state of CMAKE_FIND_LIBRARY_*
        set(CMAKE_FIND_LIBRARY_PREFIXES_org ${CMAKE_FIND_LIBRARY_PREFIXES})
        set(CMAKE_FIND_LIBRARY_SUFFIXES_org ${CMAKE_FIND_LIBRARY_SUFFIXES})
        message(STATUS "variable CMAKE_FIND_LIBRARY_PREFIXES_org=" ${CMAKE_FIND_LIBRARY_PREFIXES_org})
        message(STATUS "variable CMAKE_FIND_LIBRARY_SUFFIXES_org=" ${CMAKE_FIND_LIBRARY_SUFFIXES_org})
        # adding additional prefixes and suffixes
        set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "lib")
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES} ".dll" ".dll.a" ".a")
    endif(CMAKE_GENERATOR MATCHES "Visual Studio")

    message(STATUS "variable CMAKE_FIND_LIBRARY_PREFIXES=" ${CMAKE_FIND_LIBRARY_PREFIXES})
    message(STATUS "variable CMAKE_FIND_LIBRARY_SUFFIXES=" ${CMAKE_FIND_LIBRARY_SUFFIXES})

    find_library(CURL_LIBRARIES
        NAMES curldll
        PATHS ${CURL_HOME}
        PATH_SUFFIXES lib
        NO_DEFAULT_PATH
    )

    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        # restoring old state of CMAKE_FIND_LIBRARY_*
        # NOTE: We need quotes around the variables here, because they could be empty.
        #       This would cause the variable to be unset.
        set(CMAKE_FIND_LIBRARY_PREFIXES "${CMAKE_FIND_LIBRARY_PREFIXES_org}")
        set(CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_FIND_LIBRARY_SUFFIXES_org}")
        message(STATUS "variable restored CMAKE_FIND_LIBRARY_PREFIXES=" ${CMAKE_FIND_LIBRARY_PREFIXES})
        message(STATUS "variable restored CMAKE_FIND_LIBRARY_SUFFIXES=" ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif(CMAKE_GENERATOR MATCHES "Visual Studio")

    if(CURL_INCLUDE_DIRS AND CURL_VERSION_STRING AND CURL_LIBRARIES)
        set(CURL_FOUND TRUE)
    else()
        set(CURL_FOUND FALSE)
    endif()
else(${WIN32})
    find_package(CURL REQUIRED)
endif(${WIN32})
include(CheckCurlImportTargets)

if(NOT CURL_FOUND)
    message(FATAL_ERROR "Could not find library: curl.")
endif()


include(AddExternalProject)

# ---------------------------------------------- Build library --------------------------------------------------

include(GenerateExportHeader)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${JOYNR_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${JOYNR_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${JOYNR_BINARY_DIR})

######## Build and include settings ########
link_directories(
    ${LINK_DIRECTORIES}
)

######## SET LOG LEVEL #########
# JOYNR_MAX_LOG_LEVEL_FATAL
# JOYNR_MAX_LOG_LEVEL_ERROR
# JOYNR_MAX_LOG_LEVEL_WARN
# JOYNR_MAX_LOG_LEVEL_INFO
# JOYNR_MAX_LOG_LEVEL_DEBUG
# JOYNR_MAX_LOG_LEVEL_TRACE

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
    set_property(
                 CACHE
                 ${name}
                 PROPERTY STRINGS
                 FATAL
                 ERROR
                 WARN
                 INFO
                 DEBUG
                 TRACE
    )
endmacro(CREATE_LOG_LEVEL_PROPERTY)

CREATE_LOG_LEVEL_PROPERTY(JOYNR_MAX_LOG_LEVEL)
CREATE_LOG_LEVEL_PROPERTY(JOYNR_DEFAULT_RUNTIME_LOG_LEVEL)

add_definitions("-DJOYNR_MAX_LOG_LEVEL_${JOYNR_MAX_LOG_LEVEL}")
add_definitions("-DJOYNR_DEFAULT_RUNTIME_LOG_LEVEL_${JOYNR_DEFAULT_RUNTIME_LOG_LEVEL}")

if(JOYNR_ENABLE_STDOUT_LOGGING)
    add_definitions(-DJOYNR_ENABLE_STDOUT_LOGGING)
endif(JOYNR_ENABLE_STDOUT_LOGGING)

if(JOYNR_ENABLE_DLT_LOGGING)
    add_definitions(-DJOYNR_ENABLE_DLT_LOGGING)
endif(JOYNR_ENABLE_DLT_LOGGING)

if(JOYNR_DISABLE_DEBUG_TO_INFO_LOGGING)
    add_definitions(-DJOYNR_DISABLE_DEBUG_TO_INFO_LOGGING)
endif(JOYNR_DISABLE_DEBUG_TO_INFO_LOGGING)

if(EXISTS "/etc/fedora-release")
    message(STATUS "Fedora release detected, eNULL cipher not available - disabling.")
    add_definitions(-DJOYNR_WS_TLS_DISABLE_UNENCRYPTED_TRAFFIC)
endif()

if(ENABLE_MQTT_OCSP)
    add_definitions(-DMQTT_OCSP_ENABLED)
endif(ENABLE_MQTT_OCSP)

######## DEFAULT COMPILER FLAGS #############

include(SetCppStandard)

#Other Warnings that could be turned on:
#-Wpedantic
#-Wpadded
#-Wswitch-default

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # Mac OS X
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,dynamic_lookup")
endif()

message(STATUS "CMAKE_GENERATOR=" ${CMAKE_GENERATOR})
message(STATUS "compiler CMAKE_C_COMPILER=" ${CMAKE_C_COMPILER})
message(STATUS "compiler CMAKE_C_COMPILER_ID=" ${CMAKE_C_COMPILER_ID})
message(STATUS "compiler CMAKE_C_COMPILER_VERSION=" ${CMAKE_C_COMPILER_VERSION})
message(STATUS "compiler CMAKE_CXX_COMPILER=" ${CMAKE_CXX_COMPILER})
message(STATUS "compiler CMAKE_CXX_COMPILER_ID=" ${CMAKE_CXX_COMPILER_ID})
message(STATUS "compiler CMAKE_CXX_COMPILER_VERSION=" ${CMAKE_CXX_COMPILER_VERSION})
# Compiler flags that depend on compiler type and OS
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MSVCC_AUTO_INCLUDE} /MP")

        # When compiling with DEBUG turn on runtime error checks (RTC1) and
        # Buffer Overflow detection (GS)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${MSVCC_AUTO_INCLUDE} /RTC1 /GS /MP")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(GNU_C_FLAGS " \
        -pipe \
        -fstack-protector-strong \
        -Wall \
        -Wextra \
        -fwrapv \
        -Wformat=2 \
        -Wformat-signedness \
        -Wmissing-include-dirs \
        -Wmissing-braces \
        -Wpacked \
        -Wunused-parameter \
        -Wuninitialized \
        -Wfloat-equal \
        -Wshadow \
        -Wcast-qual \
        -Wlogical-op \
        -Wformat-security \
        -Wcast-align \
        -Wredundant-decls \
        -Wdeclaration-after-statement \
        -Wold-style-definition \
        -Wmissing-prototypes \
        -Wnested-externs \
        -Werror=all \
        -Werror=extra \
        -Werror=format \
        -Werror=format-signedness \
        -Werror=missing-include-dirs \
        -Werror=unused-parameter \
        -Werror=uninitialized \
        -Werror=float-equal \
        -Werror=cast-qual \
        -Werror=logical-op \
        -Werror=format-security \
        -Werror=cast-align \
        -Werror=redundant-decls \
        ")
    if(CMAKE_C_COMPILER_VERSION VERSION_GREATER "5.4")
        # deactivate sign conversion since it causes trouble in gtest/gmock
        set(GNU_C_FLAGS " \
            ${GNU_C_FLAGS} \
            -Wimplicit-fallthrough=5 \
            -Walloca \
            -Wduplicated-branches \
            -Wduplicated-cond \
            -Walloc-zero \
            -Wsign-conversion \
            -Werror=implicit-fallthrough \
            -Werror=alloca \
            -Werror=duplicated-cond \
            -Werror=alloc-zero \
            -Werror=sign-conversion\
            ")
    endif(CMAKE_C_COMPILER_VERSION VERSION_GREATER "5.4")
    if(CMAKE_C_COMPILER_VERSION VERSION_GREATER "9.1")
        set(GNU_C_FLAGS "${GNU_C_FLAGS} -Werror=duplicated-branches")
    endif(CMAKE_C_COMPILER_VERSION VERSION_GREATER "9.1")

    if(${WIN32})
        # This setting does not work in linux:
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GNU_C_FLAGS} -enable-auto-import")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GNU_C_FLAGS} -pthread")
    endif(${WIN32})
    # check GNU C compiler version
    if(CMAKE_C_COMPILER_VERSION VERSION_GREATER "4.9" OR CMAKE_C_COMPILER_VERSION VERSION_EQUAL "4.9")
        add_definitions(-D_GLIBC_USE_NANOSLEEP)
    else()
        message(FATAL_ERROR "a gcc compiler with a version higher than 4.9 is needed.")
    endif(CMAKE_C_COMPILER_VERSION VERSION_GREATER "4.9" OR CMAKE_C_COMPILER_VERSION VERSION_EQUAL "4.9")
endif(CMAKE_C_COMPILER_ID STREQUAL "GNU")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(GNU_CXX_FLAGS " \
        -pipe \
        -Wunused-value \
        -Wunused \
        -Wpointer-arith \
        -Wformat-y2k \
        -Wno-format-extra-args \
        -Wformat-nonliteral \
        -Winit-self \
        -Winvalid-pch \
        -Wstack-protector \
        -Wswitch-enum \
        -Wfatal-errors \
        -D_GLIBCXX_ASSERTIONS \
        -fstack-protector-strong \
        -Wall \
        -Wextra \
        -fwrapv \
        -Wformat=2 \
        -Wformat-signedness \
        -Wmissing-include-dirs \
        -Wmissing-braces \
        -Wpacked \
        -Wunused-parameter \
        -Wuninitialized \
        -Wfloat-equal \
        -Wshadow \
        -Wcast-qual \
        -Wlogical-op \
        -Wformat-security \
        -Wcast-align \
        -Wredundant-decls \
        -Werror=all \
        -Werror=extra \
        -Werror=format \
        -Werror=format-signedness \
        -Werror=missing-include-dirs \
        -Werror=unused-parameter \
        -Werror=uninitialized \
        -Werror=float-equal \
        -Werror=cast-qual \
        -Werror=logical-op \
        -Werror=format-security \
        -Werror=cast-align \
        -Werror=redundant-decls \
        ")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "5.4")
        set(GNU_CXX_FLAGS " \
            ${GNU_CXX_FLAGS} \
            -Wimplicit-fallthrough=5 \
            -Walloca \
            -Wduplicated-branches \
            -Wduplicated-cond \
            -Walloc-zero \
            -Wsign-conversion \
            -Werror=implicit-fallthrough \
            -Werror=alloca \
            -Werror=duplicated-cond \
            -Werror=alloc-zero \
            -Werror=sign-conversion\
            ")
    endif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "5.4")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "9.1")
        set(GNU_CXX_FLAGS "${GNU_CXX_FLAGS} -Werror=duplicated-branches")
    endif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "9.1")

    if(${WIN32})
            # This setting does not work in linux:
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GNU_CXX_FLAGS} -enable-auto-import")
    else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GNU_CXX_FLAGS} -pthread")
    endif(${WIN32})
    # check GNU CXX compiler version
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.9" OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "4.9")
        add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
    else()
        message(FATAL_ERROR "a gcc compiler with a version higher than 4.9 is needed.")
    endif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.9" OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "4.9")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        option(
            USE_STDLIB_LIBCPP
            "use libc++"
            OFF
        )
        if(USE_STDLIB_LIBCPP)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        endif()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

    add_definitions(-DGTEST_USE_OWN_TR1_TUPLE=1)
endif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

### Add spdlog ###########################################################
find_package(Threads) # sets CMAKE_THREAD_LIBS_INIT

set(JOYNR_SPDLOG_REQUIRED_VERSION 1.4.2)
string(CONCAT JOYNR_SPDLOG_GIT_TAG "v" ${JOYNR_SPDLOG_REQUIRED_VERSION})
if (USE_PLATFORM_SPDLOG)
    find_package(spdlog ${JOYNR_SPDLOG_REQUIRED_VERSION} REQUIRED)
    get_target_property(spdlog_INCLUDE_DIRS spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
else(USE_PLATFORM_SPDLOG)
    message(STATUS "############ Download: spdlog ############")

    AddExternalProject(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog
        GIT_TAG ${JOYNR_SPDLOG_GIT_TAG}
        TIMEOUT 10
        # Disable configure step
        CONFIGURE_COMMAND ""
        # Disable build step
        BUILD_COMMAND ""
    )

     ExternalProject_Get_Property(spdlog SOURCE_DIR)
     set(spdlog_INCLUDE_DIRS "${SOURCE_DIR}/include")
endif(USE_PLATFORM_SPDLOG)

message(STATUS "variable spdlog_INCLUDE_DIRS=" ${spdlog_INCLUDE_DIRS})

### Add DLT ###########################################################
if(JOYNR_ENABLE_DLT_LOGGING)
    if (USE_PLATFORM_DLT)
        include(FindPkgConfig)
        pkg_check_modules(DLT REQUIRED automotive-dlt)
        set(DLT_LIBRARIES "${DLT_LIBRARY_DIRS}/${CMAKE_SHARED_LIBRARY_PREFIX}dlt${CMAKE_SHARED_LIBRARY_SUFFIX}")
    else(USE_PLATFORM_DLT)
        message(STATUS "############ Download: DLT ############")
        get_property(EP_PREFIX DIRECTORY PROPERTY EP_PREFIX)

        set(DLT_LIBRARY_NAME "${CMAKE_SHARED_LIBRARY_PREFIX}dlt${CMAKE_SHARED_LIBRARY_SUFFIX}")
        set(DLT_LIBRARIES "${EP_PREFIX}/src/DLT-build/src/lib/${DLT_LIBRARY_NAME}")

        AddExternalProject(
            DLT
            GIT_REPOSITORY http://git.projects.genivi.org/dlt-daemon.git
            GIT_TAG v2.15.0
            TIMEOUT 10
            CMAKE_ARGS "-DWITH_MAN=OFF;"
                       "-DWTIH_DLT_ADAPTOR=OFF;" # sic!
                       "-DWITH_DLT_CONSOLE=OFF"
                       "-DWITH_DLT_EXAMPLES=OFF"
                       "-DWITH_DLT_SYSTEM=OFF"
                       "-DWITH_DLT_TESTS=OFF"
            BUILD_BYPRODUCTS ${DLT_LIBRARIES}
        )

        ExternalProject_Get_Property(DLT SOURCE_DIR)
        set(DLT_INCLUDE_DIRS "${SOURCE_DIR}/include")

        # The directory does not exist when the import targets are checked. As a workaround
        # we need to create it here. Otherwise the initial cmake call will fail.
        file(MAKE_DIRECTORY ${EP_PREFIX}/src/DLT-build/src/lib)
        file(MAKE_DIRECTORY ${EP_PREFIX}/src/DLT/include)
    endif(USE_PLATFORM_DLT)

    include(CheckDltImportTargets)
endif(JOYNR_ENABLE_DLT_LOGGING)

### Add muesli ############################################################
set(JOYNR_MUESLI_REQUIRED_VERSION 1.0.2)
if (USE_PLATFORM_MUESLI)
    find_package(muesli ${JOYNR_MUESLI_REQUIRED_VERSION} REQUIRED CONFIG)
else(USE_PLATFORM_MUESLI)
    message(STATUS "############ Download: muesli ${JOYNR_MUESLI_REQUIRED_VERSION} ############")

    AddExternalProject(
        muesli
        GIT_REPOSITORY https://github.com/bmwcarit/muesli
        GIT_TAG ${JOYNR_MUESLI_REQUIRED_VERSION}
        TIMEOUT 10
        # Disable configure step
        #CONFIGURE_COMMAND ""
        CMAKE_ARGS "-DBUILD_MUESLI_TESTS=Off;"
                   "-DENABLE_CLANG_FORMATTER=${ENABLE_CLANG_FORMATTER}"
        # Disable build step
        #BUILD_COMMAND ""
    )

    ExternalProject_Get_Property(muesli SOURCE_DIR)
    ExternalProject_Get_Property(muesli BINARY_DIR)

    # add import target for muesli library
    add_library(muesli::muesli INTERFACE IMPORTED GLOBAL)
    add_dependencies(muesli::muesli muesli)

    # muesli fetches rapidjson via AddExternalProject,
    # hence rapidjson lies in muesli's build directory
    set(
        muesli_INCLUDE_DIRECTORIES
        "${SOURCE_DIR}/include"
        "${BINARY_DIR}/ThirdParty/src/rapidjson/include"
    )

    # need to create include dirs manually at cmake time to prevent cmake warning (non existing path
    # in INTERFACE_INCLUDE_DIRECTORIES), since otherwise they are created at make time when cloning
    # the source repositories
    file(MAKE_DIRECTORY ${muesli_INCLUDE_DIRECTORIES})
    set_property(
        TARGET muesli::muesli
        APPEND
        PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${muesli_INCLUDE_DIRECTORIES}
    )
endif(USE_PLATFORM_MUESLI)
get_target_property(muesli_INCLUDE_DIRECTORIES muesli::muesli INTERFACE_INCLUDE_DIRECTORIES)

message(STATUS "variable muesli_INCLUDE_DIRECTORIES= ${muesli_INCLUDE_DIRECTORIES}")


### Add mococrw ############################################################
set(JOYNR_MOCOCRW_REQUIRED_VERSION 0.1.0)
if (USE_PLATFORM_MOCOCRW)
    find_package(MoCOCrW ${JOYNR_MOCOCRW_REQUIRED_VERSION} REQUIRED CONFIG)
else(USE_PLATFORM_MOCOCRW)
    message(STATUS "############ Download: MoCOCrW ${JOYNR_MOCOCRW_REQUIRED_VERSION} ############")
    get_property(EP_PREFIX DIRECTORY PROPERTY EP_PREFIX)
    set(MOCOCRW_LIBRARY_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}mococrw${CMAKE_SHARED_LIBRARY_SUFFIX})

    AddExternalProject(
        MoCOCrW
        GIT_REPOSITORY https://github.com/bmwcarit/MoCOCrW
        GIT_TAG a74df3b # master
        TIMEOUT 10
        CMAKE_ARGS "-DBUILD_TESTING=Off"
    )

    ExternalProject_Get_Property(MoCOCrW SOURCE_DIR)
    # The directory does not exist when the import targets are checked. As a workaround
    # we need to create it here. Otherwise the initial cmake call will fail.
    set(MOCOCRW_INCLUDE_DIRS "${SOURCE_DIR}/src")
    file(MAKE_DIRECTORY ${MOCOCRW_INCLUDE_DIRS})

    ExternalProject_Get_Property(MoCOCrW BINARY_DIR)
    set(MOCOCRW_BINARY_DIR ${BINARY_DIR}/src)
    set(MOCOCRW_LIBRARIES "${MOCOCRW_BINARY_DIR}/${MOCOCRW_LIBRARY_NAME}")

endif(USE_PLATFORM_MOCOCRW)
include(CheckMOCOCRWImportTargets)

### Add smrf ############################################################
set(JOYNR_SMRF_REQUIRED_VERSION 0.3.4)
if (USE_PLATFORM_SMRF)
    find_package(smrf ${JOYNR_SMRF_REQUIRED_VERSION} REQUIRED CONFIG)
else(USE_PLATFORM_SMRF)
    message(STATUS "############ Download: smrf ${JOYNR_SMRF_REQUIRED_VERSION} ############")

    get_property(EP_PREFIX DIRECTORY PROPERTY EP_PREFIX)
    set(SMRF_LIBRARY_NAME "${CMAKE_SHARED_LIBRARY_PREFIX}smrf${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(SMRF_LIBRARIES "${EP_PREFIX}/src/smrf-build/bin/${SMRF_LIBRARY_NAME}")

    AddExternalProject(
        smrf
        GIT_REPOSITORY https://github.com/bmwcarit/smrf
        GIT_TAG ${JOYNR_SMRF_REQUIRED_VERSION}
        TIMEOUT 10
        # Disable configure step
        #CONFIGURE_COMMAND ""
        CMAKE_ARGS "-DENABLE_CLANG_FORMATTER=${ENABLE_CLANG_FORMATTER};"
                   "-DBUILD_TESTS=Off"
        # Disable build step
        BUILD_BYPRODUCTS ${SMRF_LIBRARIES}
    )

    ExternalProject_Get_Property(smrf SOURCE_DIR)
    set(SMRF_INCLUDE_DIRS "${SOURCE_DIR}/cpp/libsmrf/include")

    # The directory does not exist when the import targets are checked. As a workaround
    # we need to create it here. Otherwise the initial cmake call will fail.
    file(MAKE_DIRECTORY ${EP_PREFIX}/src/smrf-build/src/lib)
    file(MAKE_DIRECTORY ${EP_PREFIX}/src/smrf/cpp/libsmrf/include)
endif(USE_PLATFORM_SMRF)
include(CheckSMRFImportTargets)

######## Add submodules ########

add_subdirectory(libjoynr)
add_subdirectory(libjoynrclustercontroller)
add_subdirectory(runtimes)

if(${BUILD_CLUSTER_CONTROLLER})
    add_subdirectory(cluster-controller)
endif(${BUILD_CLUSTER_CONTROLLER})

if(${BUILD_TESTS})
    add_subdirectory(tests)
endif(${BUILD_TESTS})

######## Install targets ########

set(
    TARGET_LIBS
    Joynr
    JoynrWsRuntime
    JoynrClusterController
    JoynrClusterControllerRuntime
)

# export all targets from the build tree to use by outside projects
export(
    TARGETS
    ${TARGET_LIBS}
    FILE "${JOYNR_BINARY_DIR}/JoynrTargets.cmake"
)

# store the current build dir in the CMake user package registry
# find_package may use it while searching for Joynr
export(PACKAGE Joynr)

# install the export targets for use with the install-tree
install(
    EXPORT JoynrTargets
    DESTINATION "${INSTALL_CMAKE_DIR}"
    FILE "JoynrTargets.cmake"
)

# Create the JoynrConfig.cmake and JoynrConfigVersion files
set(
    JoynrConfig_LIB_COMMON_LIBRARIES
    Joynr
    ${CMAKE_THREAD_LIBS_INIT}
)

get_target_property(Joynr_PUBLIC_INCLUDE_DIRECTORY Joynr INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY JoynrClusterController PUBLIC_INCLUDE_DIRECTORY)
get_target_property(JoynrClusterControllerRuntime_PUBLIC_INCLUDE_DIRECTORY JoynrClusterControllerRuntime PUBLIC_INCLUDE_DIRECTORY)

set(
    JoynrConfig_LIB_INPROCESS_INCLUDE_DIRS
    ${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}
    ${JoynrClusterControllerRuntime_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    JoynrConfig_LIB_INPROCESS_LIBRARIES
    JoynrClusterController
    JoynrClusterControllerRuntime
)

set(
    JoynrConfig_LIB_WS_LIBRARIES
    JoynrWsRuntime
)

# ... for the build tree

set(
    JoynrConfig_LIB_COMMON_INCLUDE_DIRS
    ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
    ${spdlog_INCLUDE_DIRS}
    ${muesli_INCLUDE_DIRECTORIES}
    ${runtime_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    JoynrConfig_LIB_WS_INCLUDE_DIRS
    ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
    ${spdlog_INCLUDE_DIRS}
    ${muesli_INCLUDE_DIRECTORIES}
)

set(JoynrConfig_TARGETS_FILE "${JOYNR_BINARY_DIR}/JoynrTargets.cmake")
set(JoynrConfig_INSTALL_BIN_DIR "${JOYNR_BINARY_DIR}")
set(JoynrConfig_RESOURCES_DIR "${JoynrConfig_INSTALL_BIN_DIR}/resources")

get_target_property(JoynrConfig_MOSQUITTO_INCLUDE_DIRS mosquitto::mosquitto INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(JoynrConfig_MOSQUITTO_LIBRARIES mosquitto::mosquitto IMPORTED_LOCATION)
get_target_property(JoynrConfig_SMRF_INCLUDE_DIRS smrf::smrf INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(JoynrConfig_SMRF_LIBRARIES smrf::smrf IMPORTED_LOCATION)
get_target_property(JoynrConfig_MOCOCRW_INCLUDE_DIRS MoCOCrW::mococrw INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(JoynrConfig_MOCOCRW_LIBRARIES MoCOCrW::mococrw IMPORTED_LOCATION)


set(JoynrConfig_RESOLVE_DEPENDENCIES)

### Add Config files to project so that can be shown in the editor
file(GLOB_RECURSE ADD_ALL_CMAKE_FILE_TO_PROJECT
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "CMake/*"
)
add_custom_target(NOOP_SHOW_CMAKE_FILES SOURCES ${ADD_ALL_CMAKE_FILE_TO_PROJECT})

macro(FillJoynrConfigDependencies DEPENDENCY_FILE)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/CMake/JoynrConfig/${DEPENDENCY_FILE}.cmake.in" FILE_CONTENTS)
    string(CONFIGURE ${FILE_CONTENTS} FILE_CONTENTS @ONLY)
    set(JoynrConfig_RESOLVE_DEPENDENCIES ${JoynrConfig_RESOLVE_DEPENDENCIES}${FILE_CONTENTS})
endmacro(FillJoynrConfigDependencies)

if(USE_PLATFORM_MUESLI)
    FillJoynrConfigDependencies("PlatformMuesli")
endif(USE_PLATFORM_MUESLI)

if(USE_PLATFORM_MOSQUITTO)
    FillJoynrConfigDependencies("PlatformMosquitto")
else(USE_PLATFORM_MOSQUITTO)
    FillJoynrConfigDependencies("ExternalProjectMosquitto")
endif(USE_PLATFORM_MOSQUITTO)

if(USE_PLATFORM_SPDLOG)
    FillJoynrConfigDependencies("PlatformSpdlog")
endif(USE_PLATFORM_SPDLOG)

if(USE_PLATFORM_SMRF)
    FillJoynrConfigDependencies("PlatformSmrf")
else(USE_PLATFORM_SMRF)
    FillJoynrConfigDependencies("ExternalProjectSmrf")
endif(USE_PLATFORM_SMRF)

if(USE_PLATFORM_MOCOCRW)
    FillJoynrConfigDependencies("PlatformMococrw")
else(USE_PLATFORM_MOCOCRW)
    FillJoynrConfigDependencies("ExternalProjectMococrw")
endif(USE_PLATFORM_MOCOCRW)

if(JOYNR_ENABLE_DLT_LOGGING)
if(USE_PLATFORM_DLT)
    FillJoynrConfigDependencies("PlatformDltlog")
else(USE_PLATFORM_DLT)
    FillJoynrConfigDependencies("ExternalProjectDltlog")
endif(USE_PLATFORM_DLT)
endif(JOYNR_ENABLE_DLT_LOGGING)

configure_file(
    JoynrConfig.cmake.in
    "JoynrConfig.cmake" @ONLY
)
file(
    COPY
        "CMake/CheckBoostImportTargets.cmake"
        "CMake/CheckCurlImportTargets.cmake"
        "CMake/CheckOpenSSLImportTargets.cmake"
        "CMake/CheckMosquittoImportTargets.cmake"
        "CMake/CheckDltImportTargets.cmake"
        "CMake/CheckSMRFImportTargets.cmake"
        "CMake/CheckMOCOCRWImportTargets.cmake"
    DESTINATION "${CMAKE_BINARY_DIR}"
)
# ... for the install tree
file(
    RELATIVE_PATH RELATIVE_INSTALL_INCLUDE_DIR
    "${CMAKE_INSTALL_PREFIX}/${INSTALL_CMAKE_DIR}"
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}"
)
file(
    RELATIVE_PATH RELATIVE_INSTALL_FULL_SYSCONFDIR
    "${CMAKE_INSTALL_PREFIX}/${INSTALL_CMAKE_DIR}"
    "${JOYNR_INSTALL_FULL_SYSCONFDIR}"
)
set(JoynrConfig_LIB_COMMON_INCLUDE_DIRS "\${CMAKE_CURRENT_LIST_DIR}/${RELATIVE_INSTALL_INCLUDE_DIR}")
set(JoynrConfig_LIB_INPROCESS_INCLUDE_DIRS "\${CMAKE_CURRENT_LIST_DIR}/${RELATIVE_INSTALL_INCLUDE_DIR}")
set(JoynrConfig_LIB_WS_INCLUDE_DIRS "\${CMAKE_CURRENT_LIST_DIR}/${RELATIVE_INSTALL_INCLUDE_DIR}")
set(JoynrConfig_TARGETS_FILE "\${CMAKE_CURRENT_LIST_DIR}/JoynrTargets.cmake")
set(JoynrConfig_RESOURCES_DIR "\${CMAKE_CURRENT_LIST_DIR}/${RELATIVE_INSTALL_FULL_SYSCONFDIR}")

file(
    RELATIVE_PATH RELATIVE_INSTALL_BIN_DIR
    "${CMAKE_INSTALL_PREFIX}/${INSTALL_CMAKE_DIR}"
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
)
set(JoynrConfig_INSTALL_BIN_DIR "\${CMAKE_CURRENT_LIST_DIR}/${RELATIVE_INSTALL_BIN_DIR}")
configure_file(
    JoynrConfig.cmake.in
    "JoynrConfig.cmake.install-tree" @ONLY
)
# ... for both
configure_file(
    JoynrConfigVersion.cmake.in
    "JoynrConfigVersion.cmake" @ONLY
)

# install the JoynrConfig.cmake and JoynrConfigVersion.cmake
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/JoynrConfig.cmake.install-tree"
    DESTINATION "${INSTALL_CMAKE_DIR}"
    RENAME "JoynrConfig.cmake"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/JoynrConfigVersion.cmake"
        "CMake/CheckBoostImportTargets.cmake"
        "CMake/CheckCurlImportTargets.cmake"
        "CMake/CheckOpenSSLImportTargets.cmake"
        "CMake/CheckMosquittoImportTargets.cmake"
        "CMake/CheckDltImportTargets.cmake"
        "CMake/CheckSMRFImportTargets.cmake"
        "CMake/CheckMOCOCRWImportTargets.cmake"
    DESTINATION "${INSTALL_CMAKE_DIR}"
)

# install the NOTICE-CPP
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/NOTICE-CPP"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)
