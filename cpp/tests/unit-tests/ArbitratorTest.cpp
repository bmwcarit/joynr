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
#include <gmock/gmock.h>
#include <string>
#include "joynr/QosArbitrator.h"
#include "joynr/KeywordArbitrator.h"
#include "joynr/DefaultArbitrator.h"
#include "joynr/types/Version.h"

#include "tests/utils/MockObjects.h"

using namespace joynr;

static const std::string domain("unittest-domain");
static const std::string interfaceName("unittest-interface");

class ArbitratorTest : public ::testing::Test {
public:
    ArbitratorTest() :
        lastSeenDateMs(0),
        expiryDateMs(0),
        mockDiscovery()
    {}

    void SetUp(){
    }
    void TearDown(){
    }
protected:
    std::int64_t lastSeenDateMs;
    std::int64_t expiryDateMs;
    MockDiscovery mockDiscovery;
};

// Test that the QosArbitrator selects the provider with the highest priority
TEST_F(ArbitratorTest, getHighestPriority) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    joynr::types::Version providerVersion(47, 11);
    QosArbitrator qosArbitrator(domain, interfaceName, mockDiscovery, discoveryQos);

    // Create a list of provider Qos and participant ids
    std::vector<types::ProviderQos> qosEntries;
    std::vector<std::string> participantId;
    for (int priority = 0; priority < 8; priority++) {
        qosEntries.push_back(types::ProviderQos(
                          std::vector<types::CustomParameter>(),     // custom provider parameters
                          priority,                            // priority
                          joynr::types::ProviderScope::GLOBAL, // discovery scope
                          false                                // supports on change notifications
        ));
        participantId.push_back(std::to_string(priority));
    }

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 lastSeenDateMs,
                                 expiryDateMs
        ));
    }

    // Check that the correct participant was selected
    qosArbitrator.receiveCapabilitiesLookupResults(discoveryEntries);
    EXPECT_EQ(participantId.back(), qosArbitrator.getParticipantId());
}


// Test that the QosArbitrator selects a provider that supports onChange subscriptions
TEST_F(ArbitratorTest, getHighestPriorityOnChange) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setProviderMustSupportOnChange(true);
    joynr::types::Version providerVersion(47, 11);
    QosArbitrator qosArbitrator(domain, interfaceName, mockDiscovery, discoveryQos);

    // Create a list of provider Qos and participant ids
    std::vector<types::ProviderQos> qosEntries;
    std::vector<std::string> participantId;
    for (int priority = 0; priority < 8; priority++) {
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(), priority, types::ProviderScope::GLOBAL, false));
        participantId.push_back(std::to_string(priority));
    }
    for (int priority = 0; priority < 2; priority++) {
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(), priority, types::ProviderScope::GLOBAL, true));
        participantId.push_back("onChange_%1" + std::to_string(priority));
    }

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 lastSeenDateMs,
                                 expiryDateMs
        ));
    }

    // Check that the correct participant was selected
    qosArbitrator.receiveCapabilitiesLookupResults(discoveryEntries);
    EXPECT_EQ(participantId.back(), qosArbitrator.getParticipantId());
}

// Test that the KeywordArbitrator selects the provider with the correct keyword
TEST_F(ArbitratorTest, getKeywordProvider) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    KeywordArbitrator keywordArbitrator(domain, interfaceName, mockDiscovery, discoveryQos);
    joynr::types::Version providerVersion(47, 11);

    // Create a list of provider Qos and participant ids
    std::vector<types::ProviderQos> qosEntries;
    std::vector<std::string> participantId;
    for (int priority = 0; priority < 8; priority++) {
        // Entries with no parameters
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(), priority, types::ProviderScope::GLOBAL, false));
        participantId.push_back(std::to_string(priority));
    }

    // An entry with no keyword parameters
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("xxx", "yyy"));
    qosEntries.push_back(types::ProviderQos(parameterList, 1, types::ProviderScope::GLOBAL, false));
    participantId.push_back("no_keyword");

    // An entry with an incorrect keyword parameter
    parameterList.push_back(types::CustomParameter("keyword", "unwanted"));
    qosEntries.push_back(types::ProviderQos(parameterList, 1, types::ProviderScope::GLOBAL, false));
    participantId.push_back("incorrect_keyword");

    // An entry with the correct keyword parameter
    parameterList.push_back(types::CustomParameter("keyword", keywordValue));
    qosEntries.push_back(types::ProviderQos(parameterList, 1, types::ProviderScope::GLOBAL, false));
    participantId.push_back("correct_keyword");

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 lastSeenDateMs,
                                 expiryDateMs
        ));
    }

    // Check that the correct participant was selected
    keywordArbitrator.receiveCapabilitiesLookupResults(discoveryEntries);
    EXPECT_EQ(participantId.back(), keywordArbitrator.getParticipantId());
}

TEST_F(ArbitratorTest, retryFiveTimes) {
    std::vector<joynr::types::DiscoveryEntry> result;
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
                    testing::Return()
                )
            );

    DiscoveryQos discoveryQos;
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.setDiscoveryTimeoutMs(450);
    DefaultArbitrator arbitrator(domain, interfaceName, mockDiscovery, discoveryQos);

    arbitrator.startArbitration();
}
