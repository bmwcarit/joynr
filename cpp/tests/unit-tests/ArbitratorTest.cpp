/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include <unordered_set>
#include "joynr/DiscoveryQos.h"
#include "joynr/QosArbitrator.h"
#include "joynr/KeywordArbitrator.h"
#include "joynr/DefaultArbitrator.h"
#include "joynr/exceptions/NoCompatibleProviderFoundException.h"
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
        publicKeyId("publicKeyId"),
        mockDiscovery()
    {}

    void SetUp(){
    }
    void TearDown(){
    }
protected:
    std::int64_t lastSeenDateMs;
    std::int64_t expiryDateMs;
    std::string publicKeyId;
    MockDiscovery mockDiscovery;
};

// Test that the QosArbitrator selects the provider with the highest priority
TEST_F(ArbitratorTest, getHighestPriority) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    joynr::types::Version providerVersion(47, 11);
    QosArbitrator qosArbitrator(domain, interfaceName, providerVersion, mockDiscovery, discoveryQos);

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
                                 expiryDateMs,
                                 publicKeyId
        ));
    }

    // Check that the correct participant was selected
    qosArbitrator.receiveCapabilitiesLookupResults(discoveryEntries);
    EXPECT_EQ(participantId.back(), qosArbitrator.getParticipantId());
}

// Test that the QosArbitrator selects a provider with compatible version
TEST_F(ArbitratorTest, getHighestPriorityChecksVersion) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    joynr::types::Version expectedVersion(47, 11);
    QosArbitrator qosArbitrator(domain, interfaceName, expectedVersion, mockDiscovery, discoveryQos);

    // Create a list of discovery entries
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    joynr::types::Version providerVersion;
    int participantIdCounter = 0;
    std::string expectedParticipantId;
    for (std::int32_t i = -2; i < 2; i++) {
        providerVersion.setMajorVersion(expectedVersion.getMajorVersion() + i);
        for (std::int32_t j = -2; j < 2; j++) {
            providerVersion.setMinorVersion(expectedVersion.getMinorVersion() + j);
            discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                     providerVersion,
                                     domain,
                                     interfaceName,
                                     std::to_string(participantIdCounter),
                                     providerQos,
                                     lastSeenDateMs,
                                     expiryDateMs,
                                     publicKeyId
            ));
            if (providerVersion == expectedVersion) {
                expectedParticipantId = std::to_string(participantIdCounter);
            }
            participantIdCounter++;
        }
    }

    // Check that the correct participant was selected
    qosArbitrator.receiveCapabilitiesLookupResults(discoveryEntries);
    EXPECT_EQ(expectedParticipantId, qosArbitrator.getParticipantId());
}

MATCHER_P(noCompatibleProviderFoundException, expectedVersions, "") {
    try {
        auto exception = dynamic_cast<const exceptions::NoCompatibleProviderFoundException&>(arg);
        if (expectedVersions.size() != exception.getDiscoveredIncompatibleVersions().size()) {
            return false;
        }
        for (const auto& version : expectedVersions) {
            if (exception.getDiscoveredIncompatibleVersions().find(version) == exception.getDiscoveredIncompatibleVersions().end()) {
                return false;
            }
        }
        std::string expectedErrorMessage = "Unable to find a provider with a compatible version. " +
                std::to_string(expectedVersions.size()) + " incompabible versions found:";
        if (expectedErrorMessage != exception.getMessage().substr(0, expectedErrorMessage.size())) {
            return false;
        }
        for (const auto& version : expectedVersions) {
            if (exception.getMessage().find(version.toString()) == std::string::npos) {
                return false;
            }
        }
    } catch (const std::bad_cast& e) {
        return false;
    }
    return true;
}

// Test that the QosArbitrator reports a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getHighestPriorityReturnsException) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    QosArbitrator qosArbitrator(domain, interfaceName, expectedVersion, mockDiscovery, discoveryQos);

    // Create a list of discovery entries
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    // discovery entries for first lookup
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }

    EXPECT_CALL(mockDiscovery, lookup(_,_,_,_))
            .WillOnce(testing::SetArgReferee<0>(discoveryEntries1))
            .WillRepeatedly(testing::SetArgReferee<0>(discoveryEntries2));

    MockArbitrationListener* mockArbitrationListener = new MockArbitrationListener();
    qosArbitrator.setArbitrationListener(mockArbitrationListener);

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationError(noCompatibleProviderFoundException(expectedVersions)));

    qosArbitrator.startArbitration();

    delete mockArbitrationListener;
}

// Test that the QosArbitrator selects a provider that supports onChange subscriptions
TEST_F(ArbitratorTest, getHighestPriorityOnChange) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setProviderMustSupportOnChange(true);
    joynr::types::Version providerVersion(47, 11);
    QosArbitrator qosArbitrator(domain, interfaceName, providerVersion, mockDiscovery, discoveryQos);

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
                                 expiryDateMs,
                                 publicKeyId
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
    joynr::types::Version providerVersion(47, 11);
    KeywordArbitrator keywordArbitrator(domain, interfaceName, providerVersion, mockDiscovery, discoveryQos);

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
                                 expiryDateMs,
                                 publicKeyId
        ));
    }

    // Check that the correct participant was selected
    keywordArbitrator.receiveCapabilitiesLookupResults(discoveryEntries);
    EXPECT_EQ(participantId.back(), keywordArbitrator.getParticipantId());
}

