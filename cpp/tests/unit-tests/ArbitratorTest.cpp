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
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <boost/algorithm/string/join.hpp>
#include <boost/none.hpp>
#include <boost/optional/optional_io.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/ArbitrationStrategyFunction.h"
#include "joynr/Arbitrator.h"
#include "joynr/ArbitratorFactory.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/FixedParticipantArbitrationStrategyFunction.h"
#include "joynr/Future.h"
#include "joynr/KeywordArbitrationStrategyFunction.h"
#include "joynr/LastSeenArbitrationStrategyFunction.h"
#include "joynr/QosArbitrationStrategyFunction.h"
#include "joynr/Semaphore.h"
#include "joynr/exceptions/NoCompatibleProviderFoundException.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/Version.h"

#include "tests/PrettyPrint.h"
#include "tests/JoynrTest.h"
#include "tests/mock/MockDiscovery.h"

using ::testing::_;
using ::testing::A;
using ::testing::AtLeast;
using ::testing::Eq;
using ::testing::DoAll;
using ::testing::InvokeWithoutArgs;
using ::testing::Matcher;
using ::testing::Return;
using ::testing::Throw;

using namespace joynr;

static const std::string _domain("unittest-domain");
static const std::string _interfaceName("unittest-interface");
static const std::vector<std::string> _gbids {"testGbid1", "testGbid2", "testGbid3"};
static const std::string _exceptionMsgNoEntriesFound =
        "No entries found for domain: [" + _domain + "], interface: " + _interfaceName;
static std::string getExceptionMsgUnableToLookup(
        const exceptions::JoynrException& exception,
        const std::vector<std::string>& gbids,
        const std::string& participantId = "") {
    static const std::string exceptionMsgPart1 = "Unable to lookup provider (";
    static const std::string exceptionMsgPart2 = ") from discovery. JoynrException: ";
    const std::string params = participantId.empty() ? "domain: [" + _domain + "], interface: " + _interfaceName
                                                     : "participantId: " + participantId;
    const std::string gbidString = gbids.empty() ? "" : ", GBIDs: " + boost::algorithm::join(gbids, ", ");
    return exceptionMsgPart1 + params + gbidString + exceptionMsgPart2 + exception.getMessage();
}
static std::string getErrorMsgUnableToLookup(
        const types::DiscoveryError::Enum& error,
        const std::vector<std::string>& gbids,
        const std::string& participantId = "") {
    static const std::string exceptionMsgPart1 = "Unable to lookup provider (";
    static const std::string exceptionMsgPart2 = ") from discovery. DiscoveryError: ";
    const std::string params = participantId.empty() ? "domain: [" + _domain + "], interface: " + _interfaceName
                                                     : "participantId: " + participantId;
    const std::string gbidString = gbids.empty() ? "" : ", GBIDs: " + boost::algorithm::join(gbids, ", ");
    const std::string errorString = types::DiscoveryError::getLiteral(error);

    return exceptionMsgPart1 + params + gbidString + exceptionMsgPart2 + errorString;
}

class MockArbitrator : public Arbitrator
{
public:
    MockArbitrator(const std::string& domain,
                   const std::string& interfaceName,
                   const joynr::types::Version& interfaceVersion,
                   std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy,
                   const DiscoveryQos& discoveryQos,
                   const std::vector<std::string>& gbids,
                   std::unique_ptr<const ArbitrationStrategyFunction> arbitrationStrategyFunction)
            : Arbitrator(domain,
                         interfaceName,
                         interfaceVersion,
                         discoveryProxy,
                         discoveryQos,
                         gbids,
                         std::move(arbitrationStrategyFunction)){};

    MOCK_METHOD0(attemptArbitration, void(void));
};

class ArbitratorTest : public ::testing::Test
{
public:
    ArbitratorTest()
            : _lastSeenArbitrationStrategyFunction(
                      std::make_unique<const LastSeenArbitrationStrategyFunction>()),
              _qosArbitrationStrategyFunction(
                      std::make_unique<const QosArbitrationStrategyFunction>()),
              _keywordArbitrationStrategyFunction(
                      std::make_unique<const KeywordArbitrationStrategyFunction>()),
              _fixedParticipantArbitrationStrategyFunction(
                      std::make_unique<const FixedParticipantArbitrationStrategyFunction>()),
              _lastSeenDateMs(42),
              _expiryDateMs(0),
              _defaultDiscoveryTimeoutMs(30000),
              _defaultRetryIntervalMs(1000),
              _publicKeyId("publicKeyId"),
              _mockDiscovery(std::make_shared<MockDiscovery>())
    {
    }

    void testExceptionEmptyResult(std::shared_ptr<Arbitrator> arbitrator,
                                  const DiscoveryQos& discoveryQos);
    void testArbitrationStopsOnShutdown(bool testRetry);

    std::unique_ptr<const ArbitrationStrategyFunction> _lastSeenArbitrationStrategyFunction;
    std::unique_ptr<const ArbitrationStrategyFunction> _qosArbitrationStrategyFunction;
    std::unique_ptr<const ArbitrationStrategyFunction> _keywordArbitrationStrategyFunction;
    std::unique_ptr<const ArbitrationStrategyFunction> _fixedParticipantArbitrationStrategyFunction;

protected:
    ADD_LOGGER(ArbitratorTest)
    const std::int64_t _lastSeenDateMs;
    const std::int64_t _expiryDateMs;
    const std::int64_t _defaultDiscoveryTimeoutMs;
    const std::int64_t _defaultRetryIntervalMs;
    const std::string _publicKeyId;
    const std::shared_ptr<MockDiscovery> _mockDiscovery;
    const std::vector<std::string> _emptyGbidsVector;
    Semaphore _semaphore;
};

TEST_F(ArbitratorTest, arbitrationTimeout_callsOnErrorIfNoRetryIsPossible)
{
    types::Version providerVersion;
    const std::int64_t discoveryTimeoutMs = std::chrono::milliseconds(1000).count();
    const std::int64_t retryIntervalMs = std::chrono::milliseconds(400).count();
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(retryIntervalMs);
    auto mockArbitrator =
            std::make_shared<MockArbitrator>("domain",
                                             "interfaceName",
                                             providerVersion,
                                             _mockDiscovery,
                                             discoveryQos,
                                             _emptyGbidsVector,
                                             move(_lastSeenArbitrationStrategyFunction));

    auto onSuccess = [](const ArbitrationResult&) { FAIL(); };

    auto onError = [this](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException,
                    joynrException(joynr::exceptions::DiscoveryException::TYPE_NAME(),
                                   "Arbitration could not be finished in time."));
        _semaphore.notify();
    };

    auto start = std::chrono::system_clock::now();

    EXPECT_CALL(*mockArbitrator, attemptArbitration()).Times(3); // 2 retries
    mockArbitrator->startArbitration(onSuccess, onError);

    // Wait for timeout
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(discoveryTimeoutMs)));

    auto now = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

    JOYNR_LOG_DEBUG(logger(), "Time elapsed for unsuccessful arbitration : {}", elapsed.count());
    ASSERT_LT(elapsed.count(), discoveryTimeoutMs);
    ASSERT_GE(elapsed.count(), 2 * retryIntervalMs);
    mockArbitrator->stopArbitration();
}

