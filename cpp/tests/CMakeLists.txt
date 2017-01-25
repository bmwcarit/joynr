message(STATUS "#############################################")
message(STATUS "Configuring tests...")
message(STATUS "#############################################\n")

option(
    BUILD_UNIT_TESTS
    "Build unit tests?"
    ON
)

option(
    BUILD_INTEGRATION_TESTS
    "Build integration tests?"
    ON
)

option(
    BUILD_SYSTEM_INTEGRATION_TESTS
    "Build system integration tests?"
    ON
)

if(${WIN32})
    set(DISABLE_PTHREAD_CMAKE_ARG ON)
else()
endif(${WIN32})

include(AddGtestGmock)

### copy test resources and settings ##########################################
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    configure_file(
        "resources/integrationtest.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/integrationtest.settings"
        @ONLY
    )
    configure_file(
        "resources/sslintegrationtest.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/sslintegrationtest.settings"
        @ONLY
    )
    configure_file(
        "resources/HttpSystemIntegrationTest1.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/HttpSystemIntegrationTest1.settings"
        @ONLY
    )
    configure_file(
        "resources/HttpSystemIntegrationTest2.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/HttpSystemIntegrationTest2.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendSystemIntegrationTest1.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/MqttWithHttpBackendSystemIntegrationTest1.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendSystemIntegrationTest2.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/MqttWithHttpBackendSystemIntegrationTest2.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendJoynrClusterControllerRuntimeTest.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/MqttWithHttpBackendJoynrClusterControllerRuntimeTest.settings"
        @ONLY
    )
    configure_file(
        "resources/HttpMessagingSettingsTest.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/HttpMessagingSettingsTest.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttMessagingSettingsTest.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/MqttMessagingSettingsTest.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendMessagingSettingsTest.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/MqttWithHttpBackendMessagingSettingsTest.settings"
        @ONLY
    )
    configure_file(
        "resources/libjoynrSystemIntegration1.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/libjoynrSystemIntegration1.settings"
        @ONLY
    )
    configure_file(
        "resources/libjoynrSystemIntegration2.settings"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/libjoynrSystemIntegration2.settings"
        @ONLY
    )
    configure_file(
        "resources/ca.crt"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/ca.crt"
        @ONLY
    )
    configure_file(
        "resources/testclient.pem"
        "${JOYNR_BINARY_DIR}/Debug/test-resources/testclient.pem"
        @ONLY
    )
    configure_file(
        "resources/integrationtest.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/integrationtest.settings"
        @ONLY
    )
    configure_file(
        "resources/sslintegrationtest.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/sslintegrationtest.settings"
        @ONLY
    )
    configure_file(
        "resources/HttpSystemIntegrationTest1.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/HttpSystemIntegrationTest1.settings"
        @ONLY
    )
    configure_file(
        "resources/HttpSystemIntegrationTest2.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/HttpSystemIntegrationTest2.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendSystemIntegrationTest1.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/MqttWithHttpBackendSystemIntegrationTest1.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendSystemIntegrationTest2.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/MqttWithHttpBackendSystemIntegrationTest2.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendJoynrClusterControllerRuntimeTest.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/MqttWithHttpBackendJoynrClusterControllerRuntimeTest.settings"
        @ONLY
    )
    configure_file(
        "resources/HttpMessagingSettingsTest.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/HttpMessagingSettingsTest.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttMessagingSettingsTest.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/MqttMessagingSettingsTest.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendMessagingSettingsTest.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/MqttWithHttpBackendMessagingSettingsTest.settings"
        @ONLY
    )
    configure_file(
        "resources/libjoynrSystemIntegration1.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/libjoynrSystemIntegration1.settings"
        @ONLY
    )
    configure_file(
        "resources/libjoynrSystemIntegration2.settings"
        "${JOYNR_BINARY_DIR}/Release/test-resources/libjoynrSystemIntegration2.settings"
        @ONLY
    )
    configure_file(
        "resources/ca.crt"
        "${JOYNR_BINARY_DIR}/Release/test-resources/ca.crt"
        @ONLY
    )
    configure_file(
        "resources/testclient.pem"
        "${JOYNR_BINARY_DIR}/Release/test-resources/testclient.pem"
        @ONLY
    )
