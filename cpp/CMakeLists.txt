# Parts of this file are inspired by a CMake tutorial (http://www.cmake.org/Wiki/CMake/)
# that is available under Creative Commons Attribution 2.5 Generic (CC BY 2.5, http://creativecommons.org/licenses/by/2.5/).
# There are no attributions specified by the author.

cmake_minimum_required(VERSION 2.8.11)

######## Project settings ########
project(joynr-0.17.2)
set(JOYNR_MAJOR_VERSION 0)
set(JOYNR_MINOR_VERSION 17)
set(JOYNR_PATCH_VERSION 2)
set(
    JOYNR_VERSION
    ${JOYNR_MAJOR_VERSION}.${JOYNR_MINOR_VERSION}.${JOYNR_PATCH_VERSION}
)

# Policy for Qt linking on Windows
if(POLICY CMP0020)
    cmake_policy(SET CMP0020 OLD)
endif(POLICY CMP0020)

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
    USE_PLATFORM_DEPENDENCIES
    "Resolve all dependencies (link libraries) from the system?"
    OFF
)
message(STATUS "option USE_PLATFORM_DEPENDENCIES=" ${USE_PLATFORM_DEPENDENCIES})

if(USE_PLATFORM_DEPENDENCIES)
    set(USE_PLATFORM_SPDLOG ON)
    set(USE_PLATFORM_JSMN ON)
    set(USE_PLATFORM_MOSQUITTO ON)
    set(USE_PLATFORM_GTEST_GMOCK ON)
    set(USE_PLATFORM_WEBSOCKETPP ON)
endif(USE_PLATFORM_DEPENDENCIES)

option(
    USE_PLATFORM_SPDLOG
    "Resolve dependency to spdlog from the system?"
    OFF
)
message(STATUS "option USE_PLATFORM_SPDLOG=" ${USE_PLATFORM_SPDLOG})

option(
    USE_PLATFORM_JSMN
    "Resolve dependency to jsmn from the system?"
    OFF
)
message(STATUS "option USE_PLATFORM_JSMN=" ${USE_PLATFORM_JSMN})

option(
    USE_PLATFORM_MOSQUITTO
    "Resolve dependency to mosquitto from the system?"
    OFF
)
message(STATUS "option USE_PLATFORM_MOSQUITTO=" ${USE_PLATFORM_MOSQUITTO})

option(
    USE_PLATFORM_GTEST_GMOCK
    "Resolve dependency to gtest and gmock from the system?"
    OFF
)
message(STATUS "option USE_PLATFORM_GTEST_GMOCK=" ${USE_PLATFORM_GTEST_GMOCK})

option(
    USE_PLATFORM_WEBSOCKETPP
    "Resolve dependency to WebSocket++ from the system?"
    OFF
)
message(STATUS "option USE_PLATFORM_WEBSOCKETPP=" ${USE_PLATFORM_WEBSOCKETPP})

# should we use DBUS to communicate between libjoynr and cluster-controller
option(
    USE_DBUS_COMMONAPI_COMMUNICATION
    "Use DBUS and CommonAPI to communicate between libjoynr and cluster-controller?"
    OFF
)
message(STATUS "option USE_DBUS_COMMONAPI_COMMUNICATION=" ${USE_DBUS_COMMONAPI_COMMUNICATION})

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
    BUILD_CLUSTER_CONTROLLER
    "Build ClusterController?"
    ON
)
message(STATUS "option BUILD_CLUSTER_CONTROLLER=" ${BUILD_CLUSTER_CONTROLLER})

include(LocateProgram)
include(AddClangFormat)

#### BEGIN: Prevent install-target execution if platform dependencies are not used ####

# Following dependencies must be installed by the user beforehand because they either
# use a shared object or are included through joynr header files into the user's code.
# Mosquitto is only required when the cluster controller build is enabled.
if(NOT (USE_PLATFORM_SPDLOG AND USE_PLATFORM_JSMN AND (USE_PLATFORM_MOSQUITTO OR NOT BUILD_CLUSTER_CONTROLLER)))
    set(codeMsg "message(FATAL_ERROR \"Install target is not available as long as external platform")
    set(codeMsg "${codeMsg} dependencies are used. For more information see USE_PLATFORM_SPDLOG,")
    set(codeMsg "${codeMsg} USE_PLATFORM_JSMN and USE_PLATFORM_MOSQUITTO.\")")
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
        cp -r bin/resources .

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

message(STATUS "############ Import: Qt5 ############")

# Find includes in corresponding build directories (Qt MOCs are created in current build dir)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core)
find_package(Qt5WebSockets)
find_package(Qt5Sql)