// Test that the Arbitrator selects the last seen provider
TEST_F(ArbitratorTest, arbitrationStrategy_lastSeen_selectsCorrectProvider)
{
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    discoveryQos.setDiscoveryTimeoutMs(_defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(_defaultRetryIntervalMs);
    joynr::types::Version providerVersion(47, 11);
    auto lastSeenArbitrator = ArbitratorFactory::createArbitrator(
            _domain, _interfaceName, providerVersion, _mockDiscovery, discoveryQos, _emptyGbidsVector);

    std::int64_t latestLastSeenDateMs = 7;
    std::string lastSeenParticipantId = std::to_string(latestLastSeenDateMs);
    types::ProviderQos providerQos(
            std::vector<types::CustomParameter>(), // custom provider parameters
            42,                                    // priority
            joynr::types::ProviderScope::GLOBAL,   // discovery scope
            false                                  // supports on change notifications
            );

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    for (std::int64_t i = 0; i <= latestLastSeenDateMs; i++) {
        int64_t localLastSeenDateMs = i;
        std::string participantId = std::to_string(i);
        types::ProviderQos newProviderQos = providerQos;
        newProviderQos.setPriority(providerQos.getPriority() - i);
        discoveryEntries.push_back(joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                                            _domain,
                                                                            _interfaceName,
                                                                            participantId,
                                                                            newProviderQos,
                                                                            localLastSeenDateMs,
                                                                            _expiryDateMs,
                                                                            _publicKeyId,
                                                                            true));
    }

    auto mockFuture = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    joynr::types::DiscoveryQos capturedDiscoveryQos;
    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                    _, // domains
                    _, // interfaceName
                    _, // discoveryQos
                    _, // gbids
                    _, // onSuccess
                    _, // onApplicationError
                    _, // onRuntimeError
                    _)) // qos
            .WillOnce(DoAll(::testing::SaveArg<2>(&capturedDiscoveryQos), Return(mockFuture)));

    // Check that the correct participant was selected
    auto onSuccess = [this, &lastSeenParticipantId](
            const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo discoveryEntry = arbitrationResult.getDiscoveryEntries().front();
        EXPECT_EQ(lastSeenParticipantId, discoveryEntry.getParticipantId());
        _semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException& e) { FAIL() << "Got exception: " << e.getMessage(); };

    lastSeenArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs())));

    const auto capturedDiscoveryTimeout = capturedDiscoveryQos.getDiscoveryTimeout();
    const auto capturedCacheMaxAge = capturedDiscoveryQos.getCacheMaxAge();
    const auto capturedDiscoveryScope = capturedDiscoveryQos.getDiscoveryScope();
    const auto capturedProviderMustSupportOnChange = capturedDiscoveryQos.getProviderMustSupportOnChange();
    EXPECT_LE(capturedDiscoveryTimeout, _defaultDiscoveryTimeoutMs);
    EXPECT_GT(capturedDiscoveryTimeout, _defaultDiscoveryTimeoutMs / 2 );
    EXPECT_EQ(capturedCacheMaxAge, discoveryQos.getCacheMaxAgeMs());
    EXPECT_EQ(capturedDiscoveryScope, discoveryQos.getDiscoveryScope());
    EXPECT_EQ(capturedProviderMustSupportOnChange, discoveryQos.getProviderMustSupportOnChange());

    lastSeenArbitrator->stopArbitration();
}

// Test that the Arbitrator selects the provider with the highest priority
TEST_F(ArbitratorTest, arbitrationStrategy_highestPriority_selectsCorrectProvider)
{
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(_defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(_defaultRetryIntervalMs);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    joynr::types::Version providerVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(_domain,
                                                      _interfaceName,
                                                      providerVersion,
                                                      _mockDiscovery,
                                                      discoveryQos,
                                                      _emptyGbidsVector,
                                                      move(_qosArbitrationStrategyFunction));

    // Create a list of provider Qos and participant ids
    std::vector<types::ProviderQos> qosEntries;
    std::vector<std::string> participantIds;
    std::vector<std::int64_t> lastSeenDates;
    for (int priority = 0; priority < 8; priority++) {
        qosEntries.push_back(types::ProviderQos(
                std::vector<types::CustomParameter>(), // custom provider parameters
                priority,                              // priority
                joynr::types::ProviderScope::GLOBAL,   // discovery scope
                false                                  // supports on change notifications
                ));
        participantIds.push_back(std::to_string(priority));
        lastSeenDates.push_back(_lastSeenDateMs - priority);
    }

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                                            _domain,
                                                                            _interfaceName,
                                                                            participantIds[i],
                                                                            qosEntries[i],
                                                                            lastSeenDates[i],
                                                                            _expiryDateMs,
                                                                            _publicKeyId,
                                                                            true));
    }

    auto mockFuture = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*_mockDiscovery,
            lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
        .WillByDefault(Return(mockFuture));

    // Check that the correct participant was selected
    auto onSuccess =
            [this, &participantIds](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo discoveryEntry = arbitrationResult.getDiscoveryEntries().front();
        EXPECT_EQ(participantIds.back(), discoveryEntry.getParticipantId());
        _semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException& e) { FAIL() << "Got exception: " << e.getMessage(); };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));
    qosArbitrator->stopArbitration();
}

// Test that the Arbitrator selects a provider with compatible version and compatible priority
TEST_F(ArbitratorTest, arbitrationStrategy_highestPriority_checksVersion)
{
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(_defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(_defaultRetryIntervalMs);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    joynr::types::Version expectedVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(_domain,
                                                      _interfaceName,
                                                      expectedVersion,
                                                      _mockDiscovery,
                                                      discoveryQos,
                                                      _emptyGbidsVector,
                                                      move(_qosArbitrationStrategyFunction));

    // Create a list of discovery entries
    types::ProviderQos providerQos(
            std::vector<types::CustomParameter>(), // custom provider parameters
            42,                                    // priority
            joynr::types::ProviderScope::GLOBAL,   // discovery scope
            false                                  // supports on change notifications
            );
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    joynr::types::Version providerVersion;
    int participantIdCounter = 0;
    std::vector<std::string> expectedParticipantIds;
    for (std::int32_t i = -2; i < 2; ++i) {
        providerVersion.setMajorVersion(expectedVersion.getMajorVersion() + i);
        for (std::int32_t j = -2; j < 2; ++j) {
            providerVersion.setMinorVersion(expectedVersion.getMinorVersion() + j);
            discoveryEntries.push_back(
                    joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                             _domain,
                                                             _interfaceName,
                                                             std::to_string(participantIdCounter),
                                                             providerQos,
                                                             _lastSeenDateMs,
                                                             _expiryDateMs,
                                                             _publicKeyId,
                                                             true));
            if (providerVersion.getMajorVersion() == expectedVersion.getMajorVersion() &&
                providerVersion.getMinorVersion() >= expectedVersion.getMinorVersion()) {
                expectedParticipantIds.push_back(std::to_string(participantIdCounter));
            }
            ++participantIdCounter;
        }
    }

    auto mockFuture = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*_mockDiscovery,
            lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
        .WillByDefault(Return(mockFuture));

    // Check that one of the expected participant was selected
    auto onSuccess = [this, &expectedParticipantIds](
            const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo discoveryEntry = arbitrationResult.getDiscoveryEntries().front();
        EXPECT_TRUE(std::find(expectedParticipantIds.cbegin(),
                              expectedParticipantIds.cend(),
                              discoveryEntry.getParticipantId()) != expectedParticipantIds.cend());
        _semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException& ex) { FAIL() << "Got exception: " << ex.what(); };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));
    qosArbitrator->stopArbitration();
}

// Test that the Arbitrator selects a provider that supports onChange subscriptions
TEST_F(ArbitratorTest, arbitrationStrategy_highestPriority_checksOnChange)
{
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(_defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(_defaultRetryIntervalMs);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setProviderMustSupportOnChange(true);
    joynr::types::Version providerVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(_domain,
                                                      _interfaceName,
                                                      providerVersion,
                                                      _mockDiscovery,
                                                      discoveryQos,
                                                      _emptyGbidsVector,
                                                      move(_qosArbitrationStrategyFunction));

    // Create a list of provider Qos and participant ids
    std::vector<types::ProviderQos> qosEntries;
    std::vector<std::string> participantIds;
    for (int priority = 0; priority < 8; priority++) {
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(),
                                                priority,
                                                types::ProviderScope::GLOBAL,
                                                true));
        participantIds.push_back(std::to_string(priority));
    }
    const std::string expectedParticipantId = participantIds.back();
    for (int priority = 0; priority < 2; priority++) {
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(),
                                                priority,
                                                types::ProviderScope::GLOBAL,
                                                false));
        participantIds.push_back("onChange_%1" + std::to_string(priority));
    }

    // Create a list of discovery entries (expected provider is in the middle of the list)
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                                            _domain,
                                                                            _interfaceName,
                                                                            participantIds[i],
                                                                            qosEntries[i],
                                                                            _lastSeenDateMs,
                                                                            _expiryDateMs,
                                                                            _publicKeyId,
                                                                            true));
    }

    auto mockFuture = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*_mockDiscovery,
            lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
        .WillByDefault(Return(mockFuture));

    // Check that the correct participant was selected
    auto onSuccess =
            [this, &expectedParticipantId](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo discoveryEntry = arbitrationResult.getDiscoveryEntries().front();
        EXPECT_EQ(expectedParticipantId, discoveryEntry.getParticipantId());
        EXPECT_EQ(true, discoveryEntry.getQos().getSupportsOnChangeSubscriptions());
        _semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException& e) { FAIL() << "Got exception: " << e.getMessage(); };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));
    qosArbitrator->stopArbitration();
}

