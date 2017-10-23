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
#include <string>
#include <unordered_set>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/DiscoveryQos.h"
#include "joynr/Arbitrator.h"
#include "joynr/exceptions/NoCompatibleProviderFoundException.h"
#include "joynr/types/Version.h"
#include "joynr/ArbitrationStrategyFunction.h"
#include "joynr/LastSeenArbitrationStrategyFunction.h"
#include "joynr/QosArbitrationStrategyFunction.h"
#include "joynr/FixedParticipantArbitrationStrategyFunction.h"
#include "joynr/KeywordArbitrationStrategyFunction.h"
#include "joynr/Semaphore.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/Future.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockDiscovery.h"

using ::testing::AtLeast;
using ::testing::Throw;
using ::testing::Return;
using ::testing::_;
using ::testing::A;

using namespace joynr;

static const std::string domain("unittest-domain");
static const std::string interfaceName("unittest-interface");

MATCHER_P(discoveryException, msg, "") {
    return arg.getTypeName() == joynr::exceptions::DiscoveryException::TYPE_NAME() && arg.getMessage() == msg;
}
class MockArbitrator : public Arbitrator {
public:
    MockArbitrator(const std::string& domain,
                       const std::string& interfaceName,
                       const joynr::types::Version& interfaceVersion,
                       std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy,
                       const DiscoveryQos& discoveryQos,
                       std::unique_ptr<const ArbitrationStrategyFunction> arbitrationStrategyFunction) : Arbitrator(domain,
                               interfaceName,
                               interfaceVersion,
                               discoveryProxy,
                               discoveryQos,
                               std::move(arbitrationStrategyFunction)){};

    MOCK_METHOD0(attemptArbitration, void (void));
};

class ArbitratorTest : public ::testing::Test {
public:
    ArbitratorTest() :
        lastSeenArbitrationStrategyFunction(std::make_unique<const LastSeenArbitrationStrategyFunction>()),
        qosArbitrationStrategyFunction(std::make_unique<const QosArbitrationStrategyFunction>()),
        keywordArbitrationStrategyFunction(std::make_unique<const KeywordArbitrationStrategyFunction>()),
        fixedParticipantArbitrationStrategyFunction(std::make_unique<const FixedParticipantArbitrationStrategyFunction>()),
        lastSeenDateMs(0),
        expiryDateMs(0),
        defaultDiscoveryTimeoutMs(30000),
        defaultRetryIntervalMs(1000),
        publicKeyId("publicKeyId"),
        mockDiscovery(std::make_shared<MockDiscovery>())
        {}

    void testExceptionFromDiscoveryProxy(std::shared_ptr<Arbitrator> arbitrator, const DiscoveryQos& discoveryQos);
    void testExceptionEmptyResult(std::shared_ptr<Arbitrator> arbitrator, const DiscoveryQos& discoveryQos);

    std::unique_ptr<const ArbitrationStrategyFunction> lastSeenArbitrationStrategyFunction;
    std::unique_ptr<const ArbitrationStrategyFunction> qosArbitrationStrategyFunction;
    std::unique_ptr<const ArbitrationStrategyFunction> keywordArbitrationStrategyFunction;
    std::unique_ptr<const ArbitrationStrategyFunction> fixedParticipantArbitrationStrategyFunction;

protected:
    std::int64_t lastSeenDateMs;
    std::int64_t expiryDateMs;
    std::int64_t defaultDiscoveryTimeoutMs;
    std::int64_t defaultRetryIntervalMs;
    std::string publicKeyId;
    ADD_LOGGER(ArbitratorTest)
    std::shared_ptr<MockDiscovery> mockDiscovery;
    Semaphore semaphore;
};

