option(BUILD_UNIT_TESTS "Build unit tests?" ON)
option(BUILD_INTEGRATION_TESTS "Build integration tests?" ON)
option(BUILD_SYSTEM_INTEGRATION_TESTS "Build system integration tests?" ON)

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
        "uds-libjoynr.settings"
        "CCSettingsWithAccessControlDisabled.settings"
        "CCSettingsWithAccessControlEnabled.settings"
        "CCSettingsWithAccessControlEnabledAndAclFilePathSet.settings"
        "CCSettingsWithGlobalDiscovery.settings"
        "CCSettingsWithMessageQueueLimit.settings"
        "CCSettingsWithUdsDisabled.settings"
        "CCSettingsWithWebSocketDisabled.settings"
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
        ADD_VERSION_TO package
    )
endif(${GENERATE_SOURCES})

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
target_link_libraries(TestGenerated
    PUBLIC Joynr::JoynrLib
)

target_include_directories(
    TestGenerated
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/gen/include"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/gen-with-version/include"
)

## Collect link targets for tests
set(test_TARGET_LIBRARIES)

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
    Boost::regex # Used in "JoynrTest.cpp"
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
    "utils/Gtest.h"
    "utils/Gmock.h"
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
        mock/TestJoynrClusterControllerRuntime.h
        mock/MockProxyBuilder.h
        mock/MockGlobalCapabilitiesDirectoryClient.h
        mock/MockGlobalCapabilitiesDirectoryProxy.h
        mock/MockSubscriptionManager.h
        mock/MockSubscriptionCallback.h
        mock/MockParticipantIdStorage.h
        mock/MockLocalDomainAccessController.h
        mock/MockLocationUpdatedSelectiveFilter.h
        mock/MockMessageQueue.h
        mock/MockMqttMessagingSkeleton.h
        mock/MockMosquittoConnection.h
        mock/MockAccessController.h
        mock/MockTransportStatus.h
        mock/MockClusterControllerSignalHandler.h
        mock/MockWebSocketSendInterface.h
        mock/MockWebSocketClient.h
        mock/MockLocalCapabilitiesDirectory.h
        mock/MockLocalCapabilitiesDirectoryStore.h
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
        mock/MockUdsMessagingStubFactory.h
        mock/MockTaskSequencer.h
    )

    add_library(JoynrMocks STATIC ${MOCK_SOURCES})

    if(NOT USE_PLATFORM_GTEST_GMOCK)
        # gtestLib already has a dependency to googletest and
        # gmockLib already has a dependency to googlemock
        add_dependencies(JoynrMocks gtestLib)
        add_dependencies(JoynrMocks gmockLib)
    endif(NOT USE_PLATFORM_GTEST_GMOCK)

    target_link_libraries(JoynrMocks
        PUBLIC Joynr::JoynrLib
    )

    target_include_directories(JoynrMocks
        # For includes like #include "tests/mock/Mockxxx.h"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/.."
    )

    target_include_directories(JoynrMocks
        SYSTEM PUBLIC ${GTEST_INCLUDE_DIRS}
                      ${GMOCK_INCLUDE_DIRS}
    )

endif(${BUILD_UNIT_TESTS} OR ${BUILD_INTEGRATION_TESTS} OR ${BUILD_SYSTEM_INTEGRATION_TESTS})