// Test that the Arbitrator selects the provider with the correct keyword
TEST_F(ArbitratorTest, arbitrationStrategy_keyword_selectsCorrectProvider)
{
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(_defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(_defaultRetryIntervalMs);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version providerVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(_domain,
                                                      _interfaceName,
                                                      providerVersion,
                                                      _mockDiscovery,
                                                      discoveryQos,
                                                      _emptyGbidsVector,
                                                      move(_keywordArbitrationStrategyFunction));

    // Create a list of provider Qos and participant ids
    std::vector<types::ProviderQos> qosEntries;
    std::vector<std::string> participantIds;
    for (int priority = 0; priority < 8; priority++) {
        // Entries with no parameters
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(),
                                                priority,
                                                types::ProviderScope::GLOBAL,
                                                false));
        participantIds.push_back(std::to_string(priority));
    }

    // An entry with no keyword parameters
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("xxx", "yyy"));
    qosEntries.push_back(types::ProviderQos(parameterList, 1, types::ProviderScope::GLOBAL, false));
    participantIds.push_back("no_keyword");

    // An entry with the correct keyword parameter
    parameterList.push_back(types::CustomParameter("keyword", keywordValue));
    qosEntries.push_back(types::ProviderQos(parameterList, 1, types::ProviderScope::GLOBAL, false));
    const std::string expectedParticipantId = "correct_keyword";
    participantIds.push_back(expectedParticipantId);

    // An entry with an incorrect keyword parameter
    parameterList.push_back(types::CustomParameter("keyword", "unwanted"));
    qosEntries.push_back(types::ProviderQos(parameterList, 1, types::ProviderScope::GLOBAL, false));
    participantIds.push_back("incorrect_keyword");

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                                            _domain,
                                                                            _interfaceName,
                                                                            participantIds[i],
                                                                            qosEntries[i],
                                                                            _lastSeenDateMs,
                                                                            _expiryDateMs,
                                                                            _publicKeyId,
                                                                            true));
    }

    auto mockFuture = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*_mockDiscovery,
            lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
        .WillByDefault(Return(mockFuture));

    // Check that the correct participant was selected
    auto onSuccess =
            [this, &expectedParticipantId](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo discoveryEntry = arbitrationResult.getDiscoveryEntries().front();
        EXPECT_EQ(expectedParticipantId, discoveryEntry.getParticipantId());
        _semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException& e) { FAIL() << "Got exception: " << e.getMessage(); };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs())));
    qosArbitrator->stopArbitration();
}

// Test that the Arbitrator selects the provider with compatible version
TEST_F(ArbitratorTest, arbitrationStrategy_keyword_checksVersion)
{
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(_defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(_defaultRetryIntervalMs);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(_domain,
                                                      _interfaceName,
                                                      expectedVersion,
                                                      _mockDiscovery,
                                                      discoveryQos,
                                                      _emptyGbidsVector,
                                                      move(_keywordArbitrationStrategyFunction));

    // Create a list of discovery entries with the correct keyword
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("keyword", keywordValue));
    types::ProviderQos providerQos(parameterList, // custom provider parameters
                                   42,            // priority
                                   joynr::types::ProviderScope::GLOBAL, // discovery scope
                                   false // supports on change notifications
                                   );
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    joynr::types::Version providerVersion;
    int participantIdCounter = 0;
    std::vector<std::string> expectedParticipantIds;
    for (std::int32_t i = -2; i < 2; i++) {
        providerVersion.setMajorVersion(expectedVersion.getMajorVersion() + i);
        for (std::int32_t j = -2; j < 2; j++) {
            providerVersion.setMinorVersion(expectedVersion.getMinorVersion() + j);
            discoveryEntries.push_back(
                    joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                             _domain,
                                                             _interfaceName,
                                                             std::to_string(participantIdCounter),
                                                             providerQos,
                                                             _lastSeenDateMs,
                                                             _expiryDateMs,
                                                             _publicKeyId,
                                                             true));
            if (providerVersion.getMajorVersion() == expectedVersion.getMajorVersion() &&
                providerVersion.getMinorVersion() >= expectedVersion.getMinorVersion()) {
                expectedParticipantIds.push_back(std::to_string(participantIdCounter));
            }
            participantIdCounter++;
        }
    }

    auto mockFuture = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*_mockDiscovery,
            lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
        .WillByDefault(Return(mockFuture));

    // Check that the correct participant was selected
    auto onSuccess = [this, &expectedParticipantIds](
            const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo discoveryEntry = arbitrationResult.getDiscoveryEntries().front();
        EXPECT_TRUE(std::find(expectedParticipantIds.cbegin(),
                              expectedParticipantIds.cend(),
                              discoveryEntry.getParticipantId()) != expectedParticipantIds.cend());
        _semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException& e) { FAIL() << "Got exception: " << e.getMessage(); };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));
    qosArbitrator->stopArbitration();
}

TEST_F(ArbitratorTest, allowFourRetries_expectFiveDiscoveryAttempts)
{
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> emptyResult;
    auto mockFuture = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(emptyResult);

    EXPECT_CALL(*_mockDiscovery,
                lookupAsyncMock(A<const std::vector<std::string>&>(),
                                A<const std::string&>(),
                                A<const joynr::types::DiscoveryQos&>(),
                                _,
                                _,
                                _,
                                _,
                                _))
            .Times(5)
            .WillRepeatedly(Return(mockFuture));

    DiscoveryQos discoveryQos;
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.setDiscoveryTimeoutMs(490);
    joynr::types::Version providerVersion(47, 11);
    auto lastSeenArbitrator =
            std::make_shared<Arbitrator>(_domain,
                                         _interfaceName,
                                         providerVersion,
                                         _mockDiscovery,
                                         discoveryQos,
                                         _emptyGbidsVector,
                                         move(_lastSeenArbitrationStrategyFunction));

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };
    auto onError = [this](const exceptions::DiscoveryException&) { _semaphore.notify(); };

    lastSeenArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));
    lastSeenArbitrator->stopArbitration();
}

/*
 * Tests that the arbitrators report a NoCompatibleProviderFoundException if only providers
 * with incompatible versions were found
 */
MATCHER_P(noCompatibleProviderFoundException, expectedVersions, "")
{
    try {
        auto exception = dynamic_cast<const exceptions::NoCompatibleProviderFoundException&>(arg);
        if (expectedVersions.size() != exception.getDiscoveredIncompatibleVersions().size()) {
            return false;
        }
        for (const auto& version : expectedVersions) {
            if (exception.getDiscoveredIncompatibleVersions().find(version) ==
                exception.getDiscoveredIncompatibleVersions().end()) {
                return false;
            }
        }
        std::string expectedErrorMessage = "Unable to find a provider with a compatible version. " +
                                           std::to_string(expectedVersions.size()) +
                                           " incompabible versions found:";
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
TEST_F(ArbitratorTest, getHighestPriorityReturnsNoCompatibleProviderFoundException)
{
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    auto qosArbitrator = std::make_shared<Arbitrator>(_domain,
                                                      _interfaceName,
                                                      expectedVersion,
                                                      _mockDiscovery,
                                                      discoveryQos,
                                                      _emptyGbidsVector,
                                                      move(_qosArbitrationStrategyFunction));

    // Create a list of discovery entries
    types::ProviderQos providerQos(
            std::vector<types::CustomParameter>(), // custom provider parameters
            42,                                    // priority
            joynr::types::ProviderScope::GLOBAL,   // discovery scope
            false                                  // supports on change notifications
            );
    // discovery entries for first lookup
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(
                joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                         _domain,
                                                         _interfaceName,
                                                         std::to_string(participantIdCounter),
                                                         providerQos,
                                                         _lastSeenDateMs,
                                                         _expiryDateMs,
                                                         _publicKeyId,
                                                         true));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(
                joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                         _domain,
                                                         _interfaceName,
                                                         std::to_string(participantIdCounter),
                                                         providerQos,
                                                         _lastSeenDateMs,
                                                         _expiryDateMs,
                                                         _publicKeyId,
                                                         true));
        participantIdCounter++;
    }

    auto mockFuture1 = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture1->onSuccess(discoveryEntries1);
    auto mockFuture2 = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture2->onSuccess(discoveryEntries2);
    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
            .WillOnce(Return(mockFuture1))
            .WillRepeatedly(Return(mockFuture2));

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };

    auto onError = [this, &expectedVersions](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException, noCompatibleProviderFoundException(expectedVersions));
        _semaphore.notify();
    };

    qosArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));
    qosArbitrator->stopArbitration();
}

