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
#include "utils/TestQString.h"
#include "utils/QThreadSleep.h"
#include "joynr/CapabilitiesAggregator.h"
#include "tests/utils/MockObjects.h"
#include "utils/MockLocalCapabilitiesDirectoryCallback.h"
#include "common/in-process/InProcessMessagingEndpointAddress.h"
#include "joynr/types/ProviderQosRequirements.h"
#include "joynr/IAttributeListener.h"

using namespace ::testing;
using namespace joynr;

// Dummy RequestCaller
class DummyRequestCaller : public RequestCaller {
public:
    DummyRequestCaller(const QString& interfaceName) :
        RequestCaller(interfaceName) {}

    void registerAttributeListener(const QString& attributeName, IAttributeListener* attributeListener) {}
    void unregisterAttributeListener(const QString& attributeName, IAttributeListener* attributeListener) {}
private:
};

// Main test class
class CapabilitiesAggregatorTest : public ::testing::Test {
public:
    CapabilitiesAggregatorTest()
        : mockDispatcher(),
          mockCapabilitiesStub(),
          capAggregator(),
          participantId("participantId"),
          domain("testDomain"),
          interfaceName("testInterfaceName"),
          reqCacheDataFreshness_ms(10000),
          mockLookupResults(),
          requestCaller(new DummyRequestCaller(interfaceName)),
          discoveryQos(reqCacheDataFreshness_ms)
    {

    }

    void SetUp(){
        mockDispatcher = new MockIRequestCallerDirectory();
        mockCapabilitiesStub = new MockCapabilitiesStub();
        capAggregator = new CapabilitiesAggregator(mockCapabilitiesStub, mockDispatcher);
        mockLookupResults.clear();
        mockLookupResults.append(CapabilityEntry(domain,
                                             interfaceName,
                                             types::ProviderQos(),
                                             participantId,
                                             QList<QSharedPointer<EndpointAddressBase> >(),
                                             true));
    }

    void TearDown(){
        delete capAggregator;
        delete mockCapabilitiesStub;
        delete mockDispatcher;
    }

    QList<CapabilityEntry> fakeLookupBlocking(){
        QThreadSleep::msleep(2000);
        return mockLookupResults;
    }

protected:
    MockIRequestCallerDirectory* mockDispatcher;
    MockCapabilitiesStub* mockCapabilitiesStub;
    CapabilitiesAggregator* capAggregator;
    QString participantId;
    QString domain;
    QString interfaceName;
    qint64 reqCacheDataFreshness_ms;
    QList<CapabilityEntry> mockLookupResults;
    QSharedPointer<RequestCaller> requestCaller;
    const DiscoveryQos discoveryQos;
private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesAggregatorTest);
};

TEST_F(CapabilitiesAggregatorTest, lookup_byId_addsInProcessAddress){
    EXPECT_CALL(*mockDispatcher, containsRequestCaller(participantId))
            .Times(1)
            .WillOnce(Return(true));
    EXPECT_CALL(*mockDispatcher, lookupRequestCaller(participantId))
            .Times(1)
            .WillOnce(Return(requestCaller));
    EXPECT_CALL(*mockCapabilitiesStub, lookup(participantId, _))
            .Times(1)
            .WillOnce(Return(mockLookupResults));

    QList<CapabilityEntry> results = capAggregator->lookup(participantId, discoveryQos);
    ASSERT_EQ(1, results.size());
    CapabilityEntry firstEntry = results.first();
    ASSERT_EQ(1, firstEntry.getEndpointAddresses().size());
    QSharedPointer<EndpointAddressBase> firstAddress = firstEntry.getEndpointAddresses().first();
    ASSERT_STREQ(InProcessEndpointAddress::ENDPOINTADDRESSTYPE.toLatin1(), firstAddress->metaObject()->className());
    QSharedPointer<InProcessEndpointAddress> inProcessAddress = firstAddress.dynamicCast<InProcessEndpointAddress>();
    EXPECT_EQ(requestCaller, inProcessAddress->getRequestCaller());
}