TEST_F(ArbitratorTest, arbitrationTimeout) {
    types::Version providerVersion;
    std::int64_t discoveryTimeoutMs = std::chrono::milliseconds(1000).count();
    std::int64_t retryIntervalMs = std::chrono::milliseconds(450).count();
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(retryIntervalMs);
    auto mockArbitrator = std::make_shared<MockArbitrator>("domain",
                    "interfaceName",
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));

    auto onSuccess = [](const types::DiscoveryEntryWithMetaInfo&) {
        FAIL();
    };

    auto onError = [this](const exceptions::DiscoveryException& exception) {
        EXPECT_THAT(exception, discoveryException("Arbitration could not be finished in time."));
        semaphore.notify();
    };

    auto start = std::chrono::system_clock::now();

    EXPECT_CALL(*mockArbitrator, attemptArbitration()).Times(AtLeast(1));
    mockArbitrator->startArbitration(onSuccess, onError);

    // Wait for timeout
    // Wait for more than discoveryTimeoutMs milliseconds since it might take some time until the 
    // timeout is reported
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryTimeoutMs * 10)));

    auto now = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

    JOYNR_LOG_DEBUG(logger(), "Time elapsed for unsuccessful arbitration : {}", elapsed.count());
    ASSERT_GE(elapsed.count(), discoveryTimeoutMs);
}

// Test that the Arbitrator selects the last seen provider
TEST_F(ArbitratorTest, getLastSeen) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    discoveryQos.setDiscoveryTimeoutMs(defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(defaultRetryIntervalMs);
    joynr::types::Version providerVersion(47, 11);
    auto lastSeenArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));

    std::int64_t latestLastSeenDateMs = 7;
    std::string lastSeenParticipantId = std::to_string(latestLastSeenDateMs);
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    for (std::int64_t i = 0; i <= latestLastSeenDateMs; i++) {
        int64_t lastSeenDateMs = i;   std::string participantId = std::to_string(i);
        discoveryEntries.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 participantId,
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId,
                                 true
        ));
    }

    auto mockFuture = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*mockDiscovery, lookupAsync(_,_,_,_,_)).WillByDefault(Return(mockFuture));

    // Check that the correct participant was selected
    auto onSuccess = [this, &lastSeenParticipantId](const types::DiscoveryEntryWithMetaInfo& discoveryEntry) {
        EXPECT_EQ(lastSeenParticipantId, discoveryEntry.getParticipantId());
        semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException&) {
        FAIL();
    };

    lastSeenArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs())));
}

// Test that the Arbitrator selects the provider with the highest priority
TEST_F(ArbitratorTest, getHighestPriority) {
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(defaultRetryIntervalMs);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    joynr::types::Version providerVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

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
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId,
                                 true
        ));
    }

    auto mockFuture = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*mockDiscovery, lookupAsync(_,_,_,_,_)).WillByDefault(Return(mockFuture));

    // Check that the correct participant was selected
    auto onSuccess = [this, &participantId](const types::DiscoveryEntryWithMetaInfo& discoveryEntry) {
        EXPECT_EQ(participantId.back(), discoveryEntry.getParticipantId());
        semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException&) {
        FAIL();
    };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}

// Test that the Arbitrator selects a provider with compatible version and compatible priority
TEST_F(ArbitratorTest, getHighestPriorityChecksVersion) {
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(defaultRetryIntervalMs);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    joynr::types::Version expectedVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

    // Create a list of discovery entries
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    joynr::types::Version providerVersion;
    int participantIdCounter = 0;
    std::vector<std::string> expectedParticipantIds;
    for (std::int32_t i = -2; i < 2; ++i) {
        providerVersion.setMajorVersion(expectedVersion.getMajorVersion() + i);
        for (std::int32_t j = -2; j < 2; ++j) {
            providerVersion.setMinorVersion(expectedVersion.getMinorVersion() + j);
            discoveryEntries.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                         providerVersion,
                                         domain,
                                         interfaceName,
                                         std::to_string(participantIdCounter),
                                         providerQos,
                                         lastSeenDateMs,
                                         expiryDateMs,
                                         publicKeyId,
                                         true
                                     )
            );
            if (providerVersion.getMajorVersion() == expectedVersion.getMajorVersion() ||
                providerVersion.getMinorVersion() > expectedVersion.getMinorVersion()){
                expectedParticipantIds.push_back(std::to_string(participantIdCounter));
            }
            ++participantIdCounter;
        }
    }

    auto mockFuture = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*mockDiscovery, lookupAsync(_,_,_,_,_)).WillByDefault(Return(mockFuture));

    // Check that one of the expected participant was selected
    auto onSuccess = [this, &expectedParticipantIds](const types::DiscoveryEntryWithMetaInfo& discoveryEntry) {
        EXPECT_TRUE(std::find(expectedParticipantIds.cbegin(),
                              expectedParticipantIds.cend(),
                              discoveryEntry.getParticipantId()
                              ) != expectedParticipantIds.cend() );
        semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException& ex) {
        FAIL() << ex.what();
    };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}