// Test that the Arbitrator returns a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getKeywordProviderReturnsNoCompatibleProviderFoundException)
{
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    auto keywordArbitrator = std::make_shared<Arbitrator>(_domain,
                                                          _interfaceName,
                                                          expectedVersion,
                                                          _mockDiscovery,
                                                          discoveryQos,
                                                          _emptyGbidsVector,
                                                          move(_keywordArbitrationStrategyFunction));

    // Create a list of discovery entries with the correct keyword
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("keyword", keywordValue));
    types::ProviderQos providerQos(parameterList, // custom provider parameters
                                   42,            // priority
                                   joynr::types::ProviderScope::GLOBAL, // discovery scope
                                   false // supports on change notifications
                                   );
    // discovery entries for first lookup
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(
                joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                         _domain,
                                                         _interfaceName,
                                                         std::to_string(participantIdCounter),
                                                         providerQos,
                                                         _lastSeenDateMs,
                                                         _expiryDateMs,
                                                         _publicKeyId,
                                                         true));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(
                joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                         _domain,
                                                         _interfaceName,
                                                         std::to_string(participantIdCounter),
                                                         providerQos,
                                                         _lastSeenDateMs,
                                                         _expiryDateMs,
                                                         _publicKeyId,
                                                         true));
        participantIdCounter++;
    }

    auto mockFuture1 = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture1->onSuccess(discoveryEntries1);
    auto mockFuture2 = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture2->onSuccess(discoveryEntries2);
    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
            .WillOnce(Return(mockFuture1))
            .WillRepeatedly(Return(mockFuture2));

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };

    auto onError = [this, &expectedVersions](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException, noCompatibleProviderFoundException(expectedVersions));
        _semaphore.notify();
    };

    keywordArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));
    keywordArbitrator->stopArbitration();
}

// Test that the Arbitrator reports a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getFixedParticipantProviderReturnsNoCompatibleProviderFoundException)
{
    // Search for this keyword value
    const std::string participantId("unittests-participantId");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    joynr::types::Version expectedVersion(47, 11);
    auto fixedParticipantArbitrator =
            std::make_shared<Arbitrator>(_domain,
                                         _interfaceName,
                                         expectedVersion,
                                         _mockDiscovery,
                                         discoveryQos,
                                         _emptyGbidsVector,
                                         move(_fixedParticipantArbitrationStrategyFunction));

    // Create a discovery entries with the correct participantId
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("fixedParticipantId", participantId));
    types::ProviderQos providerQos(parameterList, // custom provider parameters
                                   42,            // priority
                                   joynr::types::ProviderScope::GLOBAL, // discovery scope
                                   false // supports on change notifications
                                   );
    // discovery entries for first lookup
    joynr::types::Version providerVersion1(7, 8);
    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry1(providerVersion1,
                                                             _domain,
                                                             _interfaceName,
                                                             participantId,
                                                             providerQos,
                                                             _lastSeenDateMs,
                                                             _expiryDateMs,
                                                             _publicKeyId,
                                                             true);
    // discoveryEntries for subsequent lookups
    joynr::types::Version providerVersion2(23, 12);
    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry2(providerVersion2,
                                                             _domain,
                                                             _interfaceName,
                                                             participantId,
                                                             providerQos,
                                                             _lastSeenDateMs,
                                                             _expiryDateMs,
                                                             _publicKeyId,
                                                             true);

    auto mockFuture1 = std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
    mockFuture1->onSuccess(discoveryEntry1);
    auto mockFuture2 = std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
    mockFuture2->onSuccess(discoveryEntry2);
    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(_, _, _, _, _, _, _))
            .WillOnce(Return(mockFuture1))
            .WillRepeatedly(Return(mockFuture2));

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersion2);

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString(); };

    auto onError = [this, &expectedVersions](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException, noCompatibleProviderFoundException(expectedVersions));
        _semaphore.notify();
    };

    fixedParticipantArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));
    fixedParticipantArbitrator->stopArbitration();
}

// Test that the lastSeenArbitrator reports a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getDefaultReturnsNoCompatibleProviderFoundException)
{
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    auto lastSeenArbitrator =
            std::make_shared<Arbitrator>(_domain,
                                         _interfaceName,
                                         expectedVersion,
                                         _mockDiscovery,
                                         discoveryQos,
                                         _emptyGbidsVector,
                                         move(_lastSeenArbitrationStrategyFunction));

    // Create a list of discovery entries
    types::ProviderQos providerQos(
            std::vector<types::CustomParameter>(), // custom provider parameters
            42,                                    // priority
            joynr::types::ProviderScope::GLOBAL,   // discovery scope
            false                                  // supports on change notifications
            );
    // discovery entries for first lookup
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(
                joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                         _domain,
                                                         _interfaceName,
                                                         std::to_string(participantIdCounter),
                                                         providerQos,
                                                         _lastSeenDateMs,
                                                         _expiryDateMs,
                                                         _publicKeyId,
                                                         true));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(
                joynr::types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                         _domain,
                                                         _interfaceName,
                                                         std::to_string(participantIdCounter),
                                                         providerQos,
                                                         _lastSeenDateMs,
                                                         _expiryDateMs,
                                                         _publicKeyId,
                                                         true));
        participantIdCounter++;
    }

    auto mockFuture1 = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture1->onSuccess(discoveryEntries1);
    auto mockFuture2 = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture2->onSuccess(discoveryEntries2);
    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
            .WillOnce(Return(mockFuture1))
            .WillRepeatedly(Return(mockFuture2));

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };

    auto onError = [this, &expectedVersions](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException, noCompatibleProviderFoundException(expectedVersions));
        _semaphore.notify();
    };

    lastSeenArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));
    lastSeenArbitrator->stopArbitration();
}

TEST_F(ArbitratorTest, discoveryException_discoveryErrorFromDiscoveryProxy_noEntryForParticipant_retries)
{
    const types::DiscoveryError::Enum& error = types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT;
    const std::int64_t discoveryTimeoutMs = 199;
    const std::int64_t retryIntervalMs = 100;
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(retryIntervalMs);
    const std::string participantId = "unittests-participantId";
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);

    const std::string expectedErrorMessage = getErrorMsgUnableToLookup(error, _gbids, participantId);

    auto exception = std::make_shared<exceptions::ApplicationException>(
                "no entry found",
                std::make_shared<types::DiscoveryError>(
                    types::DiscoveryError::getLiteral(error)));
    auto mockFutureFixedPartId =
            std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
    mockFutureFixedPartId->onError(exception);

    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                    _, // participantId
                    _, // discoveryQos
                    _, // gbids
                    _, // onSuccess
                    _, // onApplicationError
                    _, // onRuntimeError
                    _)) // qos
            .Times(2) // 1 retry
            .WillRepeatedly(Return(mockFutureFixedPartId));

    joynr::types::Version version;
    auto arbitrator = ArbitratorFactory::createArbitrator(
            _domain, _interfaceName, version, _mockDiscovery, discoveryQos, _gbids);

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };
    auto onError = [this, &expectedErrorMessage](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException,
                    joynrException(joynr::exceptions::DiscoveryException::TYPE_NAME(),
                                   expectedErrorMessage));
        _semaphore.notify();
    };

    auto start = std::chrono::system_clock::now();

    arbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

    JOYNR_LOG_DEBUG(logger(), "Time elapsed for unsuccessful arbitration : {}", elapsed.count());
    ASSERT_LT(elapsed.count(), discoveryTimeoutMs);
    ASSERT_GE(elapsed.count(), retryIntervalMs);
    arbitrator->stopArbitration();
}

TEST_F(ArbitratorTest, discoveryException_discoveryErrorFromDiscoveryProxy_noEntryForSelectedBackends_retries)
{
    const types::DiscoveryError::Enum& error = types::DiscoveryError::NO_ENTRY_FOR_SELECTED_BACKENDS;
    const std::int64_t discoveryTimeoutMs = 199;
    const std::int64_t retryIntervalMs = 100;
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(retryIntervalMs);

    const std::string expectedErrorMessage = getErrorMsgUnableToLookup(error, _gbids);

    auto exception = std::make_shared<exceptions::ApplicationException>(
                "no entry found",
                std::make_shared<types::DiscoveryError>(
                    types::DiscoveryError::getLiteral(error)));
    auto mockFuture =
            std::make_shared<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onError(exception);

    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                    _, // domains
                    _, // interfaceName
                    _, // discoveryQos
                    _, // gbids
                    _, // onSuccess
                    _, // onApplicationError
                    _, // onRuntimeError
                    _)) // qos
            .Times(2) // 1 retry
            .WillRepeatedly(Return(mockFuture));

    joynr::types::Version version;
    auto arbitrator = ArbitratorFactory::createArbitrator(
            _domain, _interfaceName, version, _mockDiscovery, discoveryQos, _gbids);

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };
    auto onError = [this, &expectedErrorMessage](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException,
                    joynrException(joynr::exceptions::DiscoveryException::TYPE_NAME(),
                                   expectedErrorMessage));
        _semaphore.notify();
    };

    auto start = std::chrono::system_clock::now();

    arbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

    JOYNR_LOG_DEBUG(logger(), "Time elapsed for unsuccessful arbitration : {}", elapsed.count());
    ASSERT_LT(elapsed.count(), discoveryTimeoutMs);
    ASSERT_GE(elapsed.count(), retryIntervalMs);
    arbitrator->stopArbitration();
}

