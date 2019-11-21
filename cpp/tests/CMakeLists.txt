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
macro(CONFIGURE_TEST_FILES targetDirectory)
    set(TEST_FILES
        "AccessControlNoPermission.entries"
        "AccessControlYesPermission.entries"
        "AccessStoreTest.persist"
        "AclRclJoynrClusterControllerRuntimeTest.settings"
        "application1_ACL_RCL_Permissions.json"
        "application2_ACL_RCL_Permissions.json"
        "application3_ACL_RCL_Permissions.json"
        "application4_ACL_RCL_Permissions.json"
        "application5_ACL_RCL_Permissions.json"
        "integrationtest.settings"
        "websocket-cc-tls.settings"
        "websocket-libjoynr-tls.settings"
        "websocket-libjoynr-non-tls.settings"
        "CCSettingsWithAccessControlDisabled.settings"
        "CCSettingsWithAccessControlEnabled.settings"
        "CCSettingsWithAccessControlEnabledAndAclFilePathSet.settings"
        "CCSettingsWithGlobalDiscovery.settings"
        "CCSettingsWithMessageQueueLimit.settings"
        "HttpSystemIntegrationTest1.settings"
        "HttpSystemIntegrationTest2.settings"
        "HttpJoynrClusterControllerRuntimeTest.settings"
        "MqttSenderTestWithMaxMessageSizeLimits1.settings"
        "MqttSenderTestWithMaxMessageSizeLimits2.settings"
        "MqttSystemIntegrationTest1.settings"
        "MqttSystemIntegrationTest2.settings"
        "MqttOverTLSSystemIntegrationTest1.settings"
        "MqttOverTLSSystemIntegrationTest2.settings"
        "MqttJoynrClusterControllerRuntimeTest.settings"
        "MqttJoynrClusterControllerRuntimeTlsOnNoCertificatesTest.settings"
        "MqttMB_JoynrClusterControllerRuntimeTest.settings"
        "MqttMB_JoynrClusterControllerRuntimeEmptyGbid.settings"
        "MqttMB_JoynrClusterControllerRuntimeMisconfiguredTest.settings"
        "HttpMessagingSettingsTest.settings"
        "MqttMessagingSettingsTest.settings"
        "MqttMessagingSettingsWithGbidTest.settings"
        "MessagingSettingsDiscoveryEntryExpiryIntervalMs.settings"
        "MessagingSettingsDiscardUnroutableRepliesAndPublications.settings"
        "libjoynrSystemIntegration1.settings"
        "libjoynrSystemIntegration2.settings"
        "persistency-cc-disabled.settings"
        "persistency-cc-enabled.settings"
        "ca.crt"
        "testclient.pem"
        "LDAS_checkPermissionToAdd.json"
    )

    foreach(testFile ${TEST_FILES})
        configure_file(
            "resources/${testFile}"
            "${targetDirectory}/test-resources/${testFile}"
            @ONLY
        )
    endforeach()
endmacro(CONFIGURE_TEST_FILES)

if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    CONFIGURE_TEST_FILES("${JOYNR_BINARY_DIR}/Debug")
    CONFIGURE_TEST_FILES("${JOYNR_BINARY_DIR}/Release")
else()
    CONFIGURE_TEST_FILES(${JOYNR_BINARY_DIR})
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")

file(COPY "resources/ListOfCapabilitiesToInject.json"
     DESTINATION "${JOYNR_BINARY_DIR}/test-resources/")

set(MULTIPLE_ACL_RCL_TEST_FILES
    "MasterAccessTable.json"
    "MasterRegistrationTable.json"
    "OwnerRegistrationTable.json"
    "OwnerAccessTable.json"
)

foreach(testFile ${MULTIPLE_ACL_RCL_TEST_FILES})
file(COPY "resources/${testFile}"
     DESTINATION "${JOYNR_BINARY_DIR}/test-resources/multiple-acl-rcl-resources/")
endforeach()

### set up stuff common for all test targets ##################################

if(${GENERATE_SOURCES})
    JoynrGenerator(
        OUTPUTPATH "${CMAKE_CURRENT_SOURCE_DIR}/gen"
        MODELPATH "${CMAKE_CURRENT_SOURCE_DIR}/../../basemodel/src/test/franca"
        GENERATION_LANGUAGE "cpp"
    )
    JoynrGenerator(
        OUTPUTPATH "${CMAKE_CURRENT_SOURCE_DIR}/gen-with-version"
        MODELPATH "${CMAKE_CURRENT_SOURCE_DIR}/../../basemodel/src/test/franca-with-version"
        GENERATION_LANGUAGE "cpp"
        ADD_VERSION_TO none
    )
    JoynrGenerator(
        OUTPUTPATH "${CMAKE_CURRENT_SOURCE_DIR}/gen-with-version"
        MODELPATH "${CMAKE_CURRENT_SOURCE_DIR}/../../basemodel/src/test/franca-with-version"
        GENERATION_LANGUAGE "cpp"
        ADD_VERSION_TO name
    )
    JoynrGenerator(
        OUTPUTPATH "${CMAKE_CURRENT_SOURCE_DIR}/gen-with-version"
        MODELPATH "${CMAKE_CURRENT_SOURCE_DIR}/../../basemodel/src/test/franca-with-version"
        GENERATION_LANGUAGE "cpp"
        ADD_VERSION_TO package
    )