// Test that the Arbitrator selects a provider that supports onChange subscriptions
TEST_F(ArbitratorTest, getHighestPriorityOnChange) {
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(defaultRetryIntervalMs);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setProviderMustSupportOnChange(true);
    joynr::types::Version providerVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

    // Create a list of provider Qos and participant ids
    std::vector<types::ProviderQos> qosEntries;
    std::vector<std::string> participantId;
    for (int priority = 0; priority < 8; priority++) {
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(),
                                priority,
                                types::ProviderScope::GLOBAL,
                                false));
        participantId.push_back(std::to_string(priority));
    }
    for (int priority = 0; priority < 2; priority++) {
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(),
                                priority,
                                types::ProviderScope::GLOBAL,
                                true));
        participantId.push_back("onChange_%1" + std::to_string(priority));
    }

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId,
                                 true
        ));
    }

    auto mockFuture = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*mockDiscovery, lookupAsync(_,_,_,_,_)).WillByDefault(Return(mockFuture));

    // Check that the correct participant was selected
    auto onSuccess = [this, &participantId](const types::DiscoveryEntryWithMetaInfo& discoveryEntry) {
        EXPECT_EQ(participantId.back(), discoveryEntry.getParticipantId());
        semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException&) {
        FAIL();
    };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}

// Test that the Arbitrator selects the provider with the correct keyword
TEST_F(ArbitratorTest, getKeywordProvider) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(defaultRetryIntervalMs);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version providerVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(keywordArbitrationStrategyFunction));

    // Create a list of provider Qos and participant ids
    std::vector<types::ProviderQos> qosEntries;
    std::vector<std::string> participantId;
    for (int priority = 0; priority < 8; priority++) {
        // Entries with no parameters
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(),
                                priority,
                                types::ProviderScope::GLOBAL,
                                false));
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
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId,
                                 true
        ));
    }

    auto mockFuture = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*mockDiscovery, lookupAsync(_,_,_,_,_)).WillByDefault(Return(mockFuture));

    // Check that the correct participant was selected
    auto onSuccess = [this, &participantId](const types::DiscoveryEntryWithMetaInfo& discoveryEntry) {
        EXPECT_EQ(participantId.back(), discoveryEntry.getParticipantId());
        semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException&) {
        FAIL();
    };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs())));
}

// Test that the Arbitrator selects the provider with compatible version
TEST_F(ArbitratorTest, getKeywordProviderChecksVersion) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(defaultRetryIntervalMs);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(keywordArbitrationStrategyFunction));

    // Create a list of discovery entries with the correct keyword
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("keyword", keywordValue));
    types::ProviderQos providerQos(
                      parameterList,                        // custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    joynr::types::Version providerVersion;
    int participantIdCounter = 0;
    std::string expectedParticipantId;
    for (std::int32_t i = -2; i < 2; i++) {
        providerVersion.setMajorVersion(expectedVersion.getMajorVersion() + i);
        for (std::int32_t j = -2; j < 2; j++) {
            providerVersion.setMinorVersion(expectedVersion.getMinorVersion() + j);
            discoveryEntries.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                     providerVersion,
                                     domain,
                                     interfaceName,
                                     std::to_string(participantIdCounter),
                                     providerQos,
                                     lastSeenDateMs,
                                     expiryDateMs,
                                     publicKeyId,
                                     true
            ));
            if (providerVersion == expectedVersion) {
                expectedParticipantId = std::to_string(participantIdCounter);
            }
            participantIdCounter++;
        }
    }

    auto mockFuture = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*mockDiscovery, lookupAsync(_,_,_,_,_)).WillByDefault(Return(mockFuture));

    // Check that the correct participant was selected
    auto onSuccess = [this, &expectedParticipantId](const types::DiscoveryEntryWithMetaInfo& discoveryEntry) {
        EXPECT_EQ(expectedParticipantId, discoveryEntry.getParticipantId());
        semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException&) {
        FAIL();
    };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}