/**
 * Test whether discovery entries are filtered by version and arbitartion strategy,
 * if filterByVersionAndArbitrationStartegy is disabled
*/
TEST_F(ArbitratorTest, filterByVersionAndArbitrationStrategy_disabled_versionAndStrategyNotChecked)
{
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(_defaultDiscoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(_defaultRetryIntervalMs);
    joynr::types::Version providerVersion(47, 11);

    auto arbitrator =
            std::make_shared<Arbitrator>(_domain,
                                         _interfaceName,
                                         providerVersion,
                                         _mockDiscovery,
                                         discoveryQos,
                                         _emptyGbidsVector,
                                         move(_lastSeenArbitrationStrategyFunction));

    types::ProviderQos providerQos(
            std::vector<types::CustomParameter>(), // custom provider parameters
            42,                                    // priority
            joynr::types::ProviderScope::GLOBAL,   // discovery scope
            false                                  // supports on change notifications
            );

    types::Version incorrectProviderVersion(providerVersion.getMajorVersion() - 1, providerVersion.getMinorVersion());
    types::DiscoveryEntryWithMetaInfo discoveryEntry1 = types::DiscoveryEntryWithMetaInfo(incorrectProviderVersion,
                                                                                         _domain,
                                                                                         _interfaceName,
                                                                                         "participantId1",
                                                                                         providerQos,
                                                                                         _lastSeenDateMs,
                                                                                         _expiryDateMs,
                                                                                         _publicKeyId,
                                                                                         true);

    types::DiscoveryEntryWithMetaInfo discoveryEntry2 = types::DiscoveryEntryWithMetaInfo(providerVersion,
                                                                                         _domain,
                                                                                         _interfaceName,
                                                                                         "participantId2",
                                                                                         providerQos,
                                                                                         _lastSeenDateMs,
                                                                                         _expiryDateMs,
                                                                                         _publicKeyId,
                                                                                         true);

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries {discoveryEntry1, discoveryEntry2};

    auto mockFuture = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess(discoveryEntries);
    ON_CALL(*_mockDiscovery,
            lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
        .WillByDefault(Return(mockFuture));

    // Check that the correct participant was selected
    auto onSuccess = [this, &discoveryEntries, &discoveryEntry1, &discoveryEntry2](
            const ArbitrationResult& arbitrationResult) {
        EXPECT_EQ(arbitrationResult.getDiscoveryEntries().size(), 2);
        bool foundDiscoveryEntry1 = false;
        bool foundDiscoveryEntry2 = false;
        for (const auto& entry : discoveryEntries) {
            if (entry.getParticipantId() == discoveryEntry1.getParticipantId()) {
                foundDiscoveryEntry1 = true;
            }
            if (entry.getParticipantId() == discoveryEntry2.getParticipantId()) {
                foundDiscoveryEntry2 = true;
            }
        }
        EXPECT_TRUE(foundDiscoveryEntry1);
        EXPECT_TRUE(foundDiscoveryEntry2);
        _semaphore.notify();
    };

    auto onError = [](const exceptions::DiscoveryException& e) { FAIL() << "Got exception: " << e.getMessage(); };

    arbitrator->startArbitration(onSuccess, onError, false);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs())));
    arbitrator->stopArbitration();
}

TEST_F(ArbitratorTest, arbitrationStarted_localDiscoveryAggregatorLookupCalledWithCorrectDiscoveryTimeout_fixedParticipantId)
{
    const types::DiscoveryError::Enum& error = types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT;
    constexpr std::int64_t discoveryTimeoutMs = 199;
    constexpr std::int64_t retryIntervalMs = 100;
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(retryIntervalMs);
    const std::string participantId = "unittests-participantId";
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);

    const std::string expectedErrorMessage = getErrorMsgUnableToLookup(error, _gbids, participantId);

    auto exception = std::make_unique<exceptions::ApplicationException>(
                "no entry found",
                std::make_shared<types::DiscoveryError>(
                    types::DiscoveryError::getLiteral(error)));
    auto mockFutureFixedPartId =
            std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
    mockFutureFixedPartId->onError(std::move(exception));

    joynr::types::DiscoveryQos capturedDiscoveryQosStarted;
    joynr::types::DiscoveryQos capturedDiscoveryQosRetried;
    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                    _, // participantId
                    _, // discoveryQos
                    _, // gbids
                    _, // onSuccess
                    _, // onApplicationError
                    _, // onRuntimeError
                    _)) // qos
            .WillOnce(DoAll(::testing::SaveArg<1>(&capturedDiscoveryQosStarted), Return(mockFutureFixedPartId)))
            .WillOnce(DoAll(::testing::SaveArg<1>(&capturedDiscoveryQosRetried), Return(mockFutureFixedPartId)));

    joynr::types::Version version;
    auto arbitrator = ArbitratorFactory::createArbitrator(
            _domain, _interfaceName, version, _mockDiscovery, discoveryQos, _gbids);

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };
    auto onError = [this, &expectedErrorMessage](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException,
                    joynrException(joynr::exceptions::DiscoveryException::TYPE_NAME(),
                                   expectedErrorMessage));
        _semaphore.notify();
    };

    arbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));

    const std::int64_t capturedGlobalRemainingTtlAtStart = capturedDiscoveryQosStarted.getDiscoveryTimeout();
    const std::int64_t capturedGlobalRemainingTtlAtRetry = capturedDiscoveryQosRetried.getDiscoveryTimeout();
    EXPECT_LE(capturedGlobalRemainingTtlAtStart, discoveryTimeoutMs);
    EXPECT_GT(capturedGlobalRemainingTtlAtStart, discoveryTimeoutMs - (retryIntervalMs / 2));
    EXPECT_LE(capturedGlobalRemainingTtlAtRetry, discoveryTimeoutMs - retryIntervalMs);
    EXPECT_GT(capturedGlobalRemainingTtlAtRetry, 0);
    arbitrator->stopArbitration();
}

TEST_F(ArbitratorTest, arbitrationStarted_localDiscoveryAggregatorLookupCalledWithCorrectMessagingQos_fixedParticipantId)
{
    const types::DiscoveryError::Enum& error = types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT;
    constexpr std::int64_t discoveryTimeoutMs = 199;
    constexpr std::int64_t retryIntervalMs = 100;
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(retryIntervalMs);
    const std::string participantId = "unittests-participantId";
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);

    const std::string expectedErrorMessage = getErrorMsgUnableToLookup(error, _gbids, participantId);

    auto exception = std::make_unique<exceptions::ApplicationException>(
                "no entry found",
                std::make_shared<types::DiscoveryError>(
                    types::DiscoveryError::getLiteral(error)));
    auto mockFutureFixedPartId =
            std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
    mockFutureFixedPartId->onError(std::move(exception));

    boost::optional<joynr::MessagingQos> capturedMessagingQosStarted;
    boost::optional<joynr::MessagingQos> capturedMessagingQosRetried;
    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                    _, // participantId
                    _, // discoveryQos
                    _, // gbids
                    _, // onSuccess
                    _, // onApplicationError
                    _, // onRuntimeError
                    _)) // qos
            .WillOnce(DoAll(::testing::SaveArg<6>(&capturedMessagingQosStarted), Return(mockFutureFixedPartId)))
            .WillOnce(DoAll(::testing::SaveArg<6>(&capturedMessagingQosRetried), Return(mockFutureFixedPartId)));

    joynr::types::Version version;
    auto arbitrator = ArbitratorFactory::createArbitrator(
            _domain, _interfaceName, version, _mockDiscovery, discoveryQos, _gbids);

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };
    auto onError = [this, &expectedErrorMessage](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException,
                    joynrException(joynr::exceptions::DiscoveryException::TYPE_NAME(),
                                   expectedErrorMessage));
        _semaphore.notify();
    };

    arbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));

    constexpr std::uint64_t epsilon = 10000;
    ASSERT_TRUE(capturedMessagingQosStarted);
    ASSERT_TRUE(capturedMessagingQosRetried);
    const std::uint64_t capturedMessagingQosTtlAtStart = capturedMessagingQosStarted.value().getTtl();
    const std::uint64_t capturedMessagingQosTtlAtRetry = capturedMessagingQosRetried.value().getTtl();
    EXPECT_LE(capturedMessagingQosTtlAtStart, discoveryTimeoutMs + epsilon);
    EXPECT_GT(capturedMessagingQosTtlAtStart, discoveryTimeoutMs - (retryIntervalMs / 2) + epsilon);
    EXPECT_LE(capturedMessagingQosTtlAtRetry, discoveryTimeoutMs - retryIntervalMs + epsilon);
    EXPECT_GT(capturedMessagingQosTtlAtRetry, epsilon);
    arbitrator->stopArbitration();
}