# Qt-Websockets and Qt-Core is only needed for the Cluster-Controller

IF(${Qt5Core_FOUND} AND ${Qt5WebSockets_FOUND})

    message(STATUS "variable Qt5Core_VERSION_STRING=${Qt5Core_VERSION_STRING}")
    message(STATUS "variable Qt5Core_INCLUDE_DIRS=${Qt5Core_INCLUDE_DIRS}")
    get_target_property(Qt5Core_LOCATION Qt5::Core LOCATION)
    message(STATUS "variable Qt5::Core LOCATION=${Qt5Core_LOCATION}")
    message(STATUS "variable Qt5Core_LIBRARIES=${Qt5Core_LIBRARIES}")
    message(STATUS "variable Qt5Core_DEFINITIONS=${Qt5Core_DEFINITIONS}")

    message(STATUS "variable Qt5WebSockets_VERSION_STRING=${Qt5WebSockets_VERSION_STRING}")
    message(STATUS "variable Qt5WebSockets_INCLUDE_DIRS=${Qt5WebSockets_INCLUDE_DIRS}")
    get_target_property(Qt5WebSockets_LOCATION Qt5::WebSockets LOCATION)
    message(STATUS "variable Qt5::WebSockets LOCATION=${Qt5WebSockets_LOCATION}")
    message(STATUS "variable Qt5WebSockets_LIBRARIES=${Qt5WebSockets_LIBRARIES}")
    message(STATUS "variable Qt5WebSockets_DEFINITIONS=${Qt5WebSockets_DEFINITIONS}")

    message(STATUS "variable Qt5Sql_VERSION_STRING=${Qt5Sql_VERSION_STRING}")
    message(STATUS "variable Qt5Sql_INCLUDE_DIRS=${Qt5Sql_INCLUDE_DIRS}")
    get_target_property(Qt5Sql_LOCATION Qt5::Sql LOCATION)
    message(STATUS "variable Qt5Sql LOCATION=${Qt5Sql_LOCATION}")
    message(STATUS "variable Qt5Sql_LIBRARIES=${Qt5Sql_LIBRARIES}")
    message(STATUS "variable Qt5Sql_DEFINITIONS=${Qt5Sql_DEFINITIONS}")

ELSE(${Qt5Core_FOUND} AND ${Qt5WebSockets_FOUND})

    SET(BUILD_CLUSTER_CONTROLLER OFF)

ENDIF(${Qt5Core_FOUND} AND ${Qt5WebSockets_FOUND})

message(STATUS "option BUILD_CLUSTER_CONTROLLER=" ${BUILD_CLUSTER_CONTROLLER})

find_package(Qt5Test REQUIRED)
message(STATUS "variable Qt5Test_VERSION_STRING=${Qt5Test_VERSION_STRING}")
message(STATUS "variable Qt5Test_INCLUDE_DIRS=${Qt5Test_INCLUDE_DIRS}")
get_target_property(Qt5Test_LOCATION Qt5::Test LOCATION)
message(STATUS "variable Qt5::Test LOCATION=${Qt5Test_LOCATION}")
message(STATUS "variable Qt5Test_LIBRARIES=${Qt5Test_LIBRARIES}")
message(STATUS "variable Qt5Test_DEFINITIONS=${Qt5Test_DEFINITIONS}")

######## Add Boost ########

find_package(Boost 1.56.0 REQUIRED
    COMPONENTS
    system
    thread
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

# for every other build type than debug we limit log messages to level ERROR
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_TOLOWER)
if(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug)
    set(JOYNR_MAX_LOG_LEVEL "TRACE" CACHE STRING "joynr max log level")
else()
    set(JOYNR_MAX_LOG_LEVEL "ERROR" CACHE STRING "joynr max log level")
endif()

set_property(
             CACHE
             JOYNR_MAX_LOG_LEVEL
             PROPERTY STRINGS
             FATAL
             ERROR
             WARN
             INFO
             DEBUG
             TRACE
)

add_definitions("-DJOYNR_MAX_LOG_LEVEL_${JOYNR_MAX_LOG_LEVEL}")

######## DEFAULT COMPILER FLAGS #############

include(SetCppStandard)