endif(${GENERATE_SOURCES})

get_target_property(joynrwsruntime_PUBLIC_INCLUDE_DIRECTORY JoynrWsRuntime PUBLIC_INCLUDE_DIRECTORY)
get_target_property(JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY JoynrClusterController PUBLIC_INCLUDE_DIRECTORY)
get_target_property(JoynrClusterControllerRuntime_PUBLIC_INCLUDE_DIRECTORY JoynrClusterControllerRuntime PUBLIC_INCLUDE_DIRECTORY)

set(
    test_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_SOURCE_DIR}/gen/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/gen-with-version/include"
    ${joynrwsruntime_PUBLIC_INCLUDE_DIRECTORY}
    ${JoynrClusterController_PUBLIC_INCLUDE_DIRECTORY}
    ${JoynrClusterControllerRuntime_PUBLIC_INCLUDE_DIRECTORY}
)

set(
    JoynrTest_SYSTEM_INCLUDE_DIRECTORIES
    ${spdlog_INCLUDE_DIRS}
)

file(
    GLOB_RECURSE JoynrTest_GENSOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "gen*/*.cpp"
)

file(
    GLOB_RECURSE JoynrTest_GENHEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "gen*/*.h"
)

add_library(TestGenerated SHARED
    ${JoynrTest_GENSOURCES}
    ${JoynrTest_GENHEADERS}
)

# add needed dependencies by linking to target Joynr
target_link_libraries(TestGenerated Joynr)

if(NOT USE_PLATFORM_SPDLOG)
    add_dependencies(TestGenerated spdlog)
endif(NOT USE_PLATFORM_SPDLOG)

target_include_directories(
    TestGenerated
    PUBLIC ${test_INCLUDE_DIRECTORIES}
)

target_include_directories(
    TestGenerated
    SYSTEM PUBLIC
    ${JoynrTest_SYSTEM_INCLUDE_DIRECTORIES}
)

## Collect link targets for tests
set(
    test_TARGET_LIBRARIES
    Joynr
    JoynrClusterController
    JoynrClusterControllerRuntime
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
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
    )
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")


# Add additional boost dependencies for tests
list(APPEND JOYNR_BOOST_REQUIRED_COMPONENTS regex)
find_package(Boost ${JOYNR_BOOST_REQUIRED_VERSION} REQUIRED
    COMPONENTS ${JOYNR_BOOST_REQUIRED_COMPONENTS}
)
include(CheckBoostImportTargets)

list(
    APPEND test_TARGET_LIBRARIES
    Boost::regex
)

## Collect common header files for tests
set(
    test_HEADERS
    "JoynrTest.h"
    "PrettyPrint.h"
    "utils/PtrUtils.h"
    "utils/TimeUtils.h"
    "utils/TestRunnable.h"
    "utils/TestLibJoynrWebSocketRuntime.h"
    "utils/MyTestProvider.h"
)

## Collect common source files for tests
set(
    test_SOURCES
    "JoynrTest.cpp"
    "PrettyPrint.cpp"
    "utils/TestRunnable.cpp"
    "TestMain.cpp"
)

################
# Mock objects #
################
if(${BUILD_UNIT_TESTS} OR ${BUILD_INTEGRATION_TESTS} OR ${BUILD_SYSTEM_INTEGRATION_TESTS})
    set(
        MOCK_SOURCES
        mock/MockSubscriptionListener.h
        mock/MockCallback.h
	mock/MockCapabilitiesStorage.h
        mock/MockDiscovery.h
        mock/MockProvider.cpp
        mock/MockProvider.h
        mock/MockDispatcher.h
        mock/MockMessagingStub.h
        mock/MockMessagingStubFactory.h
        mock/MockMessageRouter.h
        mock/MockPublicationSender.h
        mock/MockJoynrClusterControllerMqttConnectionData.h
        mock/MockProxyBuilder.h
        mock/MockGlobalCapabilitiesDirectoryClient.h
        mock/MockGlobalCapabilitiesDirectoryProxy.h
        mock/MockSubscriptionManager.h
        mock/MockSubscriptionCallback.h
        mock/MockParticipantIdStorage.h
        mock/MockGlobalDomainAccessControllerProxy.h
        mock/MockGlobalDomainRoleControllerProxy.h
        mock/MockLocalDomainAccessController.h
        mock/MockLocationUpdatedSelectiveFilter.h
        mock/MockMqttMessagingSkeleton.h
        mock/MockMosquittoConnection.h
        mock/MockAccessController.h
        mock/MockTransportStatus.h
        mock/MockClusterControllerSignalHandler.h
        mock/MockWebSocketSendInterface.h
        mock/MockWebSocketClient.h
        mock/MockLocalCapabilitiesDirectory.h
        mock/MockTestRequestCaller.h
        mock/MockRunnable.h
        mock/MockRunnableWithAccuracy.cpp
        mock/MockRunnableWithAccuracy.h
        mock/MockRunnableBlocking.h
        mock/MockKeychain.h
        mock/MockMessageSender.h
        mock/MockReplyCaller.h
        mock/MockGpsFloatSubscriptionListener.h
        mock/MockInProcessMessagingSkeleton.h
        mock/MockGpsProvider.h
        mock/MockMessagingMulticastSubscriber.h
        mock/MockRoutingProxy.h
        mock/MockTransportMessageSender.h
        mock/MockTransportMessageReceiver.h
        mock/MockJoynrRuntime.h
        mock/MockDelayedScheduler.h
        mock/MockTestProvider.h
    )

    add_library(
        JoynrMocks
        STATIC
        ${MOCK_SOURCES}
    )

    if(NOT USE_PLATFORM_GTEST_GMOCK)
        add_dependencies(JoynrMocks googletest)
        add_dependencies(JoynrMocks googlemock)
    endif(NOT USE_PLATFORM_GTEST_GMOCK)

    target_link_libraries(
        JoynrMocks
        PUBLIC
        ${test_TARGET_LIBRARIES}
    )

    target_include_directories(
        JoynrMocks
        SYSTEM
        PUBLIC
        ${GTEST_INCLUDE_DIRS}
        ${GMOCK_INCLUDE_DIRS}
    )

    target_include_directories(
        JoynrMocks
        PUBLIC
        ${test_INCLUDE_DIRECTORIES}
    )