TEST_F(ArbitratorTest, arbitrationStarted_localDiscoveryAggregatorLookupCalledWithCorrectDiscoveryTimeout)
{
    const types::DiscoveryError::Enum& error = types::DiscoveryError::NO_ENTRY_FOR_SELECTED_BACKENDS;
    constexpr std::int64_t discoveryTimeoutMs = 199;
    constexpr std::int64_t retryIntervalMs = 100;
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(retryIntervalMs);

    const std::string expectedErrorMessage = getErrorMsgUnableToLookup(error, _gbids);
    auto exception = std::make_unique<exceptions::ApplicationException>(
                "no entry found",
                std::make_shared<types::DiscoveryError>(
                    types::DiscoveryError::getLiteral(error)));
    auto mockFuture = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onError(std::move(exception));

    joynr::types::DiscoveryQos capturedDiscoveryQosStarted;
    joynr::types::DiscoveryQos capturedDiscoveryQosRetried;
    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                    _, // domains
                    _, // interfaceName
                    _, // discoveryQos
                    _, // gbids
                    _, // onSuccess
                    _, // onApplicationError
                    _, // onRuntimeError
                    _)) // qos
            .WillOnce(DoAll(::testing::SaveArg<2>(&capturedDiscoveryQosStarted), Return(mockFuture)))
            .WillOnce(DoAll(::testing::SaveArg<2>(&capturedDiscoveryQosRetried), Return(mockFuture)));

    joynr::types::Version version;
    auto arbitrator = ArbitratorFactory::createArbitrator(
            _domain, _interfaceName, version, _mockDiscovery, discoveryQos, _gbids);

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };
    auto onError = [this, &expectedErrorMessage](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException,
                    joynrException(joynr::exceptions::DiscoveryException::TYPE_NAME(),
                                   expectedErrorMessage));
        _semaphore.notify();
    };

    arbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));

    const std::int64_t capturedGlobalRemainingTtlAtStart = capturedDiscoveryQosStarted.getDiscoveryTimeout();
    const std::int64_t capturedGlobalRemainingTtlAtRetry = capturedDiscoveryQosRetried.getDiscoveryTimeout();
    EXPECT_LE(capturedGlobalRemainingTtlAtStart, discoveryTimeoutMs);
    EXPECT_GT(capturedGlobalRemainingTtlAtStart, discoveryTimeoutMs - (retryIntervalMs / 2));
    EXPECT_LE(capturedGlobalRemainingTtlAtRetry, discoveryTimeoutMs - retryIntervalMs);
    EXPECT_GT(capturedGlobalRemainingTtlAtRetry, 0);
    arbitrator->stopArbitration();
}

TEST_F(ArbitratorTest, arbitrationStarted_localDiscoveryAggregatorLookupCalledWithCorrectMessagingQos)
{
    const types::DiscoveryError::Enum& error = types::DiscoveryError::NO_ENTRY_FOR_SELECTED_BACKENDS;
    constexpr std::int64_t discoveryTimeoutMs = 199;
    constexpr std::int64_t retryIntervalMs = 100;
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(retryIntervalMs);

    const std::string expectedErrorMessage = getErrorMsgUnableToLookup(error, _gbids);
    auto exception = std::make_unique<exceptions::ApplicationException>(
                "no entry found",
                std::make_shared<types::DiscoveryError>(
                    types::DiscoveryError::getLiteral(error)));
    auto mockFuture = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onError(std::move(exception));

    boost::optional<joynr::MessagingQos> capturedMessagingQosStarted;
    boost::optional<joynr::MessagingQos> capturedMessagingQosRetried;
    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                    _, // domains
                    _, // interfaceName
                    _, // discoveryQos
                    _, // gbids
                    _, // onSuccess
                    _, // onApplicationError
                    _, // onRuntimeError
                    _)) // qos
            .WillOnce(DoAll(::testing::SaveArg<7>(&capturedMessagingQosStarted), Return(mockFuture)))
            .WillOnce(DoAll(::testing::SaveArg<7>(&capturedMessagingQosRetried), Return(mockFuture)));

    joynr::types::Version version;
    auto arbitrator = ArbitratorFactory::createArbitrator(
            _domain, _interfaceName, version, _mockDiscovery, discoveryQos, _gbids);

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };
    auto onError = [this, &expectedErrorMessage](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException,
                    joynrException(joynr::exceptions::DiscoveryException::TYPE_NAME(),
                                   expectedErrorMessage));
        _semaphore.notify();
    };

    arbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));

    constexpr std::uint64_t epsilon = 10000;
    ASSERT_TRUE(capturedMessagingQosStarted);
    ASSERT_TRUE(capturedMessagingQosRetried);
    const std::uint64_t capturedMessagingQosTtlAtStart = capturedMessagingQosStarted.value().getTtl();
    const std::uint64_t capturedMessagingQosTtlAtRetry = capturedMessagingQosRetried.value().getTtl();
    EXPECT_LE(capturedMessagingQosTtlAtStart, discoveryTimeoutMs + epsilon);
    EXPECT_GE(capturedMessagingQosTtlAtStart, discoveryTimeoutMs - (retryIntervalMs / 2) + epsilon);
    EXPECT_LE(capturedMessagingQosTtlAtRetry, discoveryTimeoutMs - retryIntervalMs + epsilon);
    EXPECT_GE(capturedMessagingQosTtlAtRetry, epsilon);
    arbitrator->stopArbitration();
}

/*
 * Tests that the arbitrators report the exception from the discoveryProxy if the lookup fails
 * during the last retry
 */
