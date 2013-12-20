message(STATUS "#############################################")
message(STATUS "Configuring tests...")
message(STATUS "#############################################\n")

# Enable ExternalProject CMake module
include(ExternalProject)
# Set default ExternalProject root directory
set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/ThirdParty)

if(${WIN32})
    set(DISABLE_PTHREAD_CMAKE_ARG ON)
else()
    #Disable some sets of warnings in test-directory.
    remove_definitions(-Weffc++)
    add_definitions(-Wno-unused-parameter)
    add_definitions(-Wno-reorder)
    set(DISABLE_PTHREAD_CMAKE_ARG OFF)
endif(${WIN32})


### Add google test ###########################################################
ExternalProject_Add(
    googletest
    SVN_REPOSITORY http://googletest.googlecode.com/svn/tags/release-1.6.0
    TIMEOUT 10
    # Force separate output paths for debug and release builds to allow easy
    # identification of correct lib in subsequent TARGET_LINK_LIBRARIES commands
    CMAKE_ARGS #-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
               #-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
               -DBUILD_SHARED_LIBS=OFF
               -Dgtest_force_shared_crt=ON
               -Dgtest_disable_pthreads=${DISABLE_PTHREAD_CMAKE_ARG}
    # Disable svn update
    UPDATE_COMMAND ""
    # Disable install step
    INSTALL_COMMAND ""
    # Wrap download, configure and build steps in a script to log output
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
)

# Specify include dir
ExternalProject_Get_Property(googletest source_dir)
set(googletest_source_dir ${source_dir})
message(STATUS "variable googletest_source_dir=${googletest_source_dir}")
include_directories(SYSTEM ${googletest_source_dir}/include)

ExternalProject_Get_Property(googletest binary_dir)
set(googletest_binary_dir ${binary_dir})
message(STATUS "variable googletest_binary_dir=${googletest_binary_dir}")

### Add google mock ###########################################################
ExternalProject_Add(
    googlemock
    SVN_REPOSITORY http://googlemock.googlecode.com/svn/tags/release-1.6.0
    TIMEOUT 10
    # Force separate output paths for debug and release builds to allow easy
    # identification of correct lib in subsequent TARGET_LINK_LIBRARIES commands
    CMAKE_ARGS #-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
               #-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
               -DBUILD_SHARED_LIBS=OFF
               -Dgtest_force_shared_crt=ON
               -Dgtest_disable_pthreads=${DISABLE_PTHREAD_CMAKE_ARG}
    # Disable svn update
    UPDATE_COMMAND ""
    # Disable install step
    INSTALL_COMMAND ""
    # Wrap download, configure and build steps in a script to log output
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
)

# Specify include dir
ExternalProject_Get_Property(googlemock source_dir)
set(googlemock_source_dir ${source_dir})
message(STATUS "variable googlemock_source_dir=${googlemock_source_dir}")
include_directories(SYSTEM ${googlemock_source_dir}/include)

ExternalProject_Get_Property(googlemock binary_dir)
set(googlemock_binary_dir ${binary_dir})
message(STATUS "variable googlemock_binary_dir=${googlemock_binary_dir}")

### copy test resources and settings ##########################################
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    configure_file("resources/integrationtest.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/integrationtest.settings")
    configure_file("resources/SystemIntegrationTest1.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/SystemIntegrationTest1.settings")
    configure_file("resources/SystemIntegrationTest2.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/SystemIntegrationTest2.settings")
    configure_file("resources/libjoynrintegrationtest.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/libjoynrintegrationtest.settings")
    configure_file("resources/libjoynrSystemIntegration1.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/libjoynrSystemIntegration1.settings")
    configure_file("resources/libjoynrSystemIntegration2.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/libjoynrSystemIntegration2.settings")
    configure_file("resources/integrationtest.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/integrationtest.settings")
    configure_file("resources/SystemIntegrationTest1.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/SystemIntegrationTest1.settings")
    configure_file("resources/SystemIntegrationTest2.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/SystemIntegrationTest2.settings")
    configure_file("resources/libjoynrintegrationtest.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/libjoynrintegrationtest.settings")
    configure_file("resources/libjoynrSystemIntegration1.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/libjoynrSystemIntegration1.settings")
    configure_file("resources/libjoynrSystemIntegration2.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/libjoynrSystemIntegration2.settings")
else()
    configure_file("resources/integrationtest.settings" "${JOYNR_BINARY_DIR}/test-resources/integrationtest.settings")
    configure_file("resources/SystemIntegrationTest1.settings" "${JOYNR_BINARY_DIR}/test-resources/SystemIntegrationTest1.settings")
    configure_file("resources/SystemIntegrationTest2.settings" "${JOYNR_BINARY_DIR}/test-resources/SystemIntegrationTest2.settings")
    configure_file("resources/libjoynrintegrationtest.settings" "${JOYNR_BINARY_DIR}/test-resources/libjoynrintegrationtest.settings")
    configure_file("resources/libjoynrSystemIntegration1.settings" "${JOYNR_BINARY_DIR}/test-resources/libjoynrSystemIntegration1.settings")
    configure_file("resources/libjoynrSystemIntegration2.settings" "${JOYNR_BINARY_DIR}/test-resources/libjoynrSystemIntegration2.settings")
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")


### set up stuff common for all test targets ##################################