TEST_F(CapabilitiesAggregatorTest, lookup_byInterface_addsInProcessAddress){
    EXPECT_CALL(*mockDispatcher, containsRequestCaller(participantId))
            .Times(1)
            .WillOnce(Return(true));
    EXPECT_CALL(*mockDispatcher, lookupRequestCaller(participantId))
            .Times(1)
            .WillOnce(Return(requestCaller));
    EXPECT_CALL(*mockCapabilitiesStub, lookup(domain, interfaceName, _, _))
            .Times(1)
            .WillOnce(Return(mockLookupResults));

    QList<CapabilityEntry> results = capAggregator->lookup(domain, interfaceName, types::ProviderQosRequirements(), discoveryQos);
    ASSERT_EQ(1, results.size());
    CapabilityEntry firstEntry = results.first();
    ASSERT_EQ(1, firstEntry.getEndpointAddresses().size());
    QSharedPointer<EndpointAddressBase> firstAddress = firstEntry.getEndpointAddresses().first();
    ASSERT_STREQ(InProcessEndpointAddress::ENDPOINTADDRESSTYPE.toLatin1(), firstAddress->metaObject()->className());
    QSharedPointer<InProcessEndpointAddress> inProcessAddress = firstAddress.dynamicCast<InProcessEndpointAddress>();
    EXPECT_EQ(requestCaller, inProcessAddress->getRequestCaller());
}

TEST_F(CapabilitiesAggregatorTest, async_lookup_byId){
    EXPECT_CALL(*mockCapabilitiesStub, lookup(participantId, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &CapabilitiesAggregatorTest::fakeLookupBlocking));
    EXPECT_CALL(*mockDispatcher, containsRequestCaller(participantId))
            .Times(1)
            .WillOnce(Return(false));

    EXPECT_CALL(*mockDispatcher, lookupRequestCaller(participantId))
            .Times(0);

    QSharedPointer<MockLocalCapabilitiesDirectoryCallback> callback(new MockLocalCapabilitiesDirectoryCallback());
    QDateTime startTime(QDateTime::currentDateTime());
    capAggregator->lookup(participantId, discoveryQos, callback);
    QDateTime endTime(QDateTime::currentDateTime());
    //make shure the call was not blocking (mockCapabilitiesStub->lookup is blocking for 2 seconds)
    EXPECT_TRUE(startTime.msecsTo(endTime) < 2000);
    QList<CapabilityEntry> results(callback->getResults(3000));
    ASSERT_EQ(1, results.size());
    EXPECT_EQ(0, results.at(0).getEndpointAddresses().size());
    EXPECT_EQ(mockLookupResults.at(0),results.at(0));
}
TEST_F(CapabilitiesAggregatorTest, async_lookup_byInterface){
    EXPECT_CALL(*mockCapabilitiesStub, lookup(domain, interfaceName, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &CapabilitiesAggregatorTest::fakeLookupBlocking));
    EXPECT_CALL(*mockDispatcher, containsRequestCaller(participantId))
            .Times(1)
            .WillOnce(Return(false));

    EXPECT_CALL(*mockDispatcher, lookupRequestCaller(participantId))
            .Times(0);

    QSharedPointer<MockLocalCapabilitiesDirectoryCallback> callback(new MockLocalCapabilitiesDirectoryCallback());
    QDateTime startTime(QDateTime::currentDateTime());
    capAggregator->lookup(domain, interfaceName, types::ProviderQosRequirements(), discoveryQos, callback);
    QDateTime endTime(QDateTime::currentDateTime());
    //make shure the call was not blocking (mockCapabilitiesStub->lookup is blocking for 2 seconds)
    EXPECT_TRUE(startTime.msecsTo(endTime) < 2000);
    QList<CapabilityEntry> results(callback->getResults(3000));
    ASSERT_EQ(1, results.size());
    EXPECT_EQ(0, results.at(0).getEndpointAddresses().size());
    EXPECT_EQ(mockLookupResults.at(0),results.at(0));
}