endif(${BUILD_UNIT_TESTS} OR ${BUILD_INTEGRATION_TESTS} OR ${BUILD_SYSTEM_INTEGRATION_TESTS})

###############
# g_UnitTests #
###############

if(${BUILD_UNIT_TESTS})
    file(
        GLOB g_UnitTests_SOURCES
        "unit-tests/*.cpp"
        "unit-tests/mqtt/*.cpp"
        "unit-tests/websocket/*.cpp"
        "unit-tests/jsonserializer/*.cpp"
        "unit-tests/serializer/*.cpp"
    )

    file(
        GLOB g_UnitTests_HEADERS
        "unit-tests/*.h"
    )

    AddTest(
        g_UnitTests
        ${test_HEADERS}
        ${test_SOURCES}
        ${g_UnitTests_HEADERS}
        ${g_UnitTests_SOURCES}
    )

    target_link_libraries(
        g_UnitTests
        TestGenerated
        ${test_TARGET_LIBRARIES}
        JoynrMocks
    )

    target_include_directories(
        g_UnitTests
        PRIVATE ${test_INCLUDE_DIRECTORIES}
    )

    install(TARGETS g_UnitTests TestGenerated
        RUNTIME DESTINATION ${INSTALL_TEST_DIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(${BUILD_UNIT_TESTS})

######################
# g_IntegrationTests #
######################

if(${BUILD_INTEGRATION_TESTS})
    file(
       GLOB g_IntegrationTests_SOURCES
       "integration-tests/*.cpp"
    )

    AddTest(
       g_IntegrationTests
       ${test_HEADERS}
       ${test_SOURCES}
       ${g_IntegrationTests_SOURCES}
       # Make the LibJoynrWebSocketRuntime available in parallel to the cluster controller runtime
       "../runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
       "../runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.cpp"
       "../runtimes/libjoynr-runtime/LibJoynrRuntime.cpp"
       # Allow unit-testing of private header WildcardStorage.h
       "../libjoynrclustercontroller/access-control/WildcardStorage.h"
    )

    target_link_libraries(
        g_IntegrationTests
        TestGenerated
        ${test_TARGET_LIBRARIES}
        JoynrMocks
    )

    target_include_directories(
        g_IntegrationTests
        PRIVATE ${test_INCLUDE_DIRECTORIES}
    )

    install(TARGETS g_IntegrationTests TestGenerated
        RUNTIME DESTINATION ${INSTALL_TEST_DIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(${BUILD_INTEGRATION_TESTS})

############################
# g_SystemIntegrationTests #
############################

if(${BUILD_SYSTEM_INTEGRATION_TESTS})
    file(
       GLOB g_SystemIntegrationTests_SOURCES
       "systemintegration-tests/*.cpp"
    )

    AddTest(
       g_SystemIntegrationTests
       ${test_HEADERS}
       ${test_SOURCES}
       ${g_SystemIntegrationTests_SOURCES}
       "../runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
       "../runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.cpp"
       "../runtimes/libjoynr-runtime/LibJoynrRuntime.cpp"
    )

    target_link_libraries(
        g_SystemIntegrationTests
        TestGenerated
        ${test_TARGET_LIBRARIES}
        JoynrMocks
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
        PRIVATE ${test_INCLUDE_DIRECTORIES}
        PRIVATE ${BOOST_INCLUDE_DIRS}
    )

    install(TARGETS g_SystemIntegrationTests TestGenerated
        RUNTIME DESTINATION ${INSTALL_TEST_DIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(${BUILD_SYSTEM_INTEGRATION_TESTS})

install(DIRECTORY resources
        DESTINATION ${INSTALL_TEST_DIR}
)