class ArbitratorTestWithParams
        : public ArbitratorTest,
          public ::testing::WithParamInterface<DiscoveryQos::ArbitrationStrategy>
{
public:
    ArbitratorTestWithParams() : arbitrationStrategy(GetParam())
    {
    }

protected:
    void testCallsDiscoveryProxyCorrectly(const std::vector<std::string>& gbids,
                                          const std::vector<std::string>& expectedGbids) {
        const std::vector<std::string> expectedDomains {_domain};
        constexpr std::int64_t discoveryTimeoutMs = 5000;
        constexpr std::int64_t retryIntervalMs = 6000; // no retry
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(this->arbitrationStrategy);
        discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
        discoveryQos.setRetryIntervalMs(retryIntervalMs);

        joynr::types::DiscoveryQos capturedDiscoveryQos;
        if (this->arbitrationStrategy == DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT) {
            const std::string participantId = "unittests-participantId";
            discoveryQos.addCustomParameter("fixedParticipantId", participantId);

            auto mockFutureFixedPartId =
                    std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
            mockFutureFixedPartId->onError(
                    std::make_shared<exceptions::JoynrRuntimeException>("exception"));

            EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                            Eq(participantId), // participantId
                            _, // discoveryQos
                            Eq(expectedGbids), // gbids
                            _, // onSuccess
                            _, // onApplicationError
                            _, // onRuntimeError
                            _)) // qos
                    .WillOnce(DoAll(::testing::SaveArg<1>(&capturedDiscoveryQos), Return(mockFutureFixedPartId)));
        } else {
            if (this->arbitrationStrategy == DiscoveryQos::ArbitrationStrategy::KEYWORD) {
                discoveryQos.addCustomParameter("keyword", "keywordValue");
            }
            auto mockFuture = std::make_shared<
                    joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
            mockFuture->onError(
                    std::make_shared<exceptions::JoynrRuntimeException>("exception"));

            EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                            Eq(expectedDomains), // domains
                            Eq(_interfaceName), // interfaceName
                            _, // discoveryQos
                            Eq(expectedGbids), // gbids
                            _, // onSuccess
                            _, // onApplicationError
                            _, // onRuntimeError
                            _)) // qos
                    .WillOnce(DoAll(::testing::SaveArg<2>(&capturedDiscoveryQos), Return(mockFuture)));
        }

        joynr::types::Version version;
        auto arbitrator = ArbitratorFactory::createArbitrator(
                _domain, _interfaceName, version, _mockDiscovery, discoveryQos, gbids);

        auto onSuccess = [this](const ArbitrationResult&) {
            _semaphore.notify();
        };
        auto onError = [this](const exceptions::DiscoveryException&) {
            _semaphore.notify();
        };

        arbitrator->startArbitration(onSuccess, onError);
        EXPECT_TRUE(_semaphore.waitFor(
                std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));

        const auto capturedDiscoveryTimeout = capturedDiscoveryQos.getDiscoveryTimeout();
        const auto capturedCacheMaxAge = capturedDiscoveryQos.getCacheMaxAge();
        const auto capturedDiscoveryScope = capturedDiscoveryQos.getDiscoveryScope();
        const auto capturedProviderMustSupportOnChange = capturedDiscoveryQos.getProviderMustSupportOnChange();
        EXPECT_LE(capturedDiscoveryTimeout, discoveryTimeoutMs);
        EXPECT_GT(capturedDiscoveryTimeout, discoveryTimeoutMs / 2 );
        EXPECT_EQ(capturedCacheMaxAge, discoveryQos.getCacheMaxAgeMs());
        EXPECT_EQ(capturedDiscoveryScope, discoveryQos.getDiscoveryScope());
        EXPECT_EQ(capturedProviderMustSupportOnChange, discoveryQos.getProviderMustSupportOnChange());

        arbitrator->stopArbitration();
    }

    void testDiscoveryErrorFromDiscoveryProxy_doesNotRetry(const types::DiscoveryError::Enum& error)
    {
        const std::int64_t retryIntervalMs = 100;
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(this->arbitrationStrategy);
        discoveryQos.setDiscoveryTimeoutMs(199);
        discoveryQos.setRetryIntervalMs(retryIntervalMs);

        auto exception = std::make_shared<exceptions::ApplicationException>(
                    "no entry found",
                    std::make_shared<types::DiscoveryError>(
                        types::DiscoveryError::getLiteral(error)));
        std::string expectedErrorMessage;
        if (this->arbitrationStrategy == DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT) {
            const std::string participantId = "unittests-participantId";
            discoveryQos.addCustomParameter("fixedParticipantId", participantId);
            expectedErrorMessage = getErrorMsgUnableToLookup(error, _gbids, participantId);

            auto mockFutureFixedPartId =
                    std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
            mockFutureFixedPartId->onError(exception);

            EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                            _, // participantId
                            _, // discoveryQos
                            _, // gbids
                            _, // onSuccess
                            _, // onApplicationError
                            _, // onRuntimeError
                            _)) // qos
                    .WillOnce(Return(mockFutureFixedPartId));
        } else {
            if (this->arbitrationStrategy == DiscoveryQos::ArbitrationStrategy::KEYWORD) {
                discoveryQos.addCustomParameter("keyword", "keywordValue");
            }
            expectedErrorMessage = getErrorMsgUnableToLookup(error, _gbids);
            auto mockFuture = std::make_shared<
                    joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
            mockFuture->onError(exception);

            EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                            _, // domains
                            _, // interfaceName
                            _, // discoveryQos
                            _, // gbids
                            _, // onSuccess
                            _, // onApplicationError
                            _, // onRuntimeError
                            _)) // qos
                    .WillOnce(Return(mockFuture));
        }

        joynr::types::Version version;
        auto arbitrator = ArbitratorFactory::createArbitrator(
                _domain, _interfaceName, version, _mockDiscovery, discoveryQos, _gbids);

        auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
            types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
            FAIL() << "Got result: " << result.toString();
        };
        auto onError = [this, &expectedErrorMessage](const exceptions::DiscoveryException& discoveryException) {
            EXPECT_THAT(discoveryException,
                        joynrException(joynr::exceptions::DiscoveryException::TYPE_NAME(),
                                       expectedErrorMessage));
            _semaphore.notify();
        };

        auto start = std::chrono::system_clock::now();

        arbitrator->startArbitration(onSuccess, onError);
        EXPECT_TRUE(_semaphore.waitFor(
                std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));

        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

        JOYNR_LOG_DEBUG(logger(), "Time elapsed for unsuccessful arbitration : {}", elapsed.count());
        ASSERT_LT(elapsed.count(), retryIntervalMs);
        arbitrator->stopArbitration();
    }

    const DiscoveryQos::ArbitrationStrategy arbitrationStrategy;
};

TEST_P(ArbitratorTestWithParams, callsDiscoveryProxyCorrectly_withGbids)
{
    const std::vector<std::string> expectedGbids = _gbids;

    testCallsDiscoveryProxyCorrectly(_gbids, expectedGbids);
}

TEST_P(ArbitratorTestWithParams, callsDiscoveryProxyCorrectly_withoutGbids)
{
    const std::vector<std::string> gbids {};
    const std::vector<std::string> expectedGbids = gbids;

    testCallsDiscoveryProxyCorrectly(gbids, expectedGbids);
}

TEST_P(ArbitratorTestWithParams, discoveryException_discoveryErrorFromDiscoveryProxy_unknownGbid_doesNotretry)
{
    testDiscoveryErrorFromDiscoveryProxy_doesNotRetry(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_P(ArbitratorTestWithParams, discoveryException_discoveryErrorFromDiscoveryProxy_invalidGbid_doesNotretry)
{
    testDiscoveryErrorFromDiscoveryProxy_doesNotRetry(types::DiscoveryError::INVALID_GBID);
}

TEST_P(ArbitratorTestWithParams, discoveryException_discoveryErrorFromDiscoveryProxy_internalError_doesNotretry)
{
    testDiscoveryErrorFromDiscoveryProxy_doesNotRetry(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_P(ArbitratorTestWithParams, discoveryException_exceptionFromDiscoveryProxy)
{
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(this->arbitrationStrategy);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version version;

    const exceptions::JoynrRuntimeException exception("first exception");
    const exceptions::JoynrRuntimeException expectedException("expected exception");
    std::string expectedExceptionMsg;

    if (this->arbitrationStrategy == DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT) {
        const std::string participantId = "unittests-participantId";
        expectedExceptionMsg = getExceptionMsgUnableToLookup(expectedException, _emptyGbidsVector, participantId);
        discoveryQos.addCustomParameter("fixedParticipantId", participantId);

        auto mockFutureFixedPartId1 =
                std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
        mockFutureFixedPartId1->onError(
                std::make_shared<exceptions::JoynrRuntimeException>(exception));
        auto mockFutureFixedPartId2 =
                std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
        mockFutureFixedPartId2->onError(
                std::make_shared<exceptions::JoynrRuntimeException>(expectedException));

        EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(_, _, _, _, _, _, _))
                .WillOnce(Return(mockFutureFixedPartId1))
                .WillRepeatedly(Return(mockFutureFixedPartId2));
    } else {
        expectedExceptionMsg = getExceptionMsgUnableToLookup(expectedException, _emptyGbidsVector);
        if (this->arbitrationStrategy == DiscoveryQos::ArbitrationStrategy::KEYWORD) {
            discoveryQos.addCustomParameter("keyword", "keywordValue");
        }

        auto mockFuture1 = std::make_shared<
                joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
        mockFuture1->onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
        auto mockFuture2 = std::make_shared<
                joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
        mockFuture2->onError(
                std::make_shared<exceptions::JoynrRuntimeException>(expectedException));

        EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
                .WillOnce(Return(mockFuture1))
                .WillRepeatedly(Return(mockFuture2));
    }

    auto arbitrator = ArbitratorFactory::createArbitrator(
            _domain, _interfaceName, version, _mockDiscovery, discoveryQos, _emptyGbidsVector);

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };

    auto onError = [this, &expectedExceptionMsg](const exceptions::DiscoveryException& discoveryException) {
        EXPECT_THAT(discoveryException,
                    joynrException(exceptions::DiscoveryException::TYPE_NAME(), expectedExceptionMsg));
        _semaphore.notify();
    };

    arbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));
    arbitrator->stopArbitration();
}


/*
 * Tests that the arbitrators report an exception if no entries were found during the last retry
 */