#Other Warnings that could be turned on:
#-Wpedantic
#-Wpadded
#-Wswitch-default //but QT moc_files produce lots of warnings here.

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

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # common-api/CMakeLists.txt overwrites CMAKE_CXX_FLAGS to deactivate warnings for CommonAPI
    # generated code.
    # All changes to CMAKE_CXX_FLAGS that are not warnings should be added there as well.
    set(GNU_CXX_FLAGS "-pipe -g")
    set(GNU_CXX_FLAGS "${GNU_CXX_FLAGS} -Wall -Wunused-value -Wunused -Wcast-qual -Wpointer-arith")
    set(GNU_CXX_FLAGS "${GNU_CXX_FLAGS} -Wextra -Wcast-align -Wdisabled-optimization -Wformat-y2k")
    set(GNU_CXX_FLAGS "${GNU_CXX_FLAGS} -Wno-format-extra-args -Wformat-nonliteral -Wformat=2 -Winit-self")
    set(GNU_CXX_FLAGS "${GNU_CXX_FLAGS} -Winvalid-pch -Wunsafe-loop-optimizations -Wmissing-include-dirs")
    set(GNU_CXX_FLAGS "${GNU_CXX_FLAGS} -Wmissing-braces -Wpacked -Wredundant-decls -Wstack-protector")
    set(GNU_CXX_FLAGS "${GNU_CXX_FLAGS} -Wswitch-enum -Wuninitialized -Weffc++ -Wformat-security")
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

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    message(STATUS "### DBUS CommonAPI configuration ###")

    find_package(PkgConfig)
    pkg_check_modules(DBUS1 REQUIRED dbus-1)
    message(STATUS "variable DBUS_INCLUDE_DIRS=" ${DBUS1_INCLUDE_DIRS})
    message(STATUS "variable DBUS1_LIBRARIES=" ${DBUS1_LIBRARIES})
    message(STATUS "variable DBUS1_LIBRARY_DIRS=" ${DBUS1_LIBRARY_DIRS})
    pkg_check_modules(COMMONAPI REQUIRED CommonAPI=2.1.4)
    message(STATUS "variable COMMONAPI_INCLUDE_DIRS=" ${COMMONAPI_INCLUDE_DIRS})
    message(STATUS "variable COMMONAPI_LIBRARIES=" ${COMMONAPI_LIBRARIES})
    message(STATUS "variable COMMONAPI_VERSION=" ${COMMONAPI_VERSION})
    pkg_check_modules(COMMONAPI_DBUS REQUIRED CommonAPI-DBus=2.1.4)
    message(STATUS "variable COMMONAPI_DBUS_INCLUDE_DIRS=" ${COMMONAPI_DBUS_INCLUDE_DIRS})
    message(STATUS "variable COMMONAPI_DBUS_LIBRARIES=" ${COMMONAPI_DBUS_LIBRARIES})
    message(STATUS "variable COMMONAPI_DBUS_VERSION=" ${COMMONAPI_DBUS_VERSION})

    include_directories(
        SYSTEM
        ${COMMONAPI_INCLUDE_DIRS}
        ${DBUS1_INCLUDE_DIRS}
        ${COMMONAPI_DBUS_INCLUDE_DIRS}
    )

    link_directories(${DBUS1_LIBRARY_DIRS} ${COMMONAPI_LIBRARY_DIRS} ${COMMONAPI_DBUS_LIBRARY_DIRS})
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})


### Add spdlog ###########################################################
find_package(Threads) # sets CMAKE_THREAD_LIBS_INIT

if (USE_PLATFORM_SPDLOG)
    find_package(spdlog REQUIRED)
    get_target_property(spdlog_INCLUDE_DIRS spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
else(USE_PLATFORM_SPDLOG)
    message(STATUS "############ Download: spdlog ############")

    AddExternalProject(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog
        GIT_TAG e91e1b8 # there are no version tags, hence a SHA is used
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

######## Add submodules ########
if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    add_subdirectory(common-api)
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})
add_subdirectory(common)
add_subdirectory(jsonserializer)
add_subdirectory(libjoynr)
if(${BUILD_CLUSTER_CONTROLLER})
    add_subdirectory(cluster-controller)
else()
    # We still use the websocket default configuration file
    # copy resources for build tree
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cluster-controller/resources/default-websocket.settings ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-websocket.settings @ONLY)
    # ...and for install tree
    install(
        FILES
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/default-websocket.settings
        DESTINATION ${JOYNR_INSTALL_FULL_SYSCONFDIR}
    )
endif(${BUILD_CLUSTER_CONTROLLER})
add_subdirectory(runtimes)
if(${BUILD_TESTS})
    add_subdirectory(tests)
endif(${BUILD_TESTS})

######## Install targets ########

set(
    TARGET_LIBS
    JoynrCommon
    Joynr
    JoynrWsRuntime
    JoynrJsonSerializer
)

