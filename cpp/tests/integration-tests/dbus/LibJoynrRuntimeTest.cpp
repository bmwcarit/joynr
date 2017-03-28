/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <stdint.h>
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "PrettyPrint.h"
#include "runtimes/libjoynr-runtime/dbus/LibJoynrDbusRuntime.h"
#include "joynr/MessagingSettings.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/vehicle/GpsProxy.h"
#include "joynr/vehicle/DefaultGpsProvider.h"
#include "joynr/SystemServicesSettings.h"

#include "joynr/IDbusSkeletonWrapper.h"
#include "joynr/DbusMessagingSkeleton.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/types/CommunicationMiddleware.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/Version.h"
#include "tests/utils/MockObjects.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/Future.h"
#include "joynr/Settings.h"
#include <memory>

#include "common/dbus/DbusMessagingStubAdapter.h"

#include "joynr/tests/Itest.h"
#include "joynr/tests/testProvider.h"
#include "joynr/tests/testProxy.h"

using namespace joynr;

class LibJoynrRuntimeTest : public testing::Test {

public:
    std::string settingsFilename;
    std::string temporarylibjoynrSettingsFilename;
    Settings settings;

    MockMessageReceiver* mockMessageReceiver; // will be deleted when runtime is deleted.
    MockMessageSender* mockMessageSender;
    JoynrClusterControllerRuntime* ccRuntime;
    LibJoynrDbusRuntime* runtime;
    ProxyBuilder<joynr::system::RoutingProxy>* routingProxyBuilder;
    joynr::system::RoutingProxy* routingProxy;
    joynr::types::ProviderQos mockTestProviderQos;
    std::shared_ptr<MockTestProvider> mockTestProvider;
    ProxyBuilder<joynr::system::DiscoveryProxy>* discoveryProxyBuilder;
    joynr::system::DiscoveryProxy* discoveryProxy;

    LibJoynrRuntimeTest() :
            settingsFilename("test-resources/integrationtest.settings"),
            temporarylibjoynrSettingsFilename("test-resouces/LibJoynrRuntimeTest.libjoynr.settings"),
            settings(settingsFilename),
            mockMessageReceiver(new MockMessageReceiver()),
            mockMessageSender(new MockMessageSender()),
            ccRuntime(NULL),
            runtime(NULL),
            routingProxyBuilder(NULL),
            routingProxy(NULL),
            mockTestProviderQos(
                std::vector<joynr::types::CustomParameter>(), // custom provider parameters
                1,                                      // version
                1,                                      // priority
                joynr::types::ProviderScope::LOCAL,     // visibilitiy scope
                false                                   // supports on change subscriptions
            ),
            mockTestProvider(),
            discoveryProxyBuilder(NULL),
            discoveryProxy(NULL)
    {
        std::string channelId("LibJoynrRuntimeTest.ChannelId");

        //runtime can only be created, after MockMessageReceiver has been told to return
        //a channelId for getReceiveChannelId.
        EXPECT_CALL(*mockMessageReceiver, getReceiveChannelId())
                .WillRepeatedly(::testing::ReturnRefOfCopy(channelId));

        ccRuntime = new JoynrClusterControllerRuntime(
                    NULL,
                    new Settings(settingsFilename),
                    mockMessageReceiver,
                    mockMessageSender
        );
        // routing provider is normally registered in JoynrClusterControllerRuntime::create
        ccRuntime->registerRoutingProvider();
        // discovery provider is normally registered in JoynrClusterControllerRuntime::create
        ccRuntime->registerDiscoveryProvider();
    }

    ~LibJoynrRuntimeTest() {
        ccRuntime->deleteChannel();
        ccRuntime->stopMessaging();
        delete ccRuntime;
    }

    void SetUp() {
        // start libjoynr runtime
        runtime = new LibJoynrDbusRuntime(
                    new Settings(temporarylibjoynrSettingsFilename)
        );

        SystemServicesSettings systemSettings(settings);
        systemSettings.printSettings();
        std::string systemServicesDomain(systemSettings.getDomain());

        // setup routing proxy
        std::string routingProviderParticipantId(systemSettings.getCcRoutingProviderParticipantId());
        routingProxyBuilder = runtime
                ->createProxyBuilder<joynr::system::RoutingProxy>(systemServicesDomain);
        DiscoveryQos discoveryQos;
        discoveryQos.setCacheMaxAge(1000);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
        discoveryQos.addCustomParameter("fixedParticipantId", routingProviderParticipantId);
        discoveryQos.setDiscoveryTimeout(50);
        routingProxy = routingProxyBuilder
                ->setMessagingQos(MessagingQos(5000))
                ->setCached(false)
                ->setDiscoveryQos(discoveryQos)
                ->build();
        EXPECT_TRUE(routingProxy != NULL);

        // setup discovery proxy
        std::string discoveryProviderParticipantId(systemSettings.getCcDiscoveryProviderParticipantId());
        discoveryProxyBuilder = runtime
                ->createProxyBuilder<joynr::system::DiscoveryProxy>(systemServicesDomain);
        discoveryQos = DiscoveryQos();
        discoveryQos.setCacheMaxAge(1000);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
        discoveryQos.addCustomParameter("fixedParticipantId", discoveryProviderParticipantId);
        discoveryQos.setDiscoveryTimeout(50);
        discoveryProxy = discoveryProxyBuilder
                ->setMessagingQos(MessagingQos(5000))
                ->setCached(false)
                ->setDiscoveryQos(discoveryQos)
                ->build();
        EXPECT_TRUE(discoveryProxy != NULL);

        mockTestProvider = std::make_shared<MockTestProvider>(mockTestProviderQos);
    }

