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
#include "joynr/PrivateCopyAssign.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "PrettyPrint.h"

#include "tests/utils/MockObjects.h"
#include "cluster-controller/messaging/in-process/InProcessCapabilitiesSkeleton.h"
#include "libjoynr/in-process/InProcessCapabilitiesStub.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/ClusterControllerDirectories.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/CapabilitiesAggregator.h"
#include "joynr/ParticipantIdStorage.h"

using namespace joynr;

class InProcessCapabilitiesTest : public ::testing::Test {
public:
    InProcessCapabilitiesTest()
        : messagingEndpointDirectory(new MessagingEndpointDirectory(QString("MessagingEndpointDirectory"))),
          mockCapabilitiesClient(new MockCapabilitiesClient()),
          localCapabilitiesDirectory(new LocalCapabilitiesDirectory(mockCapabilitiesClient, messagingEndpointDirectory)),
          capabilitiesSkeleton(new InProcessCapabilitiesSkeleton(messagingEndpointDirectory, localCapabilitiesDirectory, QString("ccChannelId"))),
          capabilitiesStub(new InProcessCapabilitiesStub(capabilitiesSkeleton)),
          mockDispatcher(new MockInProcessDispatcher()),
          dispatcherList(),
          capabilitiesAggregator(new CapabilitiesAggregator(capabilitiesStub, mockDispatcher)),
          capabilitiesRegistrar(NULL),
          domain("testDomain"),
          interfaceName("test/interface"),
          expectedParticipantId("testParticipant"),
          providerQos(),
          endpointAddressList(),
          messagingStubAddress(new MockEndpointAddress()),
          reqCacheDataFreshness_ms(1000),
          mockProvider(new MockProvider()),
          mockParticipantIdStorage(new MockParticipantIdStorage())
    {
        dispatcherList.append(mockDispatcher);
        capabilitiesRegistrar = new CapabilitiesRegistrar(dispatcherList,
                                                          capabilitiesAggregator,
                                                          messagingStubAddress,
                                                          mockParticipantIdStorage);
//    TM, 14.8.2012  Disabled, because after changes of IDL for capabilitiesDirectory providerQos does not have a radlaufsensor anymore.
//        providerQos.setNumberOfRadumlaufsensors(123);

        providerQos.setPriority(10000);
    }

    ~InProcessCapabilitiesTest(){
        delete localCapabilitiesDirectory;
        delete mockCapabilitiesClient;
        delete messagingEndpointDirectory;
        delete mockDispatcher;
        delete capabilitiesRegistrar;
    }

    void SetUp(){

    }
    void TearDown(){
        // Delete the participant id persistence file
        QFile::remove("joynr_participantIds.settings");
    }

protected:
    MessagingEndpointDirectory* messagingEndpointDirectory;
    MockCapabilitiesClient* mockCapabilitiesClient;
    LocalCapabilitiesDirectory* localCapabilitiesDirectory;
    InProcessCapabilitiesSkeleton* capabilitiesSkeleton;
    ICapabilities* capabilitiesStub;
    MockInProcessDispatcher* mockDispatcher;
    QList<IDispatcher*> dispatcherList;
    QSharedPointer<CapabilitiesAggregator> capabilitiesAggregator;
    CapabilitiesRegistrar* capabilitiesRegistrar;

    QString domain;
    QString interfaceName;
    QString expectedParticipantId;
    types::ProviderQos providerQos;
    QList<QSharedPointer<EndpointAddressBase> > endpointAddressList;
    QSharedPointer<EndpointAddressBase> messagingStubAddress;
    qint64 reqCacheDataFreshness_ms;
    QSharedPointer<MockProvider> mockProvider;
    QSharedPointer<MockParticipantIdStorage> mockParticipantIdStorage;
private:
    DISALLOW_COPY_AND_ASSIGN(InProcessCapabilitiesTest);
};

using namespace ::testing;

TEST_F(InProcessCapabilitiesTest, skeletonAddsToEPDirectory){
    EXPECT_CALL(*mockCapabilitiesClient, registerCapabilities(_)).Times(1);
    EXPECT_CALL(*mockCapabilitiesClient, getLocalChannelId())
            .Times(1)
            .WillOnce(Return(QString("testLocalChannel")));

    capabilitiesSkeleton->add(domain, interfaceName, expectedParticipantId, providerQos, endpointAddressList, messagingStubAddress, ICapabilities::NO_TIMEOUT());
    QSharedPointer<EndpointAddressBase> receivedEndpointAddress(messagingEndpointDirectory->lookup(expectedParticipantId));
    EXPECT_EQ(messagingStubAddress, receivedEndpointAddress);

    DiscoveryQos qos;
    qos.setCacheMaxAge(reqCacheDataFreshness_ms);
    qos.setDiscoveryTimeout(ICapabilities::NO_TIMEOUT());

    QList<CapabilityEntry> lookupResult = capabilitiesSkeleton->lookup(expectedParticipantId, qos);
    ASSERT_EQ(1, lookupResult.size());
    EXPECT_QSTREQ(domain, lookupResult.at(0).getDomain());
    EXPECT_QSTREQ(interfaceName ,lookupResult.at(0).getInterfaceName());
    EXPECT_QSTREQ(expectedParticipantId ,lookupResult.at(0).getParticipantId());
    EXPECT_EQ(providerQos ,lookupResult.at(0).getQos());
}

TEST_F(InProcessCapabilitiesTest, registrarAddsRequestCallerAndRegistersAtCC){
    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    domain,
					IMockProviderInterface::getInterfaceName(),
                    _
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));
    EXPECT_CALL(*mockCapabilitiesClient, registerCapabilities(_))
            .Times(1);
    EXPECT_CALL(*mockCapabilitiesClient, getLocalChannelId())
            .Times(1)
            .WillOnce(Return(QString("testLocalChannel")));
    EXPECT_CALL(*mockProvider, getProviderQos())
            .Times(1)
            .WillOnce(Return(types::ProviderQos()));

    EXPECT_CALL(*mockDispatcher, addRequestCaller(expectedParticipantId, _)).Times(1);

    EXPECT_CALL(*mockDispatcher, containsRequestCaller(expectedParticipantId))
            .Times(1)
            .WillOnce(Return(false));


    QString participantId = capabilitiesRegistrar->registerCapability(domain, mockProvider, QString());
    EXPECT_QSTREQ(expectedParticipantId, participantId);

    DiscoveryQos qos;
    qos.setCacheMaxAge(reqCacheDataFreshness_ms);
    qos.setDiscoveryTimeout(ICapabilities::NO_TIMEOUT());

    QList<CapabilityEntry> lookupResult = capabilitiesAggregator->lookup(expectedParticipantId, qos);
    ASSERT_EQ(1, lookupResult.size());
    EXPECT_QSTREQ(domain, lookupResult.at(0).getDomain());
    EXPECT_QSTREQ(interfaceName, lookupResult.at(0).getInterfaceName());
    EXPECT_QSTREQ(expectedParticipantId, lookupResult.at(0).getParticipantId());

    ASSERT_TRUE(messagingEndpointDirectory->contains(expectedParticipantId));
    EXPECT_EQ(messagingStubAddress, messagingEndpointDirectory->lookup(expectedParticipantId));
}


