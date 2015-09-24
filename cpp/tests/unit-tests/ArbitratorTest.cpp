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
#include <string>
#include "utils/TestQString.h"
#include "joynr/types/QtProviderQos.h"
#include "joynr/QosArbitrator.h"
#include "joynr/KeywordArbitrator.h"
#include "joynr/DefaultArbitrator.h"
#include "joynr/system/RoutingTypes/QtChannelAddress.h"

#include "tests/utils/MockObjects.h"

using namespace joynr;

static const std::string domain("unittest-domain");
static const std::string interfaceName("unittest-interface");

class ArbitratorTest : public ::testing::Test {
public:
    ArbitratorTest() :
        mockDiscovery()
    {}

    void SetUp(){
    }
    void TearDown(){
    }
protected:
    MockDiscovery mockDiscovery;
};

// Test that the QosArbitrator selects the provider with the highest priority
TEST_F(ArbitratorTest, getHighestPriority) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    QosArbitrator qosArbitrator(domain, interfaceName, mockDiscovery, discoveryQos);

    // Create a list of provider Qos and participant ids
    QList<types::ProviderQos> qosEntries;
    QList<std::string> participantId;
    for (int priority = 0; priority < 8; priority++) {
        qosEntries << types::ProviderQos(
                          std::vector<types::CustomParameter>(),     // custom provider parameters
                          1,                                   // version
                          priority,                            // priority
                          joynr::types::ProviderScope::GLOBAL, // discovery scope
                          false                                // supports on change notifications
        );
        participantId << std::to_string(priority);
    }

    // Create a list of fake connections
    std::vector<joynr::types::CommunicationMiddleware::Enum> connections {
            joynr::types::CommunicationMiddleware::JOYNR
    };

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    for (int i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 connections
        ));
    }

    // Check that the correct participant was selected
    qosArbitrator.receiveCapabilitiesLookupResults(discoveryEntries);
    EXPECT_EQ(participantId.last(), qosArbitrator.getParticipantId());
}


// Test that the QosArbitrator selects a provider that supports onChange subscriptions
TEST_F(ArbitratorTest, getHighestPriorityOnChange) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setProviderMustSupportOnChange(true);
    QosArbitrator qosArbitrator(domain, interfaceName, mockDiscovery, discoveryQos);

    // Create a list of provider Qos and participant ids
    QList<types::ProviderQos> qosEntries;
    QList<std::string> participantId;
    for (int priority = 0; priority < 8; priority++) {
        qosEntries << types::ProviderQos(std::vector<types::CustomParameter>(), 1, priority, types::ProviderScope::GLOBAL, false);
        participantId << std::to_string(priority);
    }
    for (int priority = 0; priority < 2; priority++) {
        qosEntries << types::ProviderQos(std::vector<types::CustomParameter>(), 1, priority, types::ProviderScope::GLOBAL, true);
        participantId << ("onChange_%1" + std::to_string(priority));
    }

    // Create a list of fake connections
    std::vector<joynr::types::CommunicationMiddleware::Enum> connections {
            joynr::types::CommunicationMiddleware::JOYNR
    };

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    for (int i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 connections
        ));
    }

    // Check that the correct participant was selected
    qosArbitrator.receiveCapabilitiesLookupResults(discoveryEntries);
    EXPECT_EQ(participantId.last(), qosArbitrator.getParticipantId());
}

// Test that the KeywordArbitrator selects the provider with the correct keyword
TEST_F(ArbitratorTest, getKeywordProvider) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    KeywordArbitrator keywordArbitrator(domain, interfaceName, mockDiscovery, discoveryQos);

    // Create a list of provider Qos and participant ids
    QList<types::ProviderQos> qosEntries;
    QList<std::string> participantId;
    for (int priority = 0; priority < 8; priority++) {
        // Entries with no parameters
        qosEntries << types::ProviderQos(std::vector<types::CustomParameter>(), 1, priority, types::ProviderScope::GLOBAL, false);
        participantId << std::to_string(priority);
    }

    // An entry with no keyword parameters
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("xxx", "yyy"));
    qosEntries << types::ProviderQos(parameterList, 1, 1, types::ProviderScope::GLOBAL, false);
    participantId << "no_keyword";

    // An entry with an incorrect keyword parameter
    parameterList.push_back(types::CustomParameter("keyword", "unwanted"));
    qosEntries << types::ProviderQos(parameterList, 1, 1, types::ProviderScope::GLOBAL, false);
    participantId << "incorrect_keyword";

    // An entry with the correct keyword parameter
    parameterList.push_back(types::CustomParameter("keyword", keywordValue));
    qosEntries << types::ProviderQos(parameterList, 1, 1, types::ProviderScope::GLOBAL, false);
    participantId << "correct_keyword";

    // Create a list of fake connections
    std::vector<joynr::types::CommunicationMiddleware::Enum> connections {
            joynr::types::CommunicationMiddleware::JOYNR
    };

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    for (int i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 connections
        ));
    }

    // Check that the correct participant was selected
    keywordArbitrator.receiveCapabilitiesLookupResults(discoveryEntries);
    EXPECT_EQ(participantId.last(), keywordArbitrator.getParticipantId());
}

TEST_F(ArbitratorTest, retryFiveTimes) {
    std::vector<joynr::types::DiscoveryEntry> result;
    joynr::RequestStatus status(joynr::RequestStatusCode::OK);
    EXPECT_CALL(
                mockDiscovery,
                lookup(
                    A<std::vector<joynr::types::DiscoveryEntry>&>(),
                    A<const std::string&>(),
                    A<const std::string&>(),
                    A<const joynr::types::DiscoveryQos&>()
                )
    )
            .Times(5)
            .WillRepeatedly(
                testing::DoAll(
                    testing::SetArgReferee<0>(result),
                    testing::Return(status)
                )
            );

    DiscoveryQos discoveryQos;
    discoveryQos.setRetryInterval(100);
    discoveryQos.setDiscoveryTimeout(450);
    DefaultArbitrator arbitrator(domain, interfaceName, mockDiscovery, discoveryQos);

    arbitrator.startArbitration();
}