TEST_F(ArbitratorTest, retryFiveTimes) {
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> emptyResult;
    auto mockFuture = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(emptyResult);

    EXPECT_CALL(
                *mockDiscovery,
                lookupAsync(
                    A<const std::vector<std::string>&>(),
                    A<const std::string&>(),
                    A<const joynr::types::DiscoveryQos&>(),
                    _,
                    _
                )
    )
            .Times(5)
            .WillRepeatedly(
                Return(mockFuture)
            );

    DiscoveryQos discoveryQos;
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.setDiscoveryTimeoutMs(450);
    joynr::types::Version providerVersion(47, 11);
    auto lastSeenArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));

    auto onSuccess = [](const types::DiscoveryEntryWithMetaInfo&) { FAIL(); };
    auto onError = [this](const exceptions::DiscoveryException&) {
        semaphore.notify();
    };

    lastSeenArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}

/*
 * Tests that the arbitrators report a NoCompatibleProviderFoundException if only providers
 * with incompatible versions were found
 */

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

// Test that the Arbitrator reports a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getHighestPriorityReturnsNoCompatibleProviderFoundException) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

    // Create a list of discovery entries
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    // discovery entries for first lookup
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId,
                                 true
        ));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId,
                                 true
        ));
        participantIdCounter++;
    }

    auto mockFuture1 = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture1->onSuccess(discoveryEntries1);
    auto mockFuture2 = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture2->onSuccess(discoveryEntries2);
    EXPECT_CALL(*mockDiscovery, lookupAsync(_,_,_,_,_))
            .WillOnce(Return(mockFuture1))
            .WillRepeatedly(Return(mockFuture2));

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    auto onSuccess = [](const types::DiscoveryEntryWithMetaInfo&) {
        FAIL();
    };

    auto onError = [this, &expectedVersions](const exceptions::DiscoveryException& exception) {
        EXPECT_THAT(exception, noCompatibleProviderFoundException(expectedVersions));
        semaphore.notify();
    };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}

// Test that the Arbitrator returns a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getKeywordProviderReturnsNoCompatibleProviderFoundException) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    auto keywordArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(keywordArbitrationStrategyFunction));

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
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId,
                                 true
        ));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId,
                                 true
        ));
        participantIdCounter++;
    }

    auto mockFuture1 = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture1->onSuccess(discoveryEntries1);
    auto mockFuture2 = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture2->onSuccess(discoveryEntries2);
    EXPECT_CALL(*mockDiscovery, lookupAsync(_,_,_,_,_))
            .WillOnce(Return(mockFuture1))
            .WillRepeatedly(Return(mockFuture2));

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    auto onSuccess = [](const types::DiscoveryEntryWithMetaInfo&) {
        FAIL();
    };

    auto onError = [this, &expectedVersions](const exceptions::DiscoveryException& exception) {
        EXPECT_THAT(exception, noCompatibleProviderFoundException(expectedVersions));
        semaphore.notify();
    };

    keywordArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}

