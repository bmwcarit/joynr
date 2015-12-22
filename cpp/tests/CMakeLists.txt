message(STATUS "#############################################")
message(STATUS "Configuring tests...")
message(STATUS "#############################################\n")

if(${WIN32})
    set(DISABLE_PTHREAD_CMAKE_ARG ON)
else()
endif(${WIN32})


if (USE_PLATFORM_GTEST_GMOCK)

    find_package(GTest REQUIRED)
    find_package(GMock REQUIRED)

else (USE_PLATFORM_GTEST_GMOCK)

    ### clone googletest git repository ###########################################################

    # this repository contains both googletest and googlemock
    AddExternalProject(
        googletest-repo
        DOWNLOAD_DIR "googletest-repo"
        TIMEOUT 10
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG ddb8012e # there is no version tag on github; this is to make sure that a working version is used
        CONFIGURE_COMMAND "" # Disable configuring
        BUILD_COMMAND "" # Disable building
    )

    get_property(EP_PREFIX DIRECTORY PROPERTY EP_PREFIX)

    ### Add google test ###########################################################

    # build googletest library
    AddExternalProject(
        googletest
        DOWNLOAD_COMMAND "" # already downloaded by googletest-repo
        SOURCE_DIR "${EP_PREFIX}/src/googletest-repo/googletest"
        BUILD_BYPRODUCTS "${EP_PREFIX}/src/googletest-build/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
        CMAKE_ARGS -Dgtest_force_shared_crt=ON
                   -Dgtest_disable_pthreads=${DISABLE_PTHREAD_CMAKE_ARG}
    )

    ExternalProject_Get_Property(googletest binary_dir)
    set(googletest_binary_dir ${binary_dir})
    message(STATUS "variable googletest_binary_dir=${googletest_binary_dir}")

    # build googletest AFTER downloading the sources
    add_dependencies(googletest googletest-repo)

    # Specify include dir
    ExternalProject_Get_Property(googletest source_dir)
    set(googletest_source_dir ${source_dir})
    message(STATUS "variable googletest_source_dir=${googletest_source_dir}")
    set(GTEST_INCLUDE_DIRS ${googletest_source_dir}/include)
    set(GTEST_LIBRARIES ${googletest_binary_dir}/libgtest.a)

    ### Add google mock ###########################################################

    # build googlemock library
    AddExternalProject(
        googlemock
        DOWNLOAD_COMMAND "" # already downloaded by googletest-repo
        SOURCE_DIR "${EP_PREFIX}/src/googletest-repo/googlemock"
        BUILD_BYPRODUCTS "${EP_PREFIX}/src/googlemock-build/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX}"
        CMAKE_ARGS -Dgtest_force_shared_crt=ON
                   -Dgtest_disable_pthreads=${DISABLE_PTHREAD_CMAKE_ARG}
    )

    ExternalProject_Get_Property(googlemock binary_dir)
    set(googlemock_binary_dir ${binary_dir})
    message(STATUS "variable googlemock_binary_dir=${googlemock_binary_dir}")

    # build googlemock AFTER downloading the sources
    add_dependencies(googlemock googletest-repo)

    # Specify include dir
    ExternalProject_Get_Property(googlemock source_dir)
    set(googlemock_source_dir ${source_dir})
    message(STATUS "variable googlemock_source_dir=${googlemock_source_dir}")
    set(GMOCK_INCLUDE_DIRS ${googlemock_source_dir}/include)
    set(GMOCK_LIBRARIES ${googlemock_binary_dir}/libgmock.a)

endif(USE_PLATFORM_GTEST_GMOCK)

include_directories(SYSTEM ${GTEST_INCLUDE_DIRS})
include_directories(SYSTEM ${GMOCK_INCLUDE_DIRS})