TEST_F(CapabilitiesAggregatorTest, async_lookup_byId_addsInProcessAddress){
    EXPECT_CALL(*mockDispatcher, containsRequestCaller(participantId))
            .Times(1)
            .WillOnce(Return(true));
    EXPECT_CALL(*mockDispatcher, lookupRequestCaller(participantId))
            .Times(1)
            .WillOnce(Return(requestCaller));
    EXPECT_CALL(*mockCapabilitiesStub, lookup(participantId, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &CapabilitiesAggregatorTest::fakeLookupBlocking));

    QSharedPointer<MockLocalCapabilitiesDirectoryCallback> callback(new MockLocalCapabilitiesDirectoryCallback());
    capAggregator->lookup(participantId, discoveryQos, callback);
    QList<CapabilityEntry> results(callback->getResults(3000));

    ASSERT_EQ(1, results.size());
    CapabilityEntry firstEntry = results.first();
    ASSERT_EQ(1, firstEntry.getEndpointAddresses().size());
    QSharedPointer<EndpointAddressBase> firstAddress = firstEntry.getEndpointAddresses().first();
    ASSERT_STREQ(InProcessEndpointAddress::ENDPOINTADDRESSTYPE.toLatin1(), firstAddress->metaObject()->className());
    QSharedPointer<InProcessEndpointAddress> inProcessAddress = firstAddress.dynamicCast<InProcessEndpointAddress>();
    EXPECT_EQ(requestCaller, inProcessAddress->getRequestCaller());
}

TEST_F(CapabilitiesAggregatorTest, async_lookup_byInterface_addsInProcessAddress){
    EXPECT_CALL(*mockDispatcher, containsRequestCaller(participantId))
            .Times(1)
            .WillOnce(Return(true));
    EXPECT_CALL(*mockDispatcher, lookupRequestCaller(participantId))
            .Times(1)
            .WillOnce(Return(requestCaller));
    EXPECT_CALL(*mockCapabilitiesStub, lookup(domain, interfaceName, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &CapabilitiesAggregatorTest::fakeLookupBlocking));

    QSharedPointer<MockLocalCapabilitiesDirectoryCallback> callback(new MockLocalCapabilitiesDirectoryCallback());
    capAggregator->lookup(domain, interfaceName, types::ProviderQosRequirements(), discoveryQos, callback);
    QList<CapabilityEntry> results(callback->getResults(3000));

    ASSERT_EQ(1, results.size());
    CapabilityEntry firstEntry = results.first();
    ASSERT_EQ(1, firstEntry.getEndpointAddresses().size());
    QSharedPointer<EndpointAddressBase> firstAddress = firstEntry.getEndpointAddresses().first();
    ASSERT_STREQ(InProcessEndpointAddress::ENDPOINTADDRESSTYPE.toLatin1(), firstAddress->metaObject()->className());
    QSharedPointer<InProcessEndpointAddress> inProcessAddress = firstAddress.dynamicCast<InProcessEndpointAddress>();
    EXPECT_EQ(requestCaller, inProcessAddress->getRequestCaller());
}

TEST_F(CapabilitiesAggregatorTest, add){
    types::ProviderQos providerQos;
    QList<QSharedPointer<EndpointAddressBase> > endpointAddressList;
    QSharedPointer<EndpointAddressBase> messagingStubAddress(new MockEndpointAddress());

    EXPECT_CALL(*mockCapabilitiesStub, add(domain, interfaceName, participantId, providerQos, endpointAddressList,messagingStubAddress, ICapabilities::NO_TIMEOUT()))
                .Times(1);
    capAggregator->add(domain, interfaceName, participantId, providerQos, endpointAddressList, messagingStubAddress, ICapabilities::NO_TIMEOUT());
}

TEST_F(CapabilitiesAggregatorTest, addEndpoint){
    QSharedPointer<EndpointAddressBase> messagingStubAddress(new MockEndpointAddress());
    EXPECT_CALL(*mockCapabilitiesStub, addEndpoint(participantId, messagingStubAddress, ICapabilities::NO_TIMEOUT()))
            .Times(1);
    capAggregator->addEndpoint(participantId, messagingStubAddress, ICapabilities::NO_TIMEOUT());
}

TEST_F(CapabilitiesAggregatorTest, remove){
    EXPECT_CALL(*mockCapabilitiesStub, remove(participantId, ICapabilities::NO_TIMEOUT())).Times(1);
    capAggregator->remove(participantId, ICapabilities::NO_TIMEOUT());
}
