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
#include "PrettyPrint.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "tests/utils/MockObjects.h"
#include "joynr/TypeUtil.h"

#include "joynr/system/RoutingProxy.h"

using namespace joynr;

class SystemServicesRoutingTest : public ::testing::Test {
public:
    QString settingsFilename;
    QSettings* settings;
    std::string routingDomain;
    QString routingProviderParticipantId;
    JoynrClusterControllerRuntime* runtime;
    IMessageReceiver* mockMessageReceiver;
    MockMessageSender* mockMessageSender;
    DiscoveryQos discoveryQos;
    ProxyBuilder<joynr::system::RoutingProxy>* routingProxyBuilder;
    joynr::system::RoutingProxy* routingProxy;

    SystemServicesRoutingTest() :
            settingsFilename("test-resources/SystemServicesRoutingTest.settings"),
            settings(new QSettings(settingsFilename, QSettings::IniFormat)),
            routingDomain(),
            routingProviderParticipantId(),
            runtime(NULL),
            mockMessageReceiver(new MockMessageReceiver()),
            mockMessageSender(new MockMessageSender()),
            discoveryQos(),
            routingProxyBuilder(NULL),
            routingProxy(NULL)
    {
        SystemServicesSettings systemSettings(*settings);
        systemSettings.printSettings();
        routingDomain = TypeUtil::convertQStringtoStdString(systemSettings.getDomain());
        routingProviderParticipantId = systemSettings.getCcRoutingProviderParticipantId();
        
        discoveryQos.setCacheMaxAge(1000);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
        discoveryQos.addCustomParameter("fixedParticipantId", routingProviderParticipantId);
        discoveryQos.setDiscoveryTimeout(50);

        QString channelId("SystemServicesRoutingTest.ChannelId");
        EXPECT_CALL(*(dynamic_cast<MockMessageReceiver*>(mockMessageReceiver)), getReceiveChannelId())
                .WillRepeatedly(::testing::ReturnRefOfCopy(channelId));

        //runtime can only be created, after MockMessageReceiver has been told to return
        //a channelId for getReceiveChannelId.
        runtime = new JoynrClusterControllerRuntime(NULL, settings, mockMessageReceiver, mockMessageSender);
        // routing provider is normally registered in JoynrClusterControllerRuntime::create
        runtime->registerRoutingProvider();
    }

    ~SystemServicesRoutingTest(){
        runtime->deleteChannel();
        runtime->stopMessaging();
        delete runtime;
        delete settings;
        QFile::remove(settingsFilename);
    }

    void SetUp(){
        routingProxyBuilder = runtime
                ->createProxyBuilder<joynr::system::RoutingProxy>(routingDomain);
    }

    void TearDown(){
        QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME());
        QFile::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
        delete routingProxy;
        delete routingProxyBuilder;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(SystemServicesRoutingTest);
};


TEST_F(SystemServicesRoutingTest, routingProviderIsAvailable)
{
    EXPECT_NO_THROW(
        routingProxy = routingProxyBuilder
                ->setRuntimeQos(MessagingQos(5000))
                ->setCached(false)
                ->setDiscoveryQos(discoveryQos)
                ->build();
    );
}

TEST_F(SystemServicesRoutingTest, unknowParticipantIsNotResolvable)
{
    routingProxy = routingProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    RequestStatus status;
    QString participantId("SystemServicesRoutingTest.ParticipantId.A");
    bool isResolvable = false;
    routingProxy->resolveNextHop(status, isResolvable, participantId);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    EXPECT_FALSE(isResolvable);
}


TEST_F(SystemServicesRoutingTest, addNextHop)
{
    routingProxy = routingProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    RequestStatus status;
    QString participantId("SystemServicesRoutingTest.ParticipantId.A");
    joynr::system::ChannelAddress address("SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    routingProxy->resolveNextHop(status, isResolvable, participantId);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    EXPECT_FALSE(isResolvable);

    routingProxy->addNextHop(status, participantId, address);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    routingProxy->resolveNextHop(status, isResolvable, participantId);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    EXPECT_TRUE(isResolvable);
}

TEST_F(SystemServicesRoutingTest, removeNextHop)
{
    routingProxy = routingProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    RequestStatus status;
    QString participantId("SystemServicesRoutingTest.ParticipantId.A");
    joynr::system::ChannelAddress address("SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    routingProxy->resolveNextHop(status, isResolvable, participantId);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    EXPECT_FALSE(isResolvable);

    routingProxy->addNextHop(status, participantId, address);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    routingProxy->resolveNextHop(status, isResolvable, participantId);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    EXPECT_TRUE(isResolvable);

    routingProxy->removeNextHop(status, participantId);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    routingProxy->resolveNextHop(status, isResolvable, participantId);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    EXPECT_FALSE(isResolvable);
}