else()
    configure_file(
        "resources/integrationtest.settings"
        "${JOYNR_BINARY_DIR}/test-resources/integrationtest.settings"
        @ONLY
    )
    configure_file(
        "resources/sslintegrationtest.settings"
        "${JOYNR_BINARY_DIR}/test-resources/sslintegrationtest.settings"
        @ONLY
    )
    configure_file(
        "resources/HttpSystemIntegrationTest1.settings"
        "${JOYNR_BINARY_DIR}/test-resources/HttpSystemIntegrationTest1.settings"
        @ONLY
    )
    configure_file(
        "resources/HttpSystemIntegrationTest2.settings"
        "${JOYNR_BINARY_DIR}/test-resources/HttpSystemIntegrationTest2.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendSystemIntegrationTest1.settings"
        "${JOYNR_BINARY_DIR}/test-resources/MqttWithHttpBackendSystemIntegrationTest1.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendSystemIntegrationTest2.settings"
        "${JOYNR_BINARY_DIR}/test-resources/MqttWithHttpBackendSystemIntegrationTest2.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendJoynrClusterControllerRuntimeTest.settings"
        "${JOYNR_BINARY_DIR}/test-resources/MqttWithHttpBackendJoynrClusterControllerRuntimeTest.settings"
        @ONLY
    )
    configure_file(
        "resources/HttpMessagingSettingsTest.settings"
        "${JOYNR_BINARY_DIR}/test-resources/HttpMessagingSettingsTest.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttMessagingSettingsTest.settings"
        "${JOYNR_BINARY_DIR}/test-resources/MqttMessagingSettingsTest.settings"
        @ONLY
    )
    configure_file(
        "resources/MqttWithHttpBackendMessagingSettingsTest.settings"
        "${JOYNR_BINARY_DIR}/test-resources/MqttWithHttpBackendMessagingSettingsTest.settings"
        @ONLY
    )
    configure_file(
        "resources/libjoynrSystemIntegration1.settings"
        "${JOYNR_BINARY_DIR}/test-resources/libjoynrSystemIntegration1.settings"
        @ONLY
    )
    configure_file(
        "resources/libjoynrSystemIntegration2.settings"
        "${JOYNR_BINARY_DIR}/test-resources/libjoynrSystemIntegration2.settings"
        @ONLY
    )
    configure_file(
        "resources/ca.crt"
        "${JOYNR_BINARY_DIR}/test-resources/ca.crt"
        @ONLY
    )
    configure_file(
        "resources/testclient.pem"
        "${JOYNR_BINARY_DIR}/test-resources/testclient.pem"
        @ONLY
    )
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")

file(COPY "resources/ListOfCapabilitiesToInject.json"
     DESTINATION "${JOYNR_BINARY_DIR}/test-resources/")

### set up stuff common for all test targets ##################################

if(${GENERATE_SOURCES})
    JoynrGenerator(
        OUTPUTPATH "${CMAKE_CURRENT_SOURCE_DIR}/gen"
        MODELPATH "${CMAKE_CURRENT_SOURCE_DIR}/../../basemodel/src/test/franca"
        GENERATION_LANGUAGE "cpp"
    )
endif(${GENERATE_SOURCES})

get_target_property(JoynrCommon_PUBLIC_INCLUDE_DIRECTORY JoynrCommon PUBLIC_INCLUDE_DIRECTORY)
get_target_property(Joynr_PUBLIC_INCLUDE_DIRECTORY Joynr PUBLIC_INCLUDE_DIRECTORY)