// Test that the Arbitrator reports a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getFixedParticipantProviderReturnsNoCompatibleProviderFoundException) {
    // Search for this keyword value
    const std::string participantId("unittests-participantId");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    joynr::types::Version expectedVersion(47, 11);
    auto fixedParticipantArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(fixedParticipantArbitrationStrategyFunction));

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
    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry1(
                             providerVersion1,
                             domain,
                             interfaceName,
                             participantId,
                             providerQos,
                             lastSeenDateMs,
                             expiryDateMs,
                             publicKeyId,
                             true
    );
    // discoveryEntries for subsequent lookups
    joynr::types::Version providerVersion2(23, 12);
    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry2(
                             providerVersion2,
                             domain,
                             interfaceName,
                             participantId,
                             providerQos,
                             lastSeenDateMs,
                             expiryDateMs,
                             publicKeyId,
                             true
    );

    auto mockFuture1 = std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
    mockFuture1->onSuccess(discoveryEntry1);
    auto mockFuture2 = std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
    mockFuture2->onSuccess(discoveryEntry2);
    EXPECT_CALL(*mockDiscovery, lookupAsync(_,_,_))
            .WillOnce(Return(mockFuture1))
            .WillRepeatedly(Return(mockFuture2));

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersion2);

    auto onSuccess = [](const types::DiscoveryEntryWithMetaInfo&) {
        FAIL();
    };

    auto onError = [this, &expectedVersions](const exceptions::DiscoveryException& exception) {
        EXPECT_THAT(exception, noCompatibleProviderFoundException(expectedVersions));
        semaphore.notify();
    };

    fixedParticipantArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}

// Test that the lastSeenArbitrator reports a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getDefaultReturnsNoCompatibleProviderFoundException) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    auto lastSeenArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));

    // Create a list of discovery entries
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    // discovery entries for first lookup
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId,
                                 true
        ));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId,
                                 true
        ));
        participantIdCounter++;
    }

    auto mockFuture1 = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture1->onSuccess(discoveryEntries1);
    auto mockFuture2 = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture2->onSuccess(discoveryEntries2);
    EXPECT_CALL(*mockDiscovery, lookupAsync(_,_,_,_,_))
            .WillOnce(Return(mockFuture1))
            .WillRepeatedly(Return(mockFuture2));

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    auto onSuccess = [](const types::DiscoveryEntryWithMetaInfo&) {
        FAIL();
    };

    auto onError = [this, &expectedVersions](const exceptions::DiscoveryException& exception) {
        EXPECT_THAT(exception, noCompatibleProviderFoundException(expectedVersions));
        semaphore.notify();
    };

    lastSeenArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}


/*
 * Tests that the arbitrators report the exception from the discoveryProxy if the lookup fails
 * during the last retry
 */
MATCHER_P(exceptionFromDiscoveryProxy, originalException, "") {
    std::string expectedErrorMsg = "Unable to lookup provider (domain: " + domain +
            ", interface: " + interfaceName + ") from discovery. Error: " +
            originalException.getMessage();
    return arg.getMessage() == expectedErrorMsg;
}

void ArbitratorTest::testExceptionFromDiscoveryProxy(std::shared_ptr<Arbitrator> arbitrator, const DiscoveryQos& discoveryQos){
    exceptions::JoynrRuntimeException exception1("first exception");
    exceptions::JoynrRuntimeException expectedException("expected exception");

    auto mockFuture1 = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture1->onError(std::make_shared<exceptions::JoynrRuntimeException>(exception1));
    auto mockFuture2 = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture2->onError(std::make_shared<exceptions::JoynrRuntimeException>(expectedException));
    EXPECT_CALL(*mockDiscovery, lookupAsync(_,_,_,_,_))
            .WillOnce(Return(mockFuture1))
            .WillRepeatedly(Return(mockFuture2));

    auto onSuccess = [](const types::DiscoveryEntryWithMetaInfo&) {
        FAIL();
    };

    auto onError = [this, &expectedException](const exceptions::DiscoveryException& exception) {
        EXPECT_THAT(exception, exceptionFromDiscoveryProxy(expectedException));
        semaphore.notify();
    };

    arbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}

TEST_F(ArbitratorTest, getHighestPriorityReturnsExceptionFromDiscoveryProxy) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

    testExceptionFromDiscoveryProxy(qosArbitrator, discoveryQos);
}