if(${BUILD_CLUSTER_CONTROLLER})
    LIST(
        APPEND
        TARGET_LIBS
        JoynrClusterController
        JoynrClusterControllerRuntime
    )
endif(${BUILD_CLUSTER_CONTROLLER})

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    list(
        APPEND TARGET_LIBS
        JoynrDbusRuntime
        JoynrCommonAPI
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

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
    DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev
    FILE "JoynrTargets.cmake"
)

# Create the JoynrConfig.cmake and JoynrConfigVersion files
set(
    JoynrConfig_LIB_COMMON_LIBRARIES
    JoynrCommon
    Joynr
    JoynrJsonSerializer
    ${CMAKE_THREAD_LIBS_INIT}
)

if(${BUILD_CLUSTER_CONTROLLER})
    set(
        JoynrConfig_LIB_INPROCESS_LIBRARIES
        JoynrClusterController
        JoynrClusterControllerRuntime
    )
endif(${BUILD_CLUSTER_CONTROLLER})

set(
    JoynrConfig_LIB_WS_LIBRARIES
    JoynrWsRuntime
)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    set(
        JoynrConfig_LIB_DBUS_LIBRARIES
        JoynrDbusRuntime
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

# ... for the build tree
get_target_property(JoynrCommon_PUBLIC_INCLUDE_DIRECTORY JoynrCommon PUBLIC_INCLUDE_DIRECTORY)
get_target_property(Joynr_PUBLIC_INCLUDE_DIRECTORY Joynr PUBLIC_INCLUDE_DIRECTORY)
get_target_property(JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY JoynrJsonSerializer PUBLIC_INCLUDE_DIRECTORY)
message(STATUS "variable JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY=" ${JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY})

set(
    JoynrConfig_LIB_COMMON_INCLUDE_DIRS
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
    ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
    ${spdlog_INCLUDE_DIRS}
    ${JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY}
    ${jsmn_INCLUDE_DIR}
    ${runtime_PUBLIC_INCLUDE_DIRECTORY}
)
if(${BUILD_CLUSTER_CONTROLLER})
    get_target_property(JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY JoynrClusterController PUBLIC_INCLUDE_DIRECTORY)
    get_target_property(JoynrClusterControllerRuntime_PUBLIC_INCLUDE_DIRECTORY JoynrClusterControllerRuntime PUBLIC_INCLUDE_DIRECTORY)
    set(
        JoynrConfig_LIB_INPROCESS_INCLUDE_DIRS
        ${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}
        ${JoynrClusterControllerRuntime_PUBLIC_INCLUDE_DIRECTORY}
    )
endif(${BUILD_CLUSTER_CONTROLLER})
set(
    JoynrConfig_LIB_WS_INCLUDE_DIRS
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
    ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
    ${spdlog_INCLUDE_DIRS}
    ${JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY}
    ${jsmn_INCLUDE_DIR}
)
if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    set(
        JoynrConfig_LIB_DBUS_INCLUDE_DIRS
        ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
        ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
        ${spdlog_INCLUDE_DIRS}
        ${JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY}
        ${jsmn_INCLUDE_DIR}
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})
set(JoynrConfig_TARGETS_FILE "${JOYNR_BINARY_DIR}/JoynrTargets.cmake")
set(JoynrConfig_INSTALL_BIN_DIR "${JOYNR_BINARY_DIR}")
set(JoynrConfig_RESOURCES_DIR "${JoynrConfig_INSTALL_BIN_DIR}/resources")

get_target_property(JoynrConfig_MOSQUITTO_INCLUDE_DIRS mosquitto::mosquitto INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(JoynrConfig_MOSQUITTO_LIBRARIES mosquitto::mosquitto IMPORTED_LOCATION)

set(JoynrConfig_RESOLVE_DEPENDENCIES)

macro(FillJoynrConfigDependencies DEPENDENCY_FILE)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/CMake/JoynrConfig/${DEPENDENCY_FILE}.cmake.in" FILE_CONTENTS)
    string(CONFIGURE ${FILE_CONTENTS} FILE_CONTENTS @ONLY)
    string(CONCAT JoynrConfig_RESOLVE_DEPENDENCIES ${JoynrConfig_RESOLVE_DEPENDENCIES} "${FILE_CONTENTS}")
endmacro(FillJoynrConfigDependencies)

if(USE_PLATFORM_JSMN)
    FillJoynrConfigDependencies("PlatformJsmn")
endif(USE_PLATFORM_JSMN)

if(USE_PLATFORM_MOSQUITTO)
    FillJoynrConfigDependencies("PlatformMosquitto")
else(USE_PLATFORM_MOSQUITTO)
    FillJoynrConfigDependencies("ExternalProjectMosquitto")
endif(USE_PLATFORM_MOSQUITTO)

if(USE_PLATFORM_SPDLOG)
    FillJoynrConfigDependencies("PlatformSpdlog")
endif(USE_PLATFORM_SPDLOG)

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
set(JoynrConfig_LIB_DBUS_INCLUDE_DIRS "\${CMAKE_CURRENT_LIST_DIR}/${RELATIVE_INSTALL_INCLUDE_DIR}")
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
    DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev
    RENAME "JoynrConfig.cmake"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/JoynrConfigVersion.cmake"
        "CMake/CheckBoostImportTargets.cmake"
        "CMake/CheckCurlImportTargets.cmake"
        "CMake/CheckOpenSSLImportTargets.cmake"
        "CMake/CheckMosquittoImportTargets.cmake"
    DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev
)

