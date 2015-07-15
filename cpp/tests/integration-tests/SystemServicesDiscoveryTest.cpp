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

#include "joynr/system/DiscoveryProxy.h"

using namespace joynr;

class SystemServicesDiscoveryTest : public ::testing::Test {
public:
    QString settingsFilename;
    QSettings* settings;
    std::string discoveryDomain;
    QString discoveryProviderParticipantId;
    JoynrClusterControllerRuntime* runtime;
    IMessageReceiver* mockMessageReceiver;
    DiscoveryQos discoveryQos;
    ProxyBuilder<joynr::system::DiscoveryProxy>* discoveryProxyBuilder;
    joynr::system::DiscoveryProxy* discoveryProxy;

    SystemServicesDiscoveryTest() :
        settingsFilename("test-resources/SystemServicesDiscoveryTest.settings"),
        settings(new QSettings(settingsFilename, QSettings::IniFormat)),
        discoveryDomain(),
        discoveryProviderParticipantId(),
        runtime(NULL),
        mockMessageReceiver(new MockMessageReceiver()),
        discoveryQos(),
        discoveryProxyBuilder(NULL),
        discoveryProxy(NULL)
    {
        SystemServicesSettings systemSettings(*settings);
        systemSettings.printSettings();
        discoveryDomain = TypeUtil::convertQStringtoStdString(systemSettings.getDomain());
        discoveryProviderParticipantId = systemSettings.getCcDiscoveryProviderParticipantId();

        discoveryQos.setCacheMaxAge(1000);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
        discoveryQos.addCustomParameter("fixedParticipantId", discoveryProviderParticipantId);
        discoveryQos.setDiscoveryTimeout(50);

        QString channelId("SystemServicesDiscoveryTest.ChannelId");
        EXPECT_CALL(*(dynamic_cast<MockMessageReceiver*>(mockMessageReceiver)), getReceiveChannelId())
                .WillRepeatedly(::testing::ReturnRefOfCopy(channelId));

        //runtime can only be created, after MockCommunicationManager has been told to return
        //a channelId for getReceiveChannelId.
        runtime = new JoynrClusterControllerRuntime(NULL, settings, mockMessageReceiver);
        // discovery provider is normally registered in JoynrClusterControllerRuntime::create
        runtime->registerDiscoveryProvider();
    }

    ~SystemServicesDiscoveryTest(){
        runtime->deleteChannel();
        runtime->stopMessaging();
        delete runtime;
        delete settings;
        QFile::remove(settingsFilename);
    }

    void SetUp(){
        discoveryProxyBuilder = runtime
                ->getProxyBuilder<joynr::system::DiscoveryProxy>(discoveryDomain);
    }

    void TearDown(){
        QFile::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME());
        QFile::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
        delete discoveryProxy;
        delete discoveryProxyBuilder;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(SystemServicesDiscoveryTest);
};


TEST_F(SystemServicesDiscoveryTest, discoveryProviderIsAvailable)
{
    EXPECT_NO_THROW(
        discoveryProxy = discoveryProxyBuilder
                ->setRuntimeQos(MessagingQos(5000))
                ->setCached(false)
                ->setDiscoveryQos(discoveryQos)
                ->build();
    );
}

TEST_F(SystemServicesDiscoveryTest, lookupUnknowParticipantReturnsEmptyResult)
{
    discoveryProxy = discoveryProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    RequestStatus status;
    QList<joynr::system::DiscoveryEntry> result;
    QString domain("SystemServicesDiscoveryTest.Domain.A");
    QString interfaceName("SystemServicesDiscoveryTest.InterfaceName.A");
    joynr::system::DiscoveryQos discoveryQos(
                5000,                                      // max cache age
                joynr::system::DiscoveryScope::LOCAL_ONLY, // discovery scope
                false                                      // provider must support on change subscriptions
    );

    discoveryProxy->lookup(status, result, domain, interfaceName, discoveryQos);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    EXPECT_TRUE(result.isEmpty());
}


TEST_F(SystemServicesDiscoveryTest, add)
{
    discoveryProxy = discoveryProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    RequestStatus status;
    QList<joynr::system::DiscoveryEntry> result;
    QString domain("SystemServicesDiscoveryTest.Domain.A");
    QString interfaceName("SystemServicesDiscoveryTest.InterfaceName.A");
    QString participantId("SystemServicesDiscoveryTest.ParticipantID.A");
    joynr::system::DiscoveryQos discoveryQos(
                5000,                                      // max cache age
                joynr::system::DiscoveryScope::LOCAL_ONLY, // discovery scope
                false                                      // provider must support on change subscriptions
    );
    joynr::types::ProviderQos providerQos(
                QList<joynr::types::CustomParameter>(), // custom provider parameters
                1,                                      // provider version
                1,                                      // priority
                joynr::types::ProviderScope::LOCAL,     // scope for provider registration
                false                                   // provider supports on change subscriptions
    );
    QList<joynr::system::CommunicationMiddleware::Enum> connections;
    connections << joynr::system::CommunicationMiddleware::JOYNR;
    QList<joynr::system::DiscoveryEntry> expectedResult;
    joynr::system::DiscoveryEntry discoveryEntry(
                domain,
                interfaceName,
                participantId,
                providerQos,
                connections
    );
    expectedResult.append(discoveryEntry);


    discoveryProxy->lookup(status, result, domain, interfaceName, discoveryQos);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    EXPECT_TRUE(result.isEmpty());

    discoveryProxy->add(status, discoveryEntry);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());

    discoveryProxy->lookup(status, result, domain, interfaceName, discoveryQos);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    EXPECT_EQ(expectedResult, result);
}

TEST_F(SystemServicesDiscoveryTest, remove)
{
    discoveryProxy = discoveryProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    RequestStatus status;
    QString domain("SystemServicesDiscoveryTest.Domain.A");
    QString interfaceName("SystemServicesDiscoveryTest.InterfaceName.A");
    QString participantId("SystemServicesDiscoveryTest.ParticipantID.A");
    joynr::system::DiscoveryQos discoveryQos(
                5000,                                      // max cache age
                joynr::system::DiscoveryScope::LOCAL_ONLY, // discovery scope
                false                                      // provider must support on change subscriptions
    );
    joynr::types::ProviderQos providerQos(
                QList<joynr::types::CustomParameter>(), // custom provider parameters
                1,                                      // provider version
                1,                                      // priority
                joynr::types::ProviderScope::LOCAL,     // scope for provider registration
                false                                   // provider supports on change subscriptions
    );
    QList<joynr::system::CommunicationMiddleware::Enum> connections;
    connections << joynr::system::CommunicationMiddleware::JOYNR;
    QList<joynr::system::DiscoveryEntry> expectedResult;
    joynr::system::DiscoveryEntry discoveryEntry(
                domain,
                interfaceName,
                participantId,
                providerQos,
                connections
    );
    expectedResult.append(discoveryEntry);

    discoveryProxy->add(status, discoveryEntry);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());

    QList<joynr::system::DiscoveryEntry> result;
    discoveryProxy->lookup(status, result, domain, interfaceName, discoveryQos);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    EXPECT_EQ(expectedResult, result);

    discoveryProxy->remove(status, participantId);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());

    result.clear();
    discoveryProxy->lookup(status, result, domain, interfaceName, discoveryQos);
    EXPECT_EQ(RequestStatusCode::OK, status.getCode());
    EXPECT_TRUE(result.isEmpty());
}