TEST_P(ArbitratorTestWithParams, discoveryException_emptyResult)
{
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(this->arbitrationStrategy);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);

    // discovery entries for first lookup
    types::ProviderQos providerQos(
            std::vector<types::CustomParameter>(), // custom provider parameters
            42,                                    // priority
            joynr::types::ProviderScope::GLOBAL,   // discovery scope
            false                                  // supports on change notifications
            );
    joynr::types::Version providerVersion(22, 23);
    types::DiscoveryEntryWithMetaInfo discoveryEntry1(providerVersion,
                                                      _domain,
                                                      _interfaceName,
                                                      "testParticipantId",
                                                      providerQos,
                                                      _lastSeenDateMs,
                                                      _expiryDateMs,
                                                      _publicKeyId,
                                                      false);

    std::function<void(const exceptions::DiscoveryException&)> onError;

    if (this->arbitrationStrategy == DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT) {
        const std::string fixedParticipantId("fixedParticipantId");
        discoveryQos.addCustomParameter("fixedParticipantId", fixedParticipantId);
        discoveryEntry1.setParticipantId(fixedParticipantId);

        auto mockFuture1 = std::make_shared<
                joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
        mockFuture1->onSuccess(discoveryEntry1);
        auto mockFuture2 = std::make_shared<
                joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
        auto exception = std::make_shared<exceptions::ApplicationException>(
                    "no entry found",
                    std::make_shared<types::DiscoveryError>(
                        types::DiscoveryError::getLiteral(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT)));
        mockFuture2->onError(exception);
        EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                        Matcher<const std::string&>(_), _, _, _, _, _, _))
                .WillOnce(Return(mockFuture1)) // return matching entry with incompatible version
                .WillRepeatedly(Return(mockFuture2)); // NO_ENTRY_FOR_PARTICIPANT

        onError = [this, fixedParticipantId](const exceptions::DiscoveryException& discoveryException) {
                EXPECT_THAT(discoveryException,
                            joynrException(exceptions::DiscoveryException::TYPE_NAME(),
                                           getErrorMsgUnableToLookup(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT,
                                                                     _emptyGbidsVector,
                                                                     fixedParticipantId)));
                _semaphore.notify();
            };
    } else {
        if (this->arbitrationStrategy == DiscoveryQos::ArbitrationStrategy::KEYWORD) {
            // Search for this keyword value
            const std::string keywordValue("unittests-keyword");
            discoveryQos.addCustomParameter("keyword", keywordValue);
            providerQos.setCustomParameters({types::CustomParameter("keyword", keywordValue)});
        }

        std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries1;
        discoveryEntries1.push_back(discoveryEntry1);

        // discoveryEntries for subsequent lookups
        std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries2;

        auto mockFuture1 = std::make_shared<
                joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
        mockFuture1->onSuccess(discoveryEntries1);
        auto mockFuture2 = std::make_shared<
                joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
        mockFuture2->onSuccess(discoveryEntries2);
        EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(
                        Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
                .WillOnce(Return(mockFuture1)) // return matching entry with incompatible version
                .WillRepeatedly(Return(mockFuture2)); // return empty result

        onError = [this](const exceptions::DiscoveryException& discoveryException) {
                EXPECT_THAT(discoveryException,
                            joynrException(exceptions::DiscoveryException::TYPE_NAME(),
                                           _exceptionMsgNoEntriesFound));
                _semaphore.notify();
        };
    }

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };

    auto arbitrator = std::make_shared<Arbitrator>(_domain,
                                                   _interfaceName,
                                                   expectedVersion,
                                                   _mockDiscovery,
                                                   discoveryQos,
                                                   _emptyGbidsVector,
                                                   move(_qosArbitrationStrategyFunction));

    arbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(_semaphore.waitFor(
            std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs() * 10)));
    arbitrator->stopArbitration();
}

INSTANTIATE_TEST_SUITE_P(changeArbitrationStrategy,
                        ArbitratorTestWithParams,
                        ::testing::Values(DiscoveryQos::ArbitrationStrategy::LAST_SEEN,
                                          DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT,
                                          DiscoveryQos::ArbitrationStrategy::KEYWORD,
                                          DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY));

template <int Duration>
void letThreadSleep()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(Duration));
}

void ArbitratorTest::testArbitrationStopsOnShutdown(bool testRetry)
{
    const std::string interfaceName("interfaceName");
    types::Version providerVersion;
    std::int64_t discoveryTimeoutMs = std::chrono::milliseconds(2000).count();

    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(800);

    auto mockFuture = std::make_shared<
            joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>();
    mockFuture->onSuccess({});

    if (testRetry) {
        EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
                .WillOnce(DoAll(ReleaseSemaphore(&_semaphore), Return(mockFuture)));
    } else {
        EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(Matcher<const std::vector<std::string>&>(_), _, _, _, _, _, _, _))
                .WillOnce(DoAll(ReleaseSemaphore(&_semaphore),
                                InvokeWithoutArgs(letThreadSleep<500>),
                                Return(mockFuture)));
    }

    auto arbitrator =
            std::make_shared<joynr::Arbitrator>("domain",
                                                interfaceName,
                                                providerVersion,
                                                _mockDiscovery,
                                                discoveryQos,
                                                _emptyGbidsVector,
                                                move(_lastSeenArbitrationStrategyFunction));

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };

    joynr::Semaphore onErrorSemaphore;
    auto onError = [&onErrorSemaphore, interfaceName](const exceptions::DiscoveryException& error) {
        const std::string expectedErrorMsg("Shutting Down Arbitration for interface " +
                                           interfaceName);
        EXPECT_EQ(expectedErrorMsg, error.getMessage());
        onErrorSemaphore.notify();
    };

    arbitrator->startArbitration(onSuccess, onError);

    // Wait for a time shorter than the discovery
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(100)));

    if (testRetry) {
        // wait some time shorter than retryIntervalMs
        // to assure semaphore.waitFor(retryIntervalMs) is called in arbitrator.startArbitration
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // stop arbitration before the discovery thread wakes up
    arbitrator->stopArbitration();

    // onError should be invoked
    EXPECT_TRUE(onErrorSemaphore.waitFor(std::chrono::milliseconds(100)));
}

TEST_F(ArbitratorTest, arbitrationStopsOnShutdown_noRetry)
{
    const bool testRetry(false);
    testArbitrationStopsOnShutdown(testRetry);
}

TEST_F(ArbitratorTest, arbitrationStopsOnShutdown_withRetry)
{
    const bool testRetry(true);
    testArbitrationStopsOnShutdown(testRetry);
}

TEST_F(ArbitratorTest, discoveryExceptionOnIncompatibleInterface)
{
    const std::string participantId("unittests-participantId");
    const std::string wrongInterfaceName("wrongInterfaceName");

    // build a generic arbitrator with FIXED_PARTICIPANT strategy
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    joynr::types::Version expectedVersion(47, 11);
    auto fixedParticipantArbitrator =
            std::make_shared<Arbitrator>(_domain,
                                         _interfaceName,
                                         expectedVersion,
                                         _mockDiscovery,
                                         discoveryQos,
                                         _emptyGbidsVector,
                                         std::move(_fixedParticipantArbitrationStrategyFunction));

    // generate DiscoveryEntry containing a wrong interfaceName
    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry(expectedVersion,
                                                            _domain,
                                                            wrongInterfaceName,
                                                            participantId,
                                                            types::ProviderQos{},
                                                            _lastSeenDateMs,
                                                            _expiryDateMs,
                                                            _publicKeyId,
                                                            true);

    auto mockFuture = std::make_shared<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>();
    mockFuture->onSuccess(discoveryEntry);

    // When this is called a second time, the test is aborted with a SEGV
    // because GMOCK will return an invalid future
    EXPECT_CALL(*_mockDiscovery, lookupAsyncMock(Matcher<const std::string&>(_), _, _, _, _, _, _))
            .WillOnce(Return(mockFuture));

    auto onSuccess = [](const ArbitrationResult& arbitrationResult) {
        types::DiscoveryEntryWithMetaInfo result = arbitrationResult.getDiscoveryEntries().front();
        FAIL() << "Got result: " << result.toString();
    };

    auto onError = [this, &participantId, &wrongInterfaceName](
                           const exceptions::DiscoveryException& error) {
        const std::string expectedErrorMsg(
                "Unable to lookup provider (participantId: " + participantId +
                ") from discovery. JoynrException: incompatible interface returned, "
                "expected: " +
                _interfaceName + " actual: " + wrongInterfaceName);
        EXPECT_EQ(expectedErrorMsg, error.getMessage());
        _semaphore.notify();
    };

    fixedParticipantArbitrator->startArbitration(onSuccess, onError);
    EXPECT_TRUE(
            _semaphore.waitFor(std::chrono::milliseconds(discoveryQos.getDiscoveryTimeoutMs())));
    fixedParticipantArbitrator->stopArbitration();
}