    void TearDown() {
        delete routingProxyBuilder;
        delete routingProxy;
        delete discoveryProxyBuilder;
        delete discoveryProxy;
        delete runtime;
        std::remove(temporarylibjoynrSettingsFilename.c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }
};

TEST_F(LibJoynrRuntimeTest, instantiateRuntime)
{
    ASSERT_TRUE(runtime != NULL);
}

TEST_F(LibJoynrRuntimeTest, registerProviderAddsNextHopToCcMessageRouter) {
    std::string domain("LibJoynrRuntimeTest.Domain.A");

    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );
    bool resolved = false;
    try {
        routingProxy->resolveNextHop(resolved, participantId);
        EXPECT_TRUE(resolved);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
}

TEST_F(LibJoynrRuntimeTest, unregisterProviderRemovesNextHopToCcMessageRouter) {
    std::string domain("LibJoynrRuntimeTest.Domain.B");

    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    bool resolved = false;
    try {
        routingProxy->resolveNextHop(resolved, participantId);
        EXPECT_TRUE(resolved);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }

    runtime->unregisterProvider(participantId);
    try {
        routingProxy->resolveNextHop(resolved, participantId);
        EXPECT_FALSE(resolved);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop after unregisterProvider was not successful";
    }
}

TEST_F(LibJoynrRuntimeTest, registerProviderAddsEntryToLocalCapDir) {
    std::string domain("LibJoynrRuntimeTest.Domain.F");

    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    std::vector<joynr::types::CommunicationMiddleware::Enum> connections {
            joynr::types::CommunicationMiddleware::JOYNR
    };
    joynr::types::Version providerVersion(47, 11);
    joynr::types::DiscoveryEntry expectedDiscoveryEntry(
                providerVersion,
                domain,
                tests::testProvider::INTERFACE_NAME(),
                participantId,
                mockTestProviderQos,
                connections
    );
    joynr::types::DiscoveryEntry discoveryEntry;
    try {
        discoveryProxy->lookup(discoveryEntry, participantId);
        EXPECT_EQ(expectedDiscoveryEntry, discoveryEntry);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "lookup was not successful";
    }
}

TEST_F(LibJoynrRuntimeTest, arbitrateRegisteredProvider) {
    std::string domain("LibJoynrRuntimeTest.Domain.C");
    auto mockTestProvider = std::make_shared<MockTestProvider>();

    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    tests::testProxy* testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();
    ASSERT_TRUE(testProxy != NULL);

    delete testProxyBuilder;
    delete testProxy;
}

TEST_F(LibJoynrRuntimeTest, callAsyncFunctionOnProvider) {
    std::string domain("LibJoynrRuntimeTest.Domain.D");
    auto mockTestProvider = std::make_shared<MockTestProvider>();

    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    tests::testProxy* testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();
    ASSERT_TRUE(testProxy != NULL);

    std::vector<int32_t> ints;
    ints.push_back(4);
    ints.push_back(6);
    ints.push_back(12);
    int32_t expectedSum = 22;
    std::shared_ptr<Future<int32_t> > future(testProxy->sumIntsAsync(ints));
    try {
        future->wait(500);

        ASSERT_TRUE(future->getStatus().successful());
        int32_t actualValue;
        future->get(actualValue);
        EXPECT_EQ(expectedSum, actualValue);
    } catch (exceptions::JoynrTimeOutException& e) {
        ADD_FAILURE()<< "Timeout waiting for sumIntsAsync";
    }

    delete testProxyBuilder;
    delete testProxy;
}

TEST_F(LibJoynrRuntimeTest, callSyncFunctionOnProvider) {
    std::string domain("LibJoynrRuntimeTest.Domain.E");
    auto mockTestProvider = std::make_shared<MockTestProvider>();

    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    tests::testProxy* testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();
    ASSERT_TRUE(testProxy != NULL);

    std::vector<int32_t> ints;
    ints.push_back(4);
    ints.push_back(6);
    ints.push_back(12);
    int32_t expectedSum = 22;
    int32_t sum = 0;
    try {
        testProxy->sumInts(sum, ints);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "sumInts was not successful";
        EXPECT_EQ(expectedSum, sum);
    }

    delete testProxyBuilder;
    delete testProxy;
}
