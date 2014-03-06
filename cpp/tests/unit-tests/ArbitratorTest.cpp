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
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "utils/TestQString.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/QosArbitrator.h"
#include "joynr/KeywordArbitrator.h"
#include "joynr/DefaultArbitrator.h"
#include "libjoynr/some-ip/SomeIpEndpointAddress.h"

#include "tests/utils/MockObjects.h"

using namespace joynr;

static const QString domain("unittest-domain");
static const QString interfaceName("unittest-interface");

class ArbitratorTest : public ::testing::Test {
public:
    ArbitratorTest()  {}

    void SetUp(){
    }
    void TearDown(){
    }
protected:
};

// Test that the QosArbitrator selects the provider with the highest priority
TEST_F(ArbitratorTest, getHighestPriority) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    QosArbitrator qosArbitrator(domain, interfaceName, QSharedPointer<ICapabilities>() , discoveryQos);

    // Create a list of provider Qos and participant ids
    QList<types::ProviderQos> qosEntries;
    QList<QString> participantId;
    for (int priority = 0; priority < 8; priority++) {
        qosEntries << types::ProviderQos(QList<types::CustomParameter>(), 1, priority, joynr::types::ProviderScope::GLOBAL, false);
        participantId << QString::number(priority);
    }

    // Create a list of fake endpoint addresses
    QSharedPointer<joynr::system::Address> endpointAddress(new SomeIpEndpointAddress("1.1.1.1", 80));
    QList<QSharedPointer<joynr::system::Address> > endpointAddresses;
    endpointAddresses << endpointAddress;

    // Create a list of capability entries
    QList<CapabilityEntry> capabilityEntries;
    for (int i = 0; i < qosEntries.size(); i++) {
        capabilityEntries << CapabilityEntry(domain, interfaceName, qosEntries[i], participantId[i],
                                             endpointAddresses, true);
    }

    // Check that the correct participant was selected
    qosArbitrator.receiveCapabilitiesLookupResults(capabilityEntries);
    EXPECT_EQ(participantId.last(), qosArbitrator.getParticipantId());
}


// Test that the QosArbitrator selects a provider that supports onChange subscriptions
TEST_F(ArbitratorTest, getHighestPriorityOnChange) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setProviderMustSupportOnChange(true);
    QosArbitrator qosArbitrator(domain, interfaceName, QSharedPointer<ICapabilities>() , discoveryQos);

    // Create a list of provider Qos and participant ids
    QList<types::ProviderQos> qosEntries;
    QList<QString> participantId;
    for (int priority = 0; priority < 8; priority++) {
        qosEntries << types::ProviderQos(QList<types::CustomParameter>(), 1, priority, types::ProviderScope::GLOBAL, false);
        participantId << QString::number(priority);
    }
    for (int priority = 0; priority < 2; priority++) {
        qosEntries << types::ProviderQos(QList<types::CustomParameter>(), 1, priority, types::ProviderScope::GLOBAL, true);
        participantId << QString("onChange_%1").arg(priority);
    }


    // Create a list of fake endpoint addresses
    QSharedPointer<joynr::system::Address> endpointAddress(new SomeIpEndpointAddress("1.1.1.1", 80));
    QList<QSharedPointer<joynr::system::Address> > endpointAddresses;
    endpointAddresses << endpointAddress;

    // Create a list of capability entries
    QList<CapabilityEntry> capabilityEntries;
    for (int i = 0; i < qosEntries.size(); i++) {
        capabilityEntries << CapabilityEntry(domain, interfaceName, qosEntries[i], participantId[i],
                                             endpointAddresses, true);
    }

    // Check that the correct participant was selected
    qosArbitrator.receiveCapabilitiesLookupResults(capabilityEntries);
    EXPECT_EQ(participantId.last(), qosArbitrator.getParticipantId());
}

// Test that the KeywordArbitrator selects the provider with the correct keyword
TEST_F(ArbitratorTest, getKeywordProvider) {
    // Search for this keyword value
    const QString keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    KeywordArbitrator keywordArbitrator(domain, interfaceName, QSharedPointer<ICapabilities>() , discoveryQos);

    // Create a list of provider Qos and participant ids
    QList<types::ProviderQos> qosEntries;
    QList<QString> participantId;
    for (int priority = 0; priority < 8; priority++) {
        // Entries with no parameters
        qosEntries << types::ProviderQos(QList<types::CustomParameter>(), 1, priority, types::ProviderScope::GLOBAL, false);
        participantId << QString::number(priority);
    }

    // An entry with no keyword parameters
    QList<types::CustomParameter> parameterList;
    parameterList << types::CustomParameter("xxx", "yyy");
    qosEntries << types::ProviderQos(parameterList, 1, 1, types::ProviderScope::GLOBAL, false);
    participantId << QString("no_keyword");

    // An entry with an incorrect keyword parameter
    parameterList << types::CustomParameter("keyword", "unwanted");
    qosEntries << types::ProviderQos(parameterList, 1, 1, types::ProviderScope::GLOBAL, false);
    participantId << QString("incorrect_keyword");

    // An entry with the correct keyword parameter
    parameterList << types::CustomParameter("keyword", keywordValue);
    qosEntries << types::ProviderQos(parameterList, 1, 1, types::ProviderScope::GLOBAL, false);
    participantId << QString("correct_keyword");

    // Create a list of fake endpoint addresses
    QSharedPointer<joynr::system::Address> endpointAddress(new SomeIpEndpointAddress("1.1.1.1", 80));
    QList<QSharedPointer<joynr::system::Address> > endpointAddresses;
    endpointAddresses << endpointAddress;

    // Create a list of capability entries
    QList<CapabilityEntry> capabilityEntries;
    for (int i = 0; i < qosEntries.size(); i++) {
        capabilityEntries << CapabilityEntry(domain, interfaceName, qosEntries[i], participantId[i],
                                             endpointAddresses, true);
    }

    // Check that the correct participant was selected
    keywordArbitrator.receiveCapabilitiesLookupResults(capabilityEntries);
    EXPECT_EQ(participantId.last(), keywordArbitrator.getParticipantId());
}

TEST_F(ArbitratorTest, retryFiveTimes) {
    QList<CapabilityEntry>* result = new QList<CapabilityEntry>();
    QSharedPointer<MockCapabilitiesStub> capaMock(new MockCapabilitiesStub());
    EXPECT_CALL(*capaMock.data(), lookup(A<const QString&>(),
                                  A<const QString&>(),
                                  A<const types::ProviderQosRequirements&>(),
                                  A<const DiscoveryQos&>())).Times(5).WillRepeatedly(testing::Return(*result));

    DiscoveryQos discoveryQos;
    discoveryQos.setRetryInterval(100);
    discoveryQos.setDiscoveryTimeout(450);
    DefaultArbitrator arbitrator(domain, interfaceName, capaMock, discoveryQos);

    arbitrator.startArbitration();
}