# install the NOTICE-CPP
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/NOTICE-CPP"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)

# cpack settings
include(InstallRequiredSystemLibraries)


# generic

#
# RPM specific settings
#
set(CPACK_GENERATOR "RPM")
set(CPACK_STRIP_FILES "ON")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Joynr base package")
set(CPACK_PACKAGE_NAME "joynr")
set(CPACK_PACKAGE_VERSION_MAJOR "${JOYNR_MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${JOYNR_MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${JOYNR_PATCH_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_VENDOR "BMW Car IT GmbH (http://www.bmw-carit.de)")
set(CPACK_PACKAGE_CONTACT "joynr@bmw-carit.de")
set(CPACK_RPM_PACKAGE_URL "https://github.com/bmwcarit/joynr")
set(CPACK_RPM_PACKAGE_LICENSE "Apache License, Version 2.0")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README-CPP.txt")


if(DEFINED ${BUILD_PACKAGE})
    message(STATUS "variable BUILD_PACKAGE=" ${BUILD_PACKAGE})
    if(${BUILD_PACKAGE} STREQUAL "shlib")
        message(STATUS "Building package for shlib")
        set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
        set(CPACK_RPM_COMPONENT_INSTALL 1)
        set(CPACK_COMPONENTS_ALL shlib)
    endif(${BUILD_PACKAGE} STREQUAL "shlib")

    if(${BUILD_PACKAGE} STREQUAL "shlib-debug")
        message(STATUS "Building package for shlib-debug")
        set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
        set(CPACK_RPM_COMPONENT_INSTALL 1)
        set(CPACK_COMPONENTS_ALL shlib)
        set (CPACK_RPM_SPEC_MORE_DEFINE "%debug_package")
    endif(${BUILD_PACKAGE} STREQUAL "shlib-debug")

    if(${BUILD_PACKAGE} STREQUAL "bin")
        message(STATUS "Building package for bin")
        set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
        set(CPACK_RPM_COMPONENT_INSTALL 1)
        set(CPACK_COMPONENTS_ALL bin)
    endif(${BUILD_PACKAGE} STREQUAL "bin")

    if(${BUILD_PACKAGE} STREQUAL "bin-debug")
        message(STATUS "Building package for bin-debug")
        set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
        set(CPACK_RPM_COMPONENT_INSTALL 1)
        set(CPACK_COMPONENTS_ALL bin)
        set (CPACK_RPM_SPEC_MORE_DEFINE "%debug_package")
    endif(${BUILD_PACKAGE} STREQUAL "bin-debug")

    if(${BUILD_PACKAGE} STREQUAL "dev")
        message(STATUS "Building package for dev")
        set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
        set(CPACK_RPM_COMPONENT_INSTALL 1)
        set(CPACK_COMPONENTS_ALL dev)
    endif(${BUILD_PACKAGE} STREQUAL "dev")

    if(${BUILD_PACKAGE} STREQUAL "tests")
        message(STATUS "Building package for tests")
        set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
        set(CPACK_RPM_COMPONENT_INSTALL 1)
        set(CPACK_COMPONENTS_ALL tests)
    endif(${BUILD_PACKAGE} STREQUAL "tests")

    if(${BUILD_PACKAGE} STREQUAL "library")
        message(STATUS "Building package for library")
        set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
        set(CPACK_RPM_COMPONENT_INSTALL 1)
        set(CPACK_COMPONENTS_ALL library)
    endif(${BUILD_PACKAGE} STREQUAL "library")
endif(DEFINED ${BUILD_PACKAGE})

include(CPack)