### copy test resources and settings ##########################################
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    configure_file("resources/integrationtest.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/integrationtest.settings")
    configure_file("resources/sslintegrationtest.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/sslintegrationtest.settings")
    configure_file("resources/SystemIntegrationTest1.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/SystemIntegrationTest1.settings")
    configure_file("resources/SystemIntegrationTest2.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/SystemIntegrationTest2.settings")
    configure_file("resources/libjoynrSystemIntegration1.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/libjoynrSystemIntegration1.settings")
    configure_file("resources/libjoynrSystemIntegration2.settings" "${JOYNR_BINARY_DIR}/Debug/test-resources/libjoynrSystemIntegration2.settings")
    configure_file("resources/ca.crt" "${JOYNR_BINARY_DIR}/Debug/test-resources/ca.crt")
    configure_file("resources/testclient.pem" "${JOYNR_BINARY_DIR}/Debug/test-resources/testclient.pem")
    configure_file("resources/integrationtest.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/integrationtest.settings")
    configure_file("resources/sslintegrationtest.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/sslintegrationtest.settings")
    configure_file("resources/SystemIntegrationTest1.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/SystemIntegrationTest1.settings")
    configure_file("resources/SystemIntegrationTest2.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/SystemIntegrationTest2.settings")
    configure_file("resources/libjoynrSystemIntegration1.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/libjoynrSystemIntegration1.settings")
    configure_file("resources/libjoynrSystemIntegration2.settings" "${JOYNR_BINARY_DIR}/Release/test-resources/libjoynrSystemIntegration2.settings")
    configure_file("resources/ca.crt" "${JOYNR_BINARY_DIR}/Release/test-resources/ca.crt")
    configure_file("resources/testclient.pem" "${JOYNR_BINARY_DIR}/Release/test-resources/testclient.pem")
else()
    configure_file("resources/integrationtest.settings" "${JOYNR_BINARY_DIR}/test-resources/integrationtest.settings")
    configure_file("resources/sslintegrationtest.settings" "${JOYNR_BINARY_DIR}/test-resources/sslintegrationtest.settings")
    configure_file("resources/SystemIntegrationTest1.settings" "${JOYNR_BINARY_DIR}/test-resources/SystemIntegrationTest1.settings")
    configure_file("resources/SystemIntegrationTest2.settings" "${JOYNR_BINARY_DIR}/test-resources/SystemIntegrationTest2.settings")
    configure_file("resources/libjoynrSystemIntegration1.settings" "${JOYNR_BINARY_DIR}/test-resources/libjoynrSystemIntegration1.settings")
    configure_file("resources/libjoynrSystemIntegration2.settings" "${JOYNR_BINARY_DIR}/test-resources/libjoynrSystemIntegration2.settings")
    configure_file("resources/ca.crt" "${JOYNR_BINARY_DIR}/test-resources/ca.crt")
    configure_file("resources/testclient.pem" "${JOYNR_BINARY_DIR}/test-resources/testclient.pem")
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")


### set up stuff common for all test targets ##################################

include(AddTest.cmake)

get_target_property(JoynrCommon_PUBLIC_INCLUDE_DIRECTORY JoynrCommon PUBLIC_INCLUDE_DIRECTORY)
get_target_property(Joynr_PUBLIC_INCLUDE_DIRECTORY Joynr PUBLIC_INCLUDE_DIRECTORY)
get_target_property(jsonserializer_PUBLIC_INCLUDE_DIRECTORY jsonserializer PUBLIC_INCLUDE_DIRECTORY)
get_target_property(jsmn_PUBLIC_INCLUDE_DIRECTORY jsmn PUBLIC_INCLUDE_DIRECTORY)

get_target_property(joynrwsruntime_PUBLIC_INCLUDE_DIRECTORY JoynrWsRuntime PUBLIC_INCLUDE_DIRECTORY)
if(${BUILD_CLUSTER_CONTROLLER})
    get_target_property(JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY JoynrClusterController PUBLIC_INCLUDE_DIRECTORY)
    get_target_property(JoynrClusterControllerRuntime_PUBLIC_INCLUDE_DIRECTORY JoynrClusterControllerRuntime PUBLIC_INCLUDE_DIRECTORY)
endif(${BUILD_CLUSTER_CONTROLLER})

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
    ${libwebsockets_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_SOURCE_DIR}/gen/include"
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
    ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
    ${spdlog_SOURCE_DIR}/include
    ${jsonserializer_PUBLIC_INCLUDE_DIRECTORY}
    ${jsmn_PUBLIC_INCLUDE_DIRECTORY}
    ${joynrwsruntime_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    test_TARGET_LIBRARIES
    JoynrCommon
    Joynr
    jsonserializer
)