TEST_F(ArbitratorTest, getKeywordProviderReturnsExceptionFromDiscoveryProxy) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    auto keywordArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(keywordArbitrationStrategyFunction));

    testExceptionFromDiscoveryProxy(keywordArbitrator, discoveryQos);
}

TEST_F(ArbitratorTest, getFixedParticipantProviderReturnsExceptionFromDiscoveryProxy) {
    // Search for this keyword value
    const std::string participantId("unittests-participantId");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    joynr::types::Version expectedVersion(47, 11);
    auto fixedParticipantArbitrator= std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(fixedParticipantArbitrationStrategyFunction));

    exceptions::JoynrRuntimeException exception1("first exception");
    exceptions::JoynrRuntimeException expectedException("expected exception");
    auto mockFuture1 = std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
    mockFuture1->onError(std::make_shared<exceptions::JoynrRuntimeException>(exception1));
    auto mockFuture2 = std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
    mockFuture2->onError(std::make_shared<exceptions::JoynrRuntimeException>(expectedException));
    EXPECT_CALL(*mockDiscovery, lookupAsync(_,_,_))
            .WillOnce(Return(mockFuture1))
            .WillRepeatedly(Return(mockFuture2));

    auto onSuccess = [](const types::DiscoveryEntryWithMetaInfo&) {
        FAIL();
    };

    auto onError = [this, &expectedException](const exceptions::DiscoveryException& exception) {
        EXPECT_THAT(exception, exceptionFromDiscoveryProxy(expectedException));
        semaphore.notify();
    };

    fixedParticipantArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}

TEST_F(ArbitratorTest, getLastSeenReturnsExceptionFromDiscoveryProxy) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    auto lastSeenArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));

    testExceptionFromDiscoveryProxy(lastSeenArbitrator, discoveryQos);
}


/*
 * Tests that the arbitrators report an exception if no entries were found during the last retry
 */

MATCHER(exceptionEmptyResult, "") {
    std::string expectedErrorMsg = "No entries found for domain: " + domain +
            ", interface: " + interfaceName;
    return arg.getMessage() == expectedErrorMsg;
}

void ArbitratorTest::testExceptionEmptyResult(std::shared_ptr<Arbitrator> arbitrator, const DiscoveryQos& discoveryQos){
    // discovery entries for first lookup
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries1;
    joynr::types::Version providerVersion(22, 23);
    discoveryEntries1.push_back(joynr::types::DiscoveryEntryWithMetaInfo(
                             providerVersion,
                             domain,
                             interfaceName,
                             "testParticipantId",
                             providerQos,
                             lastSeenDateMs,
                             expiryDateMs,
                             publicKeyId,
                             false
    ));

    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries2;

    auto mockFuture1 = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture1->onSuccess(discoveryEntries1);
    auto mockFuture2 = std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture2->onSuccess(discoveryEntries2);
    EXPECT_CALL(*mockDiscovery, lookupAsync(_,_,_,_,_))
            .WillOnce(Return(mockFuture1))
            .WillRepeatedly(Return(mockFuture2));

    auto onSuccess = [](const types::DiscoveryEntryWithMetaInfo&) {
        FAIL();
    };

    auto onError = [this](const exceptions::DiscoveryException& exception) {
        EXPECT_THAT(exception, exceptionEmptyResult());
        semaphore.notify();
    };

    arbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs()*10)));
}

TEST_F(ArbitratorTest, getHighestPriorityReturnsExceptionEmptyResult) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

    testExceptionEmptyResult(qosArbitrator, discoveryQos);
}

TEST_F(ArbitratorTest, getKeywordProviderReturnsExceptionEmptyResult) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    auto keywordArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(keywordArbitrationStrategyFunction));

    testExceptionEmptyResult(keywordArbitrator, discoveryQos);
}

// Arbitrator has no special exception for empty results

TEST_F(ArbitratorTest, getLastSeenReturnsExceptionEmptyResult) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    auto lastSeenArbitrator = std::make_shared<Arbitrator>(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));

    testExceptionEmptyResult(lastSeenArbitrator, discoveryQos);
}