function(GetSourceFiles OUTPUT_VAR)
    set(oneValueArgs INPUT_DIR)
    set(multiValueArgs EXCLUDES)
    cmake_parse_arguments(LTS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    file(
        GLOB_RECURSE sources
        "${LTS_INPUT_DIR}/*.cpp"
        "${LTS_INPUT_DIR}/*.h"
    )
    if(LTS_EXCLUDES)
        foreach(exclude ${LTS_EXCLUDES})
            list(FILTER sources EXCLUDE REGEX ${exclude})
        endforeach()
    endif()
    set(${OUTPUT_VAR} ${sources} PARENT_SCOPE)
endfunction()

###############
# g_UnitTests #
###############

if(${BUILD_UNIT_TESTS})
    if(NOT ${JOYNR_SUPPORT_WEBSOCKET})
        list(APPEND excludedUnitTests ".*/websocket/.*")
    endif()
    if(NOT ${JOYNR_SUPPORT_UDS})
        list(APPEND excludedUnitTests ".*/uds/.*")
    endif()
    GetSourceFiles(g_UnitTests_SOURCES INPUT_DIR unit-tests EXCLUDES ${excludedUnitTests})

    RegisterToCtest(g_UnitTests
        SOURCES
            ${test_SOURCES}
            ${test_HEADERS}
            ${g_UnitTests_SOURCES}
        LIBRARIES
            TestGenerated
            ${test_TARGET_LIBRARIES}
            JoynrMocks
            Joynr::JoynrClusterControllerRuntime
        INCLUDES
            "${CMAKE_CURRENT_SOURCE_DIR}/.."
    )

    install(TARGETS g_UnitTests TestGenerated
        RUNTIME DESTINATION ${JOYNR_INSTALL_TEST_DIR}
        LIBRARY DESTINATION ${JOYNR_INSTALL_LIBDIR}
    )
endif(${BUILD_UNIT_TESTS})

######################
# g_IntegrationTests #
######################

if(${BUILD_INTEGRATION_TESTS})
    if(NOT ${JOYNR_SUPPORT_WEBSOCKET})
        list(APPEND excludedIntegrationTests ".*/websocket/.*")
    endif()
    if(NOT ${JOYNR_SUPPORT_UDS})
        list(APPEND excludedIntegrationTests ".*/uds/.*")
    endif()
    GetSourceFiles(g_IntegrationTests_SOURCES INPUT_DIR integration-tests EXCLUDES ${excludedIntegrationTests})

    RegisterToCtest(g_IntegrationTests
    SOURCES
        ${test_SOURCES}
        ${test_HEADERS}
        ${g_IntegrationTests_SOURCES}
    LIBRARIES
        TestGenerated
        ${test_TARGET_LIBRARIES}
        JoynrMocks
        Joynr::JoynrClusterControllerRuntime
        Joynr::JoynrLibJoynrRuntime
    INCLUDES
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

    if(${JOYNR_SUPPORT_WEBSOCKET})
        target_link_objlibraries(g_IntegrationTests
            PUBLIC Joynr::JoynrWsRuntimeWithoutJoynrRuntime
        )
    endif()

    if(${JOYNR_SUPPORT_UDS})
        target_link_objlibraries(g_IntegrationTests
            PUBLIC Joynr::JoynrUdsRuntimeWithoutJoynrRuntime
        )
    endif()

    install(TARGETS g_IntegrationTests TestGenerated
        RUNTIME DESTINATION ${JOYNR_INSTALL_TEST_DIR}
        LIBRARY DESTINATION ${JOYNR_INSTALL_LIBDIR}
    )
endif(${BUILD_INTEGRATION_TESTS})

############################
# g_SystemIntegrationTests #
############################

if(${BUILD_SYSTEM_INTEGRATION_TESTS})
    if(NOT ${JOYNR_SUPPORT_WEBSOCKET})
        list(APPEND excludedSystemIntegrationTests ".*/websocket/.*")
    endif()
    if(NOT ${JOYNR_SUPPORT_UDS})
        list(APPEND excludedSystemIntegrationTests ".*/uds/.*")
    endif()
    GetSourceFiles(g_SystemIntegrationTests_SOURCES INPUT_DIR systemintegration-tests EXCLUDES ${excludedSystemIntegrationTests})

    AddTest(
       g_SystemIntegrationTests
       ${test_HEADERS}
       ${test_SOURCES}
       ${g_SystemIntegrationTests_SOURCES}
    )

    target_link_libraries(g_SystemIntegrationTests PRIVATE
        TestGenerated
        ${test_TARGET_LIBRARIES}
        JoynrMocks
        Joynr::JoynrClusterControllerRuntime
        MoCOCrW::mococrw
    )
    # JoynrRuntime::createRuntime must be found from JoynrClusterControllerRuntime,
    # not from JoynrWsRuntime. Therefore, do not link Joynr::JoynrWsRuntime.
    target_link_objlibraries(g_SystemIntegrationTests
        PUBLIC Joynr::JoynrLibJoynrRuntime
    )

    if(${JOYNR_SUPPORT_WEBSOCKET})
        target_link_objlibraries(g_SystemIntegrationTests
            PUBLIC Joynr::JoynrWsRuntimeWithoutJoynrRuntime
        )
    endif()

    if(${JOYNR_SUPPORT_UDS})
        target_link_objlibraries(g_SystemIntegrationTests
            PUBLIC Joynr::JoynrUdsRuntimeWithoutJoynrRuntime
        )
    endif()

    target_include_directories(
        g_SystemIntegrationTests
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/.."
    )

    install(TARGETS g_SystemIntegrationTests TestGenerated
        RUNTIME DESTINATION ${JOYNR_INSTALL_TEST_DIR}
        LIBRARY DESTINATION ${JOYNR_INSTALL_LIBDIR}
    )
endif(${BUILD_SYSTEM_INTEGRATION_TESTS})

install(DIRECTORY resources
        DESTINATION ${JOYNR_INSTALL_TEST_DIR}
)