// Test that the KeywordArbitrator selects the provider with compatible version
TEST_F(ArbitratorTest, getKeywordProviderChecksVersion) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    KeywordArbitrator keywordArbitrator(domain, interfaceName, expectedVersion, mockDiscovery, discoveryQos);

    // Create a list of discovery entries with the correct keyword
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("keyword", keywordValue));
    types::ProviderQos providerQos(
                      parameterList,                        // custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    joynr::types::Version providerVersion;
    int participantIdCounter = 0;
    std::string expectedParticipantId;
    for (std::int32_t i = -2; i < 2; i++) {
        providerVersion.setMajorVersion(expectedVersion.getMajorVersion() + i);
        for (std::int32_t j = -2; j < 2; j++) {
            providerVersion.setMinorVersion(expectedVersion.getMinorVersion() + j);
            discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                     providerVersion,
                                     domain,
                                     interfaceName,
                                     std::to_string(participantIdCounter),
                                     providerQos,
                                     lastSeenDateMs,
                                     expiryDateMs,
                                     publicKeyId
            ));
            if (providerVersion == expectedVersion) {
                expectedParticipantId = std::to_string(participantIdCounter);
            }
            participantIdCounter++;
        }
    }

    // Check that the correct participant was selected
    keywordArbitrator.receiveCapabilitiesLookupResults(discoveryEntries);
    EXPECT_EQ(expectedParticipantId, keywordArbitrator.getParticipantId());
}

// Test that the KeywordArbitrator returns a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getKeywordProviderReturnsException) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    KeywordArbitrator keywordArbitrator(domain, interfaceName, expectedVersion, mockDiscovery, discoveryQos);

    // Create a list of discovery entries with the correct keyword
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("keyword", keywordValue));
    types::ProviderQos providerQos(
                      parameterList,                        // custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    // discovery entries for first lookup
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }

    EXPECT_CALL(mockDiscovery, lookup(_,_,_,_))
            .WillOnce(testing::SetArgReferee<0>(discoveryEntries1))
            .WillRepeatedly(testing::SetArgReferee<0>(discoveryEntries2));

    MockArbitrationListener* mockArbitrationListener = new MockArbitrationListener();
    keywordArbitrator.setArbitrationListener(mockArbitrationListener);

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationError(noCompatibleProviderFoundException(expectedVersions)));

    keywordArbitrator.startArbitration();

    delete mockArbitrationListener;
}

// Test that the FixedParticipantArbitrator reports a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getFixedParticipantProviderReturnsException) {
    // Search for this keyword value
    const std::string participantId("unittests-participantId");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    joynr::types::Version expectedVersion(47, 11);
    FixedParticipantArbitrator fixedParticipantArbitrator(domain, interfaceName, expectedVersion, mockDiscovery, discoveryQos);

    // Create a discovery entries with the correct participantId
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("fixedParticipantId", participantId));
    types::ProviderQos providerQos(
                      parameterList,// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    // discovery entries for first lookup
    joynr::types::Version providerVersion1(7, 8);
    joynr::types::DiscoveryEntry discoveryEntry1(
                             providerVersion1,
                             domain,
                             interfaceName,
                             participantId,
                             providerQos,
                             lastSeenDateMs,
                             expiryDateMs,
                             publicKeyId
    );
    // discoveryEntries for subsequent lookups
    joynr::types::Version providerVersion2(23, 12);
    joynr::types::DiscoveryEntry discoveryEntry2(
                             providerVersion2,
                             domain,
                             interfaceName,
                             participantId,
                             providerQos,
                             lastSeenDateMs,
                             expiryDateMs,
                             publicKeyId
    );

    EXPECT_CALL(mockDiscovery, lookup(_,_))
            .WillOnce(testing::SetArgReferee<0>(discoveryEntry1))
            .WillRepeatedly(testing::SetArgReferee<0>(discoveryEntry2));

    MockArbitrationListener* mockArbitrationListener = new MockArbitrationListener();
    fixedParticipantArbitrator.setArbitrationListener(mockArbitrationListener);

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersion2);

    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationError(noCompatibleProviderFoundException(expectedVersions)));

    fixedParticipantArbitrator.startArbitration();

    delete mockArbitrationListener;
}

// Test that the DefaultArbitrator reports a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getDefaultThrowsException) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::NOT_SET);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    DefaultArbitrator defaultArbitrator(domain, interfaceName, expectedVersion, mockDiscovery, discoveryQos);

    // Create a list of discovery entries
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    // discovery entries for first lookup
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }

    EXPECT_CALL(mockDiscovery, lookup(_,_,_,_))
            .WillOnce(testing::SetArgReferee<0>(discoveryEntries1))
            .WillRepeatedly(testing::SetArgReferee<0>(discoveryEntries2));

    MockArbitrationListener* mockArbitrationListener = new MockArbitrationListener();
    defaultArbitrator.setArbitrationListener(mockArbitrationListener);

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationError(noCompatibleProviderFoundException(expectedVersions)));

    defaultArbitrator.startArbitration();

    delete mockArbitrationListener;
}

TEST_F(ArbitratorTest, retryFiveTimes) {
    std::vector<joynr::types::DiscoveryEntry> result;
    EXPECT_CALL(
                mockDiscovery,
                lookup(
                    A<std::vector<joynr::types::DiscoveryEntry>&>(),
                    A<const std::vector<std::string>&>(),
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
    joynr::types::Version providerVersion(47, 11);
    DefaultArbitrator arbitrator(domain, interfaceName, providerVersion, mockDiscovery, discoveryQos);

    arbitrator.startArbitration();
}