if(${USE_DBUS_COMMONAPI_COMMUNICATION})
    set(
        OPTIONAL_DBUS_LIBRARIES 
        JoynrDbusRuntime
    )
    get_target_property(common_api_PUBLIC_INCLUDE_DIRECTORY JoynrCommonAPI PUBLIC_INCLUDE_DIRECTORY)
    list(
        APPEND JoynrTest_INCLUDE_DIRECTORIES
        ${common_api_PUBLIC_INCLUDE_DIRECTORY}
    )
else()
    set(OPTIONAL_DBUS_LIBRARIES "")
endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

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
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
    )
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")

set(
    cluster_controller_test_INCLUDE_DIRECTORIES
    ${JoynrTest_INCLUDE_DIRECTORIES}
    ${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}
    ${JoynrClusterControllerRuntime_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    cluster_controller_test_TARGET_LIBRARIES
    ${test_TARGET_LIBRARIES}
    JoynrClusterController
    JoynrClusterControllerRuntime
    # Link the Dbus runtime last so that the cluster controller
    # runtime is the default runtime
    ${OPTIONAL_DBUS_LIBRARIES}
)

set(
    libjoynr_test_TARGET_LIBRARIES
    ${test_TARGET_LIBRARIES}
    JoynrWsRuntime
)

set(
    cluster_controller_test_HEADERS
    "PrettyPrint.h"
    "utils/MockObjects.h"
    "utils/LibJoynrMockObjects.h"
    "utils/MockLocalCapabilitiesDirectoryCallback.h"
    ${JoynrTest_GENHEADERS}
)

file(
    GLOB_RECURSE JsonSerailizerTest_GENSOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    #"gen/*.cpp" uncomment this when generate-test-sources prepared for jsonserializer
    "unit-tests/jsonserializer/generated/*.cpp" #remove this when generate-test-sources prepared for jsonserializer
)

file(
    GLOB_RECURSE JsonSerailizerTest_GENHEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    #"gen/*.h" uncomment this when generate-test-sources prepared for jsonserializer
    "unit-tests/jsonserializer/generated/*.h" #remove this when generate-test-sources prepared for jsonserializer
)

set(
    libjsonserializer_test_HEADERS
    "unit-tests/jsonserializer/ExampleTypes.h"
    ${JsonSerailizerTest_GENHEADERS}
)

set(
    libjsonserializer_test_SOURCES
    "unit-tests/jsonserializer/ExampleTypes.cpp"
    ${JsonSerailizerTest_GENSOURCES}
)

set(
    libjoynr_test_HEADERS
    "PrettyPrint.h"
    "utils/LibJoynrMockObjects.h"
    ${JoynrTest_GENHEADERS}
)

set(
    cluster_controller_test_SOURCES
    "PrettyPrint.cpp"
    "utils/MockObjects.cpp"
    "utils/MockLocalCapabilitiesDirectoryCallback.cpp"
    "TestMain.cpp"
    ${JoynrTest_GENSOURCES}
)

set(
    libjoynr_test_SOURCES
    "PrettyPrint.cpp"
    "TestMain.cpp"
    ${JoynrTest_GENSOURCES}
)

if(${BUILD_CLUSTER_CONTROLLER})
    ###############
    # g_UnitTests #
    ###############

    file(
        GLOB g_UnitTests_SOURCES
        "unit-tests/*.cpp"
        "unit-tests/websocket/*.cpp"
        "unit-tests/jsonserializer/*.cpp"
    )

    if(${USE_DBUS_COMMONAPI_COMMUNICATION})
        list(
            APPEND g_UnitTests_SOURCES
            "unit-tests/dbus/DbusSettingsTest.cpp"
        )
    endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

    AddTest(
        g_UnitTests
        ${cluster_controller_test_HEADERS}
        ${cluster_controller_test_SOURCES}
        ${libjsonserializer_test_HEADERS}
        ${libjsonserializer_test_SOURCES}
        ${g_UnitTests_SOURCES}
    )

    target_link_libraries(
        g_UnitTests
        Qt5::Test
        ${cluster_controller_test_TARGET_LIBRARIES}
    )

    target_include_directories(
        g_UnitTests
        SYSTEM
        PRIVATE ${GTEST_INCLUDE_DIRS}
        PRIVATE ${GMOCK_INCLUDE_DIRS}
        PRIVATE ${cluster_controller_test_INCLUDE_DIRECTORIES}
    )

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

    AddTest(
       g_IntegrationTests
       ${cluster_controller_test_HEADERS}
       ${cluster_controller_test_SOURCES}
       ${g_IntegrationTests_SOURCES}
    )

    target_link_libraries(g_IntegrationTests ${cluster_controller_test_TARGET_LIBRARIES})

    target_include_directories(
        g_IntegrationTests
        SYSTEM
        PRIVATE ${GTEST_INCLUDE_DIRS}
        PRIVATE ${GMOCK_INCLUDE_DIRS}
        PRIVATE ${cluster_controller_test_INCLUDE_DIRECTORIES}
    )

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

    AddTest(
       g_SystemIntegrationTests
       ${cluster_controller_test_HEADERS}
       ${cluster_controller_test_SOURCES}
       ${g_SystemIntegrationTests_SOURCES}
    )

    target_link_libraries(g_SystemIntegrationTests ${cluster_controller_test_TARGET_LIBRARIES})

    set_target_properties(
       g_SystemIntegrationTests
       PROPERTIES
       COMPILE_FLAGS "-Wno-effc++ -Wno-unused-parameter"
    )

    target_include_directories(
        g_SystemIntegrationTests
        SYSTEM
        PRIVATE ${GTEST_INCLUDE_DIRS}
        PRIVATE ${GMOCK_INCLUDE_DIRS}
        PRIVATE ${cluster_controller_test_INCLUDE_DIRECTORIES}
    )

    install(TARGETS g_UnitTests g_IntegrationTests g_SystemIntegrationTests
        DESTINATION ${INSTALL_TEST_DIR} COMPONENT tests
    )

else()
    # Build tests that run on libjoynr

    ###############
    # g_UnitTests #
    ###############

    set(
        g_UnitTests_SOURCES
        "unit-tests/JsonSerializerTest.cpp"
    )

    AddTest(
        g_UnitTests
        ${libjoynr_test_HEADERS}
        ${libjoynr_test_SOURCES}
        ${g_UnitTests_SOURCES}
    )

    target_link_libraries(
        g_UnitTests
        Qt5::Test
        ${libjoynr_test_TARGET_LIBRARIES}
    )

    target_include_directories(
        g_UnitTests
        SYSTEM
        PRIVATE ${GTEST_INCLUDE_DIRS}
        PRIVATE ${GMOCK_INCLUDE_DIRS}
        PRIVATE ${JoynrTest_INCLUDE_DIRECTORIES}
    )

    ############################
    # g_SystemIntegrationTests #
    ############################

    set(
       g_SystemIntegrationTests_SOURCES
       "systemintegration-tests/TestConfiguration.h"
       "systemintegration-tests/CombinedEnd2EndTest.h"
       "systemintegration-tests/CombinedEnd2EndTest.cpp"
       "systemintegration-tests/TestConfiguration.cpp"
    )

    AddTest(
       g_SystemIntegrationTests
       ${libjoynr_test_HEADERS}
       ${libjoynr_test_SOURCES}
       ${g_SystemIntegrationTests_SOURCES}
    )

    target_link_libraries(g_SystemIntegrationTests ${libjoynr_test_TARGET_LIBRARIES})

    target_include_directories(
        g_SystemIntegrationTests
        SYSTEM
        PRIVATE ${GTEST_INCLUDE_DIRS}
        PRIVATE ${GMOCK_INCLUDE_DIRS}
        PRIVATE ${JoynrTest_INCLUDE_DIRECTORIES}
    )

    install(TARGETS g_UnitTests g_SystemIntegrationTests
        DESTINATION ${INSTALL_TEST_DIR} COMPONENT tests
    )

endif(${BUILD_CLUSTER_CONTROLLER})

install(DIRECTORY resources
        DESTINATION ${INSTALL_TEST_DIR} COMPONENT tests
)