get_target_property(joynrwsruntime_PUBLIC_INCLUDE_DIRECTORY JoynrWsRuntime PUBLIC_INCLUDE_DIRECTORY)
if(${BUILD_CLUSTER_CONTROLLER})
    get_target_property(JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY JoynrClusterController PUBLIC_INCLUDE_DIRECTORY)
    get_target_property(JoynrClusterControllerRuntime_PUBLIC_INCLUDE_DIRECTORY JoynrClusterControllerRuntime PUBLIC_INCLUDE_DIRECTORY)
endif(${BUILD_CLUSTER_CONTROLLER})

set(
    test_TARGET_LIBRARIES
    JoynrCommon
    Joynr
)

set(
    JoynrTest_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_SOURCE_DIR}/gen/include"
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
    ${Joynr_PUBLIC_INCLUDE_DIRECTORY}
    ${joynrwsruntime_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    JoynrTest_SYSTEM_INCLUDE_DIRECTORIES
    ${spdlog_INCLUDE_DIRS}
)

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

add_library(JoynrTestGenerated SHARED
    ${JoynrTest_GENSOURCES}
    ${JoynrTest_GENHEADERS}
)

# add needed dependencies by linking to target joynr
target_link_libraries(JoynrTestGenerated Joynr)

if(NOT USE_PLATFORM_SPDLOG)
    add_dependencies(JoynrTestGenerated spdlog)
endif(NOT USE_PLATFORM_SPDLOG)

target_include_directories(
    JoynrTestGenerated
    PUBLIC ${JoynrTest_INCLUDE_DIRECTORIES}
)

target_include_directories(
    JoynrTestGenerated
    SYSTEM PUBLIC
    ${JoynrTest_SYSTEM_INCLUDE_DIRECTORIES}
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
    "JoynrTest.h"
    "PrettyPrint.h"
    "utils/MockCallback.h"
    "utils/MockObjects.h"
    "utils/LibJoynrMockObjects.h"
    "utils/MockLocalCapabilitiesDirectoryCallback.h"
    ${JoynrTest_GENHEADERS}
)

set(
    libjoynr_test_HEADERS
    "PrettyPrint.h"
    "utils/TimeUtils.h"
    "utils/TestRunnable.h"
    "utils/LibJoynrMockObjects.h"
    ${JoynrTest_GENHEADERS}
)

set(
    cluster_controller_test_SOURCES
    "PrettyPrint.cpp"
    "utils/MockObjects.cpp"
    "utils/TestRunnable.cpp"
    "utils/MockLocalCapabilitiesDirectoryCallback.cpp"
    "TestMain.cpp"
)

set(
    libjoynr_test_SOURCES
    "PrettyPrint.cpp"
    "TestMain.cpp"
)