get_target_property(JoynrCommon_PUBLIC_INCLUDE_DIRECTORY JoynrCommon PUBLIC_INCLUDE_DIRECTORY)
get_target_property(Joynr_PUBLIC_INCLUDE_DIRECTORY Joynr PUBLIC_INCLUDE_DIRECTORY)
get_target_property(JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY JoynrClusterController PUBLIC_INCLUDE_DIRECTORY)
get_target_property(JoynrClusterControllerRuntime_PUBLIC_INCLUDE_DIRECTORY JoynrClusterControllerRuntime PUBLIC_INCLUDE_DIRECTORY)
get_target_property(qjson_PUBLIC_INCLUDE_DIRECTORY qjson PUBLIC_INCLUDE_DIRECTORY)
get_target_property(log4qt_PUBLIC_INCLUDE_DIRECTORY log4qt PUBLIC_INCLUDE_DIRECTORY)

file(
    GLOB_RECURSE JoynrTest_GENSOURCES
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "gen/*.cpp"
)

file(
    GLOB_RECURSE JoynrTest_GENHEADERS
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "gen/*.h"
)

set(
    JoynrTest_INCLUDE_DIRECTORIES
    "./"
    "../"
	"gen/include"
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
    ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
    ${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}
    ${JoynrClusterControllerRuntime_PUBLIC_INCLUDE_DIRECTORY}
    ${qjson_PUBLIC_INCLUDE_DIRECTORY}
    ${log4qt_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    test_TARGET_LIBRARIES
    JoynrCommon
    Joynr
    JoynrClusterController
    JoynrClusterControllerRuntime
    qjson
    log4qt
)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    list(
        APPEND test_TARGET_LIBRARIES
        JoynrRuntime
    )
    get_target_property(common_api_PUBLIC_INCLUDE_DIRECTORY JoynrCommonAPI PUBLIC_INCLUDE_DIRECTORY)
    list(
        APPEND JoynrTest_INCLUDE_DIRECTORIES
        ${common_api_PUBLIC_INCLUDE_DIRECTORY}
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

include_directories(
    ${JoynrTest_INCLUDE_DIRECTORIES}
)

# Visual Studio uses different directories for RELEASE and DEBUG 
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    set(
        GMOCK_LINK_LIBRARY
        optimized ${googlemock_binary_dir}/Release/${CMAKE_FIND_LIBRARY_PREFIXES}gmock${CMAKE_FIND_LIBRARY_SUFFIXES}
        debug ${googlemock_binary_dir}/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}gmock${CMAKE_FIND_LIBRARY_SUFFIXES}
    )
    set(
        GTEST_LINK_LIBRARY
        optimized ${googletest_binary_dir}/Release/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${CMAKE_FIND_LIBRARY_SUFFIXES}
        debug ${googletest_binary_dir}/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${CMAKE_FIND_LIBRARY_SUFFIXES}
    )

    list(
        APPEND test_TARGET_LIBRARIES
        ${GTEST_LINK_LIBRARY}
        ${GMOCK_LINK_LIBRARY}
    )
else()
    list(
        APPEND test_TARGET_LIBRARIES
        ${googletest_binary_dir}/libgtest.a
        ${googlemock_binary_dir}/libgmock.a
    )
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")

set(
    test_HEADERS
    "PrettyPrint.h"
    "utils/MockObjects.h"
    "utils/TestQString.h"
    "utils/QThreadSleep.h"
    "utils/MockLocalCapabilitiesDirectoryCallback.h"
	${JoynrTest_GENHEADERS}
)

set(
    test_SOURCES
    "PrettyPrint.cpp"
    "utils/MockObjects.cpp"
    "utils/MockLocalCapabilitiesDirectoryCallback.cpp"
    "TestMain.cpp"
	${JoynrTest_GENSOURCES}
)

###############
# g_UnitTests #
###############

file(
    GLOB g_UnitTests_SOURCES
    "unit-tests/*.cpp"
)

add_executable(
    g_UnitTests
    ${test_HEADERS}
    ${test_SOURCES}
    ${g_UnitTests_SOURCES}
)
qt5_use_modules(g_UnitTests Core)
add_dependencies(g_UnitTests googletest)
add_dependencies(g_UnitTests googlemock)

target_link_libraries(g_UnitTests ${test_TARGET_LIBRARIES})

######################
# g_IntegrationTests #
######################

file(
    GLOB g_IntegrationTests_SOURCES
    "integration-tests/*.cpp"
)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    list(
        APPEND g_IntegrationTests_SOURCES
        "integration-tests/dbus/LibJoynrRuntimeTest.cpp"
        "integration-tests/dbus/LibJoynrDbusCommunicationTests.cpp"
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

add_executable(
    g_IntegrationTests
    ${test_HEADERS}
    ${test_SOURCES}
    ${g_IntegrationTests_SOURCES}
)
qt5_use_modules(g_IntegrationTests Core)
add_dependencies(g_IntegrationTests googletest)
add_dependencies(g_IntegrationTests googlemock)

target_link_libraries(g_IntegrationTests ${test_TARGET_LIBRARIES})

############################
# g_SystemIntegrationTests #
############################

file(
    GLOB g_SystemIntegrationTests_SOURCES
    "systemintegration-tests/*.cpp"
)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    list(
        APPEND g_SystemIntegrationTests_SOURCES
        "systemintegration-tests/dbus/End2EndDbusTest.cpp"
    )
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

add_executable(
    g_SystemIntegrationTests
    ${test_HEADERS}
    ${test_SOURCES}
    ${g_SystemIntegrationTests_SOURCES}
)
qt5_use_modules(g_SystemIntegrationTests Core)
add_dependencies(g_SystemIntegrationTests googletest)
add_dependencies(g_SystemIntegrationTests googlemock)

target_link_libraries(g_SystemIntegrationTests ${test_TARGET_LIBRARIES})
