/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/system/ChannelAddress.h"
#include "joynr/system/CommonApiDbusAddress.h"
#include "tests/utils/MockObjects.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/Future.h"
#include "joynr/TypeUtil.h"

#include "common/dbus/DbusMessagingStubAdapter.h"

#include "joynr/tests/Itest.h"
#include "joynr/tests/testProvider.h"
#include "joynr/tests/testProxy.h"

using namespace joynr;

class LibJoynrRuntimeTest : public testing::Test {

public:
    QString settingsFilename;
    QString temporarylibjoynrSettingsFilename;
    QSettings settings;

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
            settings(settingsFilename, QSettings::IniFormat),
            mockMessageReceiver(new MockMessageReceiver()),
            mockMessageSender(new MockMessageSender()),
            ccRuntime(NULL),
            runtime(NULL),
            routingProxyBuilder(NULL),
            routingProxy(NULL),
            mockTestProviderQos(
                QList<joynr::types::CustomParameter>(), // custom provider parameters
                1,                                      // version
                1,                                      // priority
                joynr::types::ProviderScope::LOCAL,     // visibilitiy scope
                false                                   // supports on change subscriptions
            ),
            mockTestProvider(),
            discoveryProxyBuilder(NULL),
            discoveryProxy(NULL)
    {
        QString channelId("LibJoynrRuntimeTest.ChannelId");

        //runtime can only be created, after MockMessageReceiver has been told to return
        //a channelId for getReceiveChannelId.
        EXPECT_CALL(*mockMessageReceiver, getReceiveChannelId())
                .WillRepeatedly(::testing::ReturnRefOfCopy(channelId));

        ccRuntime = new JoynrClusterControllerRuntime(
                    NULL,
                    new QSettings(settingsFilename, QSettings::IniFormat),
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
                    new QSettings(temporarylibjoynrSettingsFilename, QSettings::IniFormat)
        );

        SystemServicesSettings systemSettings(settings);
        systemSettings.printSettings();
        std::string systemServicesDomain(TypeUtil::toStd(systemSettings.getDomain()));

        // setup routing proxy
        QString routingProviderParticipantId(systemSettings.getCcRoutingProviderParticipantId());
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
        QString discoveryProviderParticipantId(systemSettings.getCcDiscoveryProviderParticipantId());
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
        QFile::remove(temporarylibjoynrSettingsFilename);
        QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME());
        QFile::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
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
    RequestStatus status;
    bool resolved = false;
    routingProxy->resolveNextHop(status, resolved, participantId);
    ASSERT_TRUE(status.successful());
    EXPECT_TRUE(resolved);
}

TEST_F(LibJoynrRuntimeTest, unregisterProviderRemovesNextHopToCcMessageRouter) {
    std::string domain("LibJoynrRuntimeTest.Domain.B");

    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    RequestStatus status;
    bool resolved = false;
    routingProxy->resolveNextHop(status, resolved, participantId);
    ASSERT_TRUE(status.successful());
    EXPECT_TRUE(resolved);

    runtime->unregisterProvider(participantId);
    routingProxy->resolveNextHop(status, resolved, participantId);
    ASSERT_TRUE(status.successful());
    EXPECT_FALSE(resolved);
}

TEST_F(LibJoynrRuntimeTest, registerProviderAddsEntryToLocalCapDir) {
    std::string domain("LibJoynrRuntimeTest.Domain.F");

    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    QList<joynr::system::CommunicationMiddleware::Enum> connections;
    connections << joynr::system::CommunicationMiddleware::JOYNR;
    joynr::system::DiscoveryEntry expectedDiscoveryEntry(
                TypeUtil::toQt(domain),
                TypeUtil::toQt(tests::testProvider::getInterfaceName()),
                TypeUtil::toQt(participantId),
                mockTestProviderQos,
                connections
    );
    RequestStatus status;
    joynr::system::DiscoveryEntry discoveryEntry;
    discoveryProxy->lookup(status, discoveryEntry, participantId);
    ASSERT_TRUE(status.successful());
    EXPECT_EQ(expectedDiscoveryEntry, discoveryEntry);
}

TEST_F(LibJoynrRuntimeTest, arbitrateRegisteredProvider) {
    std::string domain("LibJoynrRuntimeTest.Domain.C");
    std::shared_ptr<MockTestProvider> mockTestProvider(new MockTestProvider());

    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", TypeUtil::toQt(participantId));
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
    std::shared_ptr<MockTestProvider> mockTestProvider(new MockTestProvider());

    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", TypeUtil::toQt(participantId));
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    tests::testProxy* testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();
    ASSERT_TRUE(testProxy != NULL);

    QList<int> ints;
    ints << 4 << 6 << 12;
    int expectedSum = 22;
    QSharedPointer<Future<int> > future(testProxy->sumInts(ints));
    future->waitForFinished(500);

    ASSERT_TRUE(future->getStatus().successful());
    int actualValue;
    future->getValues(actualValue);
    EXPECT_EQ(expectedSum, actualValue);

    delete testProxyBuilder;
    delete testProxy;
}

TEST_F(LibJoynrRuntimeTest, callSyncFunctionOnProvider) {
    std::string domain("LibJoynrRuntimeTest.Domain.E");
    std::shared_ptr<MockTestProvider> mockTestProvider(new MockTestProvider());

    std::string participantId = runtime->registerProvider<tests::testProvider>(
                domain,
                mockTestProvider
    );

    ProxyBuilder<tests::testProxy>* testProxyBuilder =
            runtime->createProxyBuilder<tests::testProxy>(domain);

    DiscoveryQos discoveryQos(1000);
    discoveryQos.addCustomParameter("fixedParticipantId", TypeUtil::toQt(participantId));
    discoveryQos.setDiscoveryTimeout(50);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    tests::testProxy* testProxy = testProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();
    ASSERT_TRUE(testProxy != NULL);

    QList<int> ints;
    ints << 4 << 6 << 12;
    int expectedSum = 22;
    RequestStatus status;
    int sum = 0;
    testProxy->sumInts(status, sum, ints);
    ASSERT_TRUE(status.successful());
    EXPECT_EQ(expectedSum, sum);

    delete testProxyBuilder;
    delete testProxy;
}