if(${BUILD_CLUSTER_CONTROLLER})
    ###############
    # g_UnitTests #
    ###############

    file(
        GLOB g_UnitTests_SOURCES
        "unit-tests/*.cpp"
        "unit-tests/mqtt/*.cpp"
        "unit-tests/websocket/*.cpp"
        "unit-tests/jsonserializer/*.cpp"
        "unit-tests/serializer/*.cpp"
    )

    if(${USE_DBUS_COMMONAPI_COMMUNICATION})
        list(
            APPEND g_UnitTests_SOURCES
            "unit-tests/dbus/DbusSettingsTest.cpp"
        )
    endif(${USE_DBUS_COMMONAPI_COMMUNICATION})

    if(${BUILD_UNIT_TESTS})
        AddTest(
            g_UnitTests
            ${cluster_controller_test_HEADERS}
            ${cluster_controller_test_SOURCES}
            ${g_UnitTests_SOURCES}
        )

        target_link_libraries(
            g_UnitTests
            JoynrTestGenerated
            ${cluster_controller_test_TARGET_LIBRARIES}
        )

        target_include_directories(
            g_UnitTests
            PRIVATE ${cluster_controller_test_INCLUDE_DIRECTORIES}
        )

        install(TARGETS g_UnitTests
            DESTINATION ${INSTALL_TEST_DIR}
        )
    endif(${BUILD_UNIT_TESTS})

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

    if(${BUILD_INTEGRATION_TESTS})
        AddTest(
           g_IntegrationTests
           ${cluster_controller_test_HEADERS}
           ${cluster_controller_test_SOURCES}
           ${g_IntegrationTests_SOURCES}
        )

        target_link_libraries(
            g_IntegrationTests
            JoynrTestGenerated
            ${cluster_controller_test_TARGET_LIBRARIES}
        )

        target_include_directories(
            g_IntegrationTests
            PRIVATE ${cluster_controller_test_INCLUDE_DIRECTORIES}
        )

        install(TARGETS g_IntegrationTests
            DESTINATION ${INSTALL_TEST_DIR}
        )
    endif(${BUILD_INTEGRATION_TESTS})

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

    if(${BUILD_SYSTEM_INTEGRATION_TESTS})
        AddTest(
           g_SystemIntegrationTests
           ${cluster_controller_test_HEADERS}
           ${cluster_controller_test_SOURCES}
           ${g_SystemIntegrationTests_SOURCES}
        )

        target_link_libraries(
            g_SystemIntegrationTests
            JoynrTestGenerated
            ${cluster_controller_test_TARGET_LIBRARIES}
        )

        set_target_properties(
           g_SystemIntegrationTests
           PROPERTIES
           COMPILE_FLAGS "-Wno-effc++ -Wno-unused-parameter"
        )

        target_include_directories(
            g_SystemIntegrationTests
            SYSTEM
            PRIVATE ${CURL_INCLUDE_DIRS}
        )

        target_include_directories(
            g_SystemIntegrationTests
            PRIVATE ${cluster_controller_test_INCLUDE_DIRECTORIES}
        )

        install(TARGETS g_SystemIntegrationTests
            DESTINATION ${INSTALL_TEST_DIR}
        )
    endif(${BUILD_SYSTEM_INTEGRATION_TESTS})

else()
    # Build tests that run on libjoynr

    ###############
    # g_UnitTests #
    ###############

    set(
        g_UnitTests_SOURCES
        "unit-tests/JsonSerializerTest.cpp"
    )

    if(${BUILD_UNIT_TESTS})
        AddTest(
            g_UnitTests
            ${libjoynr_test_HEADERS}
            ${libjoynr_test_SOURCES}
            ${g_UnitTests_SOURCES}
        )

    target_link_libraries(
            g_UnitTests
            JoynrTestGenerated
            ${libjoynr_test_TARGET_LIBRARIES}
        )

    target_include_directories(
            g_UnitTests
            PRIVATE ${JoynrTest_INCLUDE_DIRECTORIES}
        )

    install(TARGETS g_UnitTests
            DESTINATION ${INSTALL_TEST_DIR}
        )
    endif(${BUILD_UNIT_TESTS})

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

    if(${BUILD_SYSTEM_INTEGRATION_TESTS})
        AddTest(
           g_SystemIntegrationTests
           ${libjoynr_test_HEADERS}
           ${libjoynr_test_SOURCES}
           ${g_SystemIntegrationTests_SOURCES}
        )

        target_link_libraries(
            g_SystemIntegrationTests
            JoynrTestGenerated
            ${libjoynr_test_TARGET_LIBRARIES}
        )

        target_include_directories(
            g_SystemIntegrationTests
            PRIVATE ${JoynrTest_INCLUDE_DIRECTORIES}
        )

        install(TARGETS g_SystemIntegrationTests
            DESTINATION ${INSTALL_TEST_DIR}
        )
    endif(${BUILD_SYSTEM_INTEGRATION_TESTS})

endif(${BUILD_CLUSTER_CONTROLLER})

install(DIRECTORY resources
        DESTINATION ${INSTALL_TEST_DIR}
)
