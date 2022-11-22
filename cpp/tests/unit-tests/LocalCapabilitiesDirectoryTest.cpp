/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "../../libjoynrclustercontroller/access-control/AccessController.h"
#include "../../libjoynrclustercontroller/access-control/LocalDomainAccessController.h"
#include "../../libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

#include "joynr/CallContext.h"
#include "joynr/CallContextStorage.h"
#include "joynr/CapabilitiesStorage.h"
#include "joynr/ClusterControllerDirectories.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/LCDUtil.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/DiscoveryScope.h"
#include "joynr/types/Version.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockAccessController.h"
#include "tests/mock/MockCallback.h"
#include "tests/mock/MockCapabilitiesStorage.h"
#include "tests/mock/MockGlobalCapabilitiesDirectoryClient.h"
#include "tests/mock/MockLocalCapabilitiesDirectoryStore.h"
#include "tests/mock/MockMessageRouter.h"
#include "tests/utils/PtrUtils.h"

// using ::testing::After;
using ::testing::InSequence;
using ::testing::Matcher;
using ::testing::Sequence;
using namespace ::testing;
using namespace joynr;

MATCHER_P2(pointerToAddressWithSerializedAddress, addressType, serializedAddress, "")
{
    if (arg == nullptr) {
        return false;
    }
    if (addressType == "mqtt") {
        auto mqttAddress = std::dynamic_pointer_cast<const system::RoutingTypes::MqttAddress>(arg);
        if (mqttAddress == nullptr) {
            return false;
        }
        return serializer::serializeToJson(*mqttAddress) == serializedAddress;
    }
    return false;
}

static constexpr std::int64_t conversionDelayToleranceMs = 2000;

bool compareDiscoveryEntries(const types::DiscoveryEntry& expected,
                             const types::DiscoveryEntry& actual)
{
    if (expected.getDomain() != actual.getDomain()) {
        std::cerr << "Expected domain: " << expected.getDomain()
                  << ", actual: " << actual.getDomain() << std::endl;
        return false;
    }
    if (expected.getInterfaceName() != actual.getInterfaceName()) {
        std::cerr << "Expected interfaceName: " << expected.getInterfaceName()
                  << ", actual: " << actual.getInterfaceName() << std::endl;
        return false;
    }
    if (expected.getParticipantId() != actual.getParticipantId()) {
        std::cerr << "Expected participantId: " << expected.getParticipantId()
                  << ", actual: " << actual.getParticipantId() << std::endl;
        return false;
    }
    if (expected.getQos() != actual.getQos()) {
        std::cerr << "Expected qos: " << expected.getQos().toString()
                  << ", actual: " << actual.getQos().toString() << std::endl;
        return false;
    }
    if (expected.getLastSeenDateMs() > actual.getLastSeenDateMs()) {
        std::cerr << "Expected lastSeenDateMs: >= " << expected.getLastSeenDateMs()
                  << ", actual: " << actual.getLastSeenDateMs() << std::endl;
        return false;
    }
    if (expected.getLastSeenDateMs() + conversionDelayToleranceMs < actual.getLastSeenDateMs()) {
        std::cerr << "Expected lastSeenDateMs: <= " << expected.getLastSeenDateMs() << " + "
                  << conversionDelayToleranceMs << ", actual: " << actual.getLastSeenDateMs()
                  << std::endl;
        return false;
    }
    if (expected.getProviderVersion() != actual.getProviderVersion()) {
        std::cerr << "Expected providerVersion: " << expected.getProviderVersion().toString()
                  << ", actual: " << actual.getProviderVersion().toString() << std::endl;
        return false;
    }
    if (expected.getExpiryDateMs() > actual.getExpiryDateMs()) {
        std::cerr << "Expected expiryDateMs: >= " << expected.getExpiryDateMs()
                  << ", actual: " << actual.getExpiryDateMs() << std::endl;
        return false;
    }
    if (expected.getExpiryDateMs() + conversionDelayToleranceMs < actual.getExpiryDateMs()) {
        std::cerr << "Expected expiryDateMs: <= " << expected.getExpiryDateMs() << " + "
                  << conversionDelayToleranceMs << ", actual: " << actual.getExpiryDateMs()
                  << std::endl;
        return false;
    }
    if (expected.getPublicKeyId() != actual.getPublicKeyId()) {
        std::cerr << "Expected publicKeyId: " << expected.getPublicKeyId()
                  << ", actual: " << actual.getPublicKeyId() << std::endl;
        return false;
    }
    return true;
}

MATCHER_P(DiscoveryEntryMatcher, other, "")
{
    return compareDiscoveryEntries(other, arg);
}

MATCHER_P(GlobalDiscoveryEntryMatcher, other, "")
{
    return compareDiscoveryEntries(other, arg) && other.getAddress() == arg.getAddress();
}

class LocalCapabilitiesDirectoryTest : public ::testing::Test
{
public:
    LocalCapabilitiesDirectoryTest()
            : _settings(),
              _settingsForPersistencyTests(),
              _clusterControllerSettings(_settings),
              _clusterControllerSettingsForPersistencyTests(_settingsForPersistencyTests),
              _purgeExpiredDiscoveryEntriesIntervalMs(3600000),
              _globalCapabilitiesDirectoryClient(
                      std::make_shared<MockGlobalCapabilitiesDirectoryClient>()),
              _localCapabilitiesDirectoryStore(std::make_shared<LocalCapabilitiesDirectoryStore>()),
              _localCapabilitiesDirectoryStoreForPersistencyTests(
                      std::make_shared<LocalCapabilitiesDirectoryStore>()),
              _mockLocallyRegisteredCapabilities(std::make_shared<capabilities::MockStorage>()),
              _mockGlobalLookupCache(std::make_shared<capabilities::MockCachingStorage>()),

              _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _mockMessageRouter(std::make_shared<MockMessageRouter>(
                      _singleThreadedIOService->getIOService())),
              _clusterControllerId("clusterControllerId"),
              _localCapabilitiesDirectory(),
              _localCapabilitiesDirectoryWithMockCapStorage(),
              _lastSeenDateMs(TimePoint::now().toMilliseconds()),
              _defaultExpiryIntervalMs(60 * 60 * 1000),
              _reAddInterval(3600000),
              _dummyParticipantIdsVector{
                      util::createUuid(), util::createUuid(), util::createUuid()},
              _defaultOnSuccess([]() {}),
              _defaultProviderRuntimeExceptionError(
                      [](const exceptions::ProviderRuntimeException&) {}),
              _unexpectedProviderRuntimeExceptionFunction(
                      [](const exceptions::ProviderRuntimeException& exception) {
                          FAIL() << "Got unexpected ProviderRuntimeException: " +
                                            exception.getMessage();
                      }),
              _unexpectedOnDiscoveryErrorFunction([](const types::DiscoveryError::Enum& errorEnum) {
                  FAIL() << "Unexpected onError call: " +
                                    types::DiscoveryError::getLiteral(errorEnum);
              }),
              _defaultProviderVersion(26, 05),
              _semaphore(std::make_shared<Semaphore>(0)),
              _finalized(false)
    {
        _singleThreadedIOService->start();
        _clusterControllerSettings.setPurgeExpiredDiscoveryEntriesIntervalMs(
                _purgeExpiredDiscoveryEntriesIntervalMs);
        // set the touch interval to 1 hour, so it does not interfere with tests
        // single test cases that need shorter interval will call
        // configureShortCapabilitiesFreshnessUpdateInterval() before building test
        // objects in finalizeTestSetupAfterMockExpectationsAreDone()
        _settings.set(
                ClusterControllerSettings::SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS(),
                3600000);
        _settings.set(ClusterControllerSettings::
                              SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED(),
                      false);
        _settingsForPersistencyTests.set(
                ClusterControllerSettings::
                        SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED(),
                true);

        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        _discoveryQos.setCacheMaxAge(10000);

        // init a capentry received from the global capabilities directory
        types::ProviderQos qos;
        qos.setScope(types::ProviderScope::GLOBAL);
        types::DiscoveryEntry globalCapEntry(_defaultProviderVersion,
                                             _DOMAIN_1_NAME,
                                             _INTERFACE_1_NAME,
                                             _dummyParticipantIdsVector[2],
                                             qos,
                                             _lastSeenDateMs,
                                             _lastSeenDateMs + 10000,
                                             _PUBLIC_KEY_ID);
        _globalCapEntryMap.insert({_EXTERNAL_ADDRESSES_VECTOR[0], globalCapEntry});
        _entry = types::DiscoveryEntry(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       types::ProviderQos(),
                                       TimePoint::now().toMilliseconds(),
                                       TimePoint::now().toMilliseconds() + _defaultExpiryIntervalMs,
                                       _PUBLIC_KEY_ID);
        _expectedGlobalCapEntry = types::GlobalDiscoveryEntry(_entry.getProviderVersion(),
                                                              _entry.getDomain(),
                                                              _entry.getInterfaceName(),
                                                              _entry.getParticipantId(),
                                                              _entry.getQos(),
                                                              _entry.getLastSeenDateMs(),
                                                              _entry.getExpiryDateMs(),
                                                              _entry.getPublicKeyId(),
                                                              _LOCAL_ADDRESS);
    }

    void initializeMockLocalCapabilitiesDirectoryStore()
    {
        _mockLocalCapabilitiesDirectoryStore =
                std::make_shared<MockLocalCapabilitiesDirectoryStore>(
                        _mockGlobalLookupCache, _mockLocallyRegisteredCapabilities);
    }

    void finalizeTestSetupAfterMockExpectationsAreDone()
    {
        if (_finalized) {
            std::cout << "finalizeTestSetupAfterMockExpectationsAreDone called twice, aborting"
                      << std::endl;
            std::abort();
        }
        _finalized = true;
        _localCapabilitiesDirectory = std::make_shared<LocalCapabilitiesDirectory>(
                _clusterControllerSettings,
                _globalCapabilitiesDirectoryClient,
                _localCapabilitiesDirectoryStore,
                _LOCAL_ADDRESS,
                _mockMessageRouter,
                _singleThreadedIOService->getIOService(),
                _clusterControllerId,
                _KNOWN_GBIDS,
                _defaultExpiryIntervalMs,
                _reAddInterval);
        _localCapabilitiesDirectory->init();
        _localCapabilitiesDirectoryWithMockCapStorage =
                std::make_shared<LocalCapabilitiesDirectory>(
                        _clusterControllerSettings,
                        _globalCapabilitiesDirectoryClient,
                        _mockLocalCapabilitiesDirectoryStore,
                        _LOCAL_ADDRESS,
                        _mockMessageRouter,
                        _singleThreadedIOService->getIOService(),
                        _clusterControllerId,
                        _KNOWN_GBIDS,
                        _defaultExpiryIntervalMs);
        _localCapabilitiesDirectoryWithMockCapStorage->init();
    }

    void configureShortCapabilitiesFreshnessUpdateInterval()
    {
        _settings.set(
                ClusterControllerSettings::SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS(),
                200);
    }

    void configureShortPurgeExpiredDiscoveryEntriesInterval()
    {
        _purgeExpiredDiscoveryEntriesIntervalMs = 1000;
        _clusterControllerSettings.setPurgeExpiredDiscoveryEntriesIntervalMs(
                _purgeExpiredDiscoveryEntriesIntervalMs);
    }

    ~LocalCapabilitiesDirectoryTest() override
    {
        _singleThreadedIOService->stop();
        test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectory);
        test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);

        test::util::removeFileInCurrentDirectory(".*\\.settings");
        test::util::removeFileInCurrentDirectory(".*\\.persist");
    }

    void fakeLookupNoEntryForParticipant(
            const std::string& participantId,
            const std::vector<std::string>& gbids,
            std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
    {
        std::ignore = participantId;
        std::ignore = gbids;
        std::ignore = messagingTtl;
        std::ignore = onSuccess;
        std::ignore = onRuntimeError;
        std::vector<types::GlobalDiscoveryEntry> result;
        onError(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT);
    }

    void fakeCapabilitiesClientLookupWithDiscoveryException(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const std::vector<std::string>& gbids,
            const std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
    {
        std::ignore = domains;
        std::ignore = interfaceName;
        std::ignore = gbids;
        std::ignore = messagingTtl;
        std::ignore = onSuccess;
        std::ignore = onError;
        exceptions::DiscoveryException fakeDiscoveryException("fakeDiscoveryException");
        onRuntimeError(fakeDiscoveryException);
    }

    void fakeCapabilitiesClientAddWithException(
            const types::GlobalDiscoveryEntry& entry,
            const bool awaitGlobalRegistration,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
    {
        std::ignore = entry;
        std::ignore = awaitGlobalRegistration;
        std::ignore = gbids;
        std::ignore = onSuccess;
        std::ignore = onError;
        exceptions::DiscoveryException fakeDiscoveryException("fakeDiscoveryException");
        onRuntimeError(fakeDiscoveryException);
    }

    void fakeCapabilitiesClientAddSuccess(
            const types::GlobalDiscoveryEntry& entry,
            const bool awaitGlobalRegistration,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
    {
        std::ignore = entry;
        std::ignore = awaitGlobalRegistration;
        std::ignore = gbids;
        std::ignore = onError;
        std::ignore = onRuntimeError;
        onSuccess();
    }

    void fakeCapabilitiesClientRemoveStaleWithException(
            const std::string& clusterControllerId,
            std::int64_t maxLastSeenDateMs,
            const std::string gbid,
            std::function<void()> onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeError)
    {
        std::ignore = clusterControllerId;
        std::ignore = maxLastSeenDateMs;
        std::ignore = gbid;
        std::ignore = onSuccess;
        exceptions::JoynrRuntimeException fakeRuntimeException("fake removeStale failed!");
        onRuntimeError(fakeRuntimeException);
    }

    void fakeCapabilitiesClientRemoveStaleSuccess(
            const std::string& clusterControllerId,
            std::int64_t maxLastSeenDateMs,
            const std::string gbid,
            std::function<void()> onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeError)
    {
        std::ignore = clusterControllerId;
        std::ignore = maxLastSeenDateMs;
        std::ignore = gbid;
        std::ignore = onRuntimeError;
        onSuccess();
    }

    void testRemoveUsesSameGbidOrderAsAdd(const std::vector<std::string>& selectedGbids)
    {
        const bool awaitGlobalRegistration = true;
        const std::vector<std::string>& expectedGbids{selectedGbids};
        types::ProviderQos providerQos;
        providerQos.setScope(types::ProviderScope::GLOBAL);
        _entry.setQos(providerQos);

        Sequence s;
        checkAddToGcdClient(expectedGbids, s);

        std::shared_ptr<LocalCapabilitiesDirectoryStore> capturedLCDStore =
                std::make_shared<LocalCapabilitiesDirectoryStore>();
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    remove(Eq(_dummyParticipantIdsVector[0]), _, _, _, _))
                .InSequence(s)
                .WillOnce(DoAll(SaveArg<1>(&capturedLCDStore), ReleaseSemaphore(_semaphore)));

        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();

        _localCapabilitiesDirectory->add(_entry,
                                         awaitGlobalRegistration,
                                         selectedGbids,
                                         createVoidOnSuccessFunction(),
                                         _unexpectedOnDiscoveryErrorFunction);
        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

        _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], _defaultOnSuccess,
                                            _defaultProviderRuntimeExceptionError);
        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

        std::unique_lock<std::recursive_mutex> cacheLock(capturedLCDStore->getCacheLock());
        std::vector<std::string> capturedGbids = capturedLCDStore->getGbidsForParticipantId(
                _dummyParticipantIdsVector[0], cacheLock);
        cacheLock.unlock();
        EXPECT_EQ(expectedGbids, capturedGbids);

        _localCapabilitiesDirectoryStore->clear();
    }

    std::vector<types::GlobalDiscoveryEntry> getGlobalDiscoveryEntries(
            const std::uint8_t numEntries)
    {
        if (numEntries > _dummyParticipantIdsVector.size()) {
            for (std::uint8_t i = _dummyParticipantIdsVector.size(); i <= numEntries; i++) {
                _dummyParticipantIdsVector.push_back(util::createUuid());
            }
        }

        std::vector<types::GlobalDiscoveryEntry> globalDiscoveryEntryList;
        for (std::uint8_t i = 0; i < numEntries; i++) {
            globalDiscoveryEntryList.push_back(
                    types::GlobalDiscoveryEntry(_defaultProviderVersion,
                                                _DOMAIN_1_NAME,
                                                _INTERFACE_1_NAME,
                                                _dummyParticipantIdsVector[i],
                                                types::ProviderQos(),
                                                _LASTSEEN_MS,
                                                _EXPIRYDATE_MS,
                                                _PUBLIC_KEY_ID,
                                                _EXTERNAL_ADDRESSES_VECTOR[i > 2 ? 0 : i]));
        }
        return globalDiscoveryEntryList;
    }

    void testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(
            const std::vector<std::string>& gbidsForLookup,
            const std::vector<std::string>& expectedGbids)
    {
        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    lookup(Eq(_dummyParticipantIdsVector[1]), Eq(expectedGbids),
                           Eq(_discoveryQos.getDiscoveryTimeout()), _, _, _))
                .Times(1);

        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();
        _localCapabilitiesDirectory->lookup(
                _dummyParticipantIdsVector[1], _discoveryQos, gbidsForLookup, nullptr, nullptr);
    }

    void testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(
            const std::vector<std::string>& gbids)
    {
        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        // caching dummy entry
        types::ProviderQos providerQos;
        providerQos.setScope(types::ProviderScope::GLOBAL);
        _entry.setQos(providerQos);
        const types::DiscoveryEntryWithMetaInfo expectedEntry = LCDUtil::convert(false, _entry);

        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, add(_, _, _, _, _, _))
                .Times(1)
                .WillOnce(InvokeArgument<3>());
        // Entry is already cached GCD should not be involved
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();

        _localCapabilitiesDirectory->add(_entry, true, _KNOWN_GBIDS, createVoidOnSuccessFunction(),
                                         _unexpectedOnDiscoveryErrorFunction);

        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

        auto onSuccess = [this, expectedEntry](const types::DiscoveryEntryWithMetaInfo& result) {
            EXPECT_TRUE(compareDiscoveryEntries(expectedEntry, result));
            _semaphore->notify();
        };

        _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                            _discoveryQos,
                                            gbids,
                                            onSuccess,
                                            _unexpectedOnDiscoveryErrorFunction);
        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void fakeLookupByParticipantIdWithResult(
            const std::string& participantId,
            const std::vector<std::string>& gbids,
            std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>
                    onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
    {
        std::ignore = gbids;
        std::ignore = messagingTtl;
        std::ignore = onError;
        std::ignore = onRuntimeError;
        types::ProviderQos qos;
        std::vector<types::GlobalDiscoveryEntry> discoveryEntryList;
        discoveryEntryList.push_back(types::GlobalDiscoveryEntry(_defaultProviderVersion,
                                                                 _DOMAIN_1_NAME,
                                                                 _INTERFACE_1_NAME,
                                                                 participantId,
                                                                 qos,
                                                                 _LASTSEEN_MS,
                                                                 _EXPIRYDATE_MS,
                                                                 _PUBLIC_KEY_ID,
                                                                 _EXTERNAL_ADDRESSES_VECTOR[0]));
        onSuccess(discoveryEntryList);
    }

    void simulateTimeout()
    {
        throw exceptions::JoynrTimeOutException("Simulating timeout");
    }

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>
    createLookupSuccessFunction(const int expectedNumberOfEntries)
    {
        return [this, expectedNumberOfEntries](
                       const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
            EXPECT_EQ(expectedNumberOfEntries, result.size());
            _semaphore->notify();
        };
    }

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>
    createLookupSuccessFunction(const int expectedNumberOfEntries,
                                std::vector<types::DiscoveryEntryWithMetaInfo>& returnValue)
    {
        return [this, expectedNumberOfEntries,
                &returnValue](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
            EXPECT_EQ(expectedNumberOfEntries, result.size());
            returnValue = result;
            _semaphore->notify();
        };
    }

    std::function<void()> createVoidOnSuccessFunction()
    {
        return [this]() { _semaphore->notify(); };
    }

    std::function<void(const types::DiscoveryEntryWithMetaInfo&)>
    createLookupParticipantIdSuccessFunction()
    {
        return [this](const types::DiscoveryEntryWithMetaInfo& result) {
            std::ignore = result;
            _semaphore->notify();
        };
    }

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>
    createUnexpectedLookupSuccessFunction()
    {
        return [](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
            FAIL() << "Got result: " + (result.empty() ? "EMPTY" : result.at(0).toString());
        };
    }

    std::function<void(const types::DiscoveryEntryWithMetaInfo&)>
    createUnexpectedLookupParticipantIdSuccessFunction()
    {
        return [](const types::DiscoveryEntryWithMetaInfo& result) {
            FAIL() << "Got result: " + result.toString();
        };
    }

    std::function<void(const types::DiscoveryError::Enum&)> createExpectedDiscoveryErrorFunction(
            const types::DiscoveryError::Enum& expectedError)
    {
        return [this, expectedError](const types::DiscoveryError::Enum& error) {
            EXPECT_EQ(expectedError, error);
            _semaphore->notify();
        };
    }

    std::function<void()> createUnexpectedAddOnSuccessFunction()
    {
        return []() { FAIL(); };
    }

    std::function<void(const exceptions::ProviderRuntimeException&)>
    createExpectedProviderRuntimeExceptionFunction(
            const joynr::exceptions::ProviderRuntimeException& expectedRuntimeException)
    {
        return [this,
                expectedRuntimeException](const exceptions::ProviderRuntimeException& exception) {
            EXPECT_EQ(expectedRuntimeException, exception);
            _semaphore->notify();
        };
    }

    void testAddToAllIsProperlyRejected(const types::DiscoveryError::Enum& expectedError)
    {
        const bool awaitGlobalRegistration = true;

        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(_entry)),
                        _, _, _, _, _))
                .Times(1)
                .WillOnce(InvokeArgument<4>(expectedError));
        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();

        _localCapabilitiesDirectory->addToAll(_entry,
                                              awaitGlobalRegistration,
                                              createUnexpectedAddOnSuccessFunction(),
                                              createExpectedDiscoveryErrorFunction(expectedError));

        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::Enum errorEnum)
    {
        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
                .Times(1)
                .WillOnce(InvokeArgument<5>(errorEnum));

        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();

        _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                            _INTERFACE_1_NAME,
                                            _discoveryQos,
                                            {},
                                            createUnexpectedLookupSuccessFunction(),
                                            createExpectedDiscoveryErrorFunction(errorEnum));
        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::Enum errorEnum)
    {
        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    lookup(Eq(_dummyParticipantIdsVector[1]), _, _, _, _, _))
                .Times(1)
                .WillOnce(InvokeArgument<4>(errorEnum));

        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();

        _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[1],
                                            _discoveryQos,
                                            _KNOWN_GBIDS,
                                            createUnexpectedLookupParticipantIdSuccessFunction(),
                                            createExpectedDiscoveryErrorFunction(errorEnum));

        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void testLookupByDomainInterfaceWithGbids_gbidValidationFails(
            const std::vector<std::string>& gbids,
            types::DiscoveryError::Enum errorEnum)
    {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);

        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();

        _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                            _INTERFACE_1_NAME,
                                            _discoveryQos,
                                            gbids,
                                            createUnexpectedLookupSuccessFunction(),
                                            createExpectedDiscoveryErrorFunction(errorEnum));
        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void testLookupByParticipantIdWithGbids_gbidValidationFails(
            const std::vector<std::string>& gbids,
            types::DiscoveryError::Enum errorEnum)
    {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();

        _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[1],
                                            _discoveryQos,
                                            gbids,
                                            createUnexpectedLookupParticipantIdSuccessFunction(),
                                            createExpectedDiscoveryErrorFunction(errorEnum));

        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void testAddWithGbidsIsProperlyRejected(
            const types::DiscoveryError::Enum& expectedDiscoveryError)
    {
        const bool awaitGlobalRegistration = true;
        const std::vector<std::string>& gbids = {_KNOWN_GBIDS[0]};
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _, _))
                .Times(1)
                .WillOnce(InvokeArgument<4>(expectedDiscoveryError));
        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();

        _localCapabilitiesDirectory->add(
                _entry,
                awaitGlobalRegistration,
                gbids,
                createUnexpectedAddOnSuccessFunction(),
                createExpectedDiscoveryErrorFunction(expectedDiscoveryError));

        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void testAddIsProperlyRejected(const types::DiscoveryError::Enum& expectedDiscoveryError)
    {
        const bool awaitGlobalRegistration = true;
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _, _))
                .Times(1)
                .WillOnce(InvokeArgument<4>(expectedDiscoveryError));

        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();

        joynr::exceptions::ProviderRuntimeException expectedException(
                fmt::format("Error registering provider {} in default backend: {}",
                            _entry.getParticipantId(),
                            types::DiscoveryError::getLiteral(expectedDiscoveryError)));
        _localCapabilitiesDirectory->add(
                _entry,
                awaitGlobalRegistration,
                createUnexpectedAddOnSuccessFunction(),
                createExpectedProviderRuntimeExceptionFunction(expectedException));

        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void checkAddToGcdClient(const std::vector<std::string>& expectedGbids, Sequence& s)
    {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    add(GlobalDiscoveryEntryMatcher(_expectedGlobalCapEntry), _, Eq(expectedGbids),
                        _, _, _))
                .Times(1)
                .InSequence(s)
                .WillOnce(InvokeArgument<3>());
    }

    void testGbidValidationOnAdd(const std::vector<std::string>& gbids,
                                 const types::DiscoveryError::Enum& expectedDiscoveryError)
    {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, add(_, _, _, _, _, _)).Times(0);
        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();
        const bool awaitGlobalRegistration = true;
        _localCapabilitiesDirectory->add(
                _entry,
                awaitGlobalRegistration,
                gbids,
                createUnexpectedAddOnSuccessFunction(),
                createExpectedDiscoveryErrorFunction(expectedDiscoveryError));

        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

protected:
    void registerReceivedCapabilities(const std::string& addressType,
                                      const std::string& serializedAddress)
    {
        const std::string& participantId = "TEST_participantId";
        EXPECT_CALL(*_mockMessageRouter,
                    addNextHop(participantId,
                               AllOf(Pointee(A<const system::RoutingTypes::Address>()),
                                     pointerToAddressWithSerializedAddress(
                                             addressType, serializedAddress)),
                               _,
                               _,
                               _,
                               _,
                               _))
                .Times(1);
        EXPECT_CALL(*_mockMessageRouter,
                    addNextHop(participantId,
                               AnyOf(Not(Pointee(A<const system::RoutingTypes::Address>())),
                                     Not(pointerToAddressWithSerializedAddress(
                                             addressType, serializedAddress))),
                               _,
                               _,
                               _,
                               _,
                               _))
                .Times(0);

        initializeMockLocalCapabilitiesDirectoryStore();
        finalizeTestSetupAfterMockExpectationsAreDone();

        std::unordered_multimap<std::string, types::DiscoveryEntry> capabilitiesMap;
        types::DiscoveryEntry capEntry;
        capEntry.setParticipantId(participantId);
        capabilitiesMap.insert({serializedAddress, capEntry});
        _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(capabilitiesMap));
    }

    Settings _settings;
    Settings _settingsForPersistencyTests;
    ClusterControllerSettings _clusterControllerSettings;
    ClusterControllerSettings _clusterControllerSettingsForPersistencyTests;
    int _purgeExpiredDiscoveryEntriesIntervalMs;
    std::shared_ptr<MockGlobalCapabilitiesDirectoryClient> _globalCapabilitiesDirectoryClient;
    std::shared_ptr<LocalCapabilitiesDirectoryStore> _localCapabilitiesDirectoryStore;
    std::shared_ptr<LocalCapabilitiesDirectoryStore>
            _localCapabilitiesDirectoryStoreForPersistencyTests;
    std::shared_ptr<capabilities::MockStorage> _mockLocallyRegisteredCapabilities;
    std::shared_ptr<capabilities::MockCachingStorage> _mockGlobalLookupCache;
    std::shared_ptr<MockLocalCapabilitiesDirectoryStore> _mockLocalCapabilitiesDirectoryStore;
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> _mockMessageRouter;
    std::string _clusterControllerId;
    std::shared_ptr<LocalCapabilitiesDirectory> _localCapabilitiesDirectory;
    std::shared_ptr<LocalCapabilitiesDirectory> _localCapabilitiesDirectoryWithMockCapStorage;
    std::int64_t _lastSeenDateMs;
    std::int64_t _defaultExpiryIntervalMs;
    std::chrono::milliseconds _reAddInterval;
    std::vector<std::string> _dummyParticipantIdsVector;
    types::DiscoveryQos _discoveryQos;
    std::unordered_multimap<std::string, types::DiscoveryEntry> _globalCapEntryMap;
    types::DiscoveryEntry _entry;
    types::GlobalDiscoveryEntry _expectedGlobalCapEntry;

    std::function<void()> _defaultOnSuccess;
    std::function<void(const exceptions::ProviderRuntimeException&)>
            _defaultProviderRuntimeExceptionError;
    std::function<void(const exceptions::ProviderRuntimeException&)>
            _unexpectedProviderRuntimeExceptionFunction;
    std::function<void(const types::DiscoveryError::Enum& errorEnum)>
            _unexpectedOnDiscoveryErrorFunction;
    types::Version _defaultProviderVersion;
    std::shared_ptr<Semaphore> _semaphore;
    bool _finalized;

    static const std::vector<std::string> _KNOWN_GBIDS;
    static const std::string _INTERFACE_1_NAME;
    static const std::string _DOMAIN_1_NAME;
    static const std::string _INTERFACE_2_NAME;
    static const std::string _DOMAIN_2_NAME;
    static const std::string _INTERFACE_3_NAME;
    static const std::string _DOMAIN_3_NAME;
    static const std::string _LOCAL_ADDRESS;
    static const std::vector<std::string> _EXTERNAL_ADDRESSES_VECTOR;
    static const std::int64_t _LASTSEEN_MS;
    static const std::int64_t _EXPIRYDATE_MS;
    static const std::string _PUBLIC_KEY_ID;
    static const int _TIMEOUT;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectoryTest);
};

const std::vector<std::string> LocalCapabilitiesDirectoryTest::_KNOWN_GBIDS{
        "testGbid1", "testGbid2", "testGbid3"};
const std::string LocalCapabilitiesDirectoryTest::_INTERFACE_1_NAME("myInterfaceA");
const std::string LocalCapabilitiesDirectoryTest::_INTERFACE_2_NAME("myInterfaceB");
const std::string LocalCapabilitiesDirectoryTest::_INTERFACE_3_NAME("myInterfaceC");
const std::string LocalCapabilitiesDirectoryTest::_DOMAIN_1_NAME("domainA");
const std::string LocalCapabilitiesDirectoryTest::_DOMAIN_2_NAME("domainB");
const std::string LocalCapabilitiesDirectoryTest::_DOMAIN_3_NAME("domainB");
const std::string LocalCapabilitiesDirectoryTest::_LOCAL_ADDRESS(serializer::serializeToJson(
        system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "localTopic")));
const std::vector<std::string> LocalCapabilitiesDirectoryTest::_EXTERNAL_ADDRESSES_VECTOR{
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "externalTopic")),
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[1], "externalTopic")),
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[2], "externalTopic"))};
const std::int64_t LocalCapabilitiesDirectoryTest::_LASTSEEN_MS(1000);
const std::int64_t LocalCapabilitiesDirectoryTest::_EXPIRYDATE_MS(10000);
const std::string LocalCapabilitiesDirectoryTest::_PUBLIC_KEY_ID("publicKeyId");
const int LocalCapabilitiesDirectoryTest::_TIMEOUT(2000);

TEST_F(LocalCapabilitiesDirectoryTest, add_global_invokesGcd)
{
    Sequence s;

    checkAddToGcdClient(_KNOWN_GBIDS, s);
    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addCapabilityWithSingleNonDefaultGbid)
{
    const std::vector<std::string>& gbids{_KNOWN_GBIDS[1]};
    const std::vector<std::string>& expectedGbids = gbids;
    const bool awaitGlobalRegistration = true;
    Sequence s;
    checkAddToGcdClient(expectedGbids, s);
    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry, awaitGlobalRegistration, gbids,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addUpdatesLastSeenDate)
{
    const std::vector<std::string>& gbids{_KNOWN_GBIDS[1]};
    const std::vector<std::string>& expectedGbids = gbids;
    const bool awaitGlobalRegistration = true;
    Sequence s;
    checkAddToGcdClient(expectedGbids, s);
    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry, awaitGlobalRegistration, gbids,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(TimePoint::now().toMilliseconds() >= _entry.getLastSeenDateMs());
    EXPECT_TRUE(_entry.getLastSeenDateMs() > TimePoint::now().toMilliseconds() - _TIMEOUT);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addWithGbids_global_multipleGbids_invokesGcd)
{
    const std::vector<std::string>& gbids{_KNOWN_GBIDS[1], _KNOWN_GBIDS[0]};
    const std::vector<std::string>& expectedGbids = gbids;
    const bool awaitGlobalRegistration = true;
    Sequence s;
    checkAddToGcdClient(expectedGbids, s);
    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry, awaitGlobalRegistration, gbids,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addWithGbids_global_emptyGbidVector_addsToKnownBackends)
{
    const std::vector<std::string>& gbids{};
    const std::vector<std::string>& expectedGbids = _KNOWN_GBIDS;
    const bool awaitGlobalRegistration = true;
    Sequence s;
    checkAddToGcdClient(expectedGbids, s);
    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry,
                                     awaitGlobalRegistration,
                                     gbids,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addToAll_global_invokesGcd)
{
    const std::vector<std::string>& expectedGbids = _KNOWN_GBIDS;
    const bool awaitGlobalRegistration = true;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(GlobalDiscoveryEntryMatcher(_expectedGlobalCapEntry), _, Eq(expectedGbids), _,
                    _, _))
            .Times(1)
            .WillOnce(InvokeArgument<3>());

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();
    _localCapabilitiesDirectory->addToAll(_entry, awaitGlobalRegistration,
                                          createVoidOnSuccessFunction(),
                                          _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, add_local_doesNotInvokeGcd)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _, _))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();
    _localCapabilitiesDirectory->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addGlobalEntry_callsMockStorage)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setQos(providerQos);
    const std::vector<std::string>& expectedGbids = _KNOWN_GBIDS;
    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                insertInLocalCapabilitiesStorage(DiscoveryEntryMatcher(_entry), expectedGbids))
            .Times(1);
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore, insertInGlobalLookupCache(_,_)).Times(0);
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testReAddAllGlobalDiscoveryEntriesPeriodically)
{
    // make sure that there is only one runtime that is periodically calling touch
    initializeMockLocalCapabilitiesDirectoryStore();
    test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);
    configureShortCapabilitiesFreshnessUpdateInterval();
    auto gcdSemaphore = std::make_shared<Semaphore>(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                reAdd(Eq(_localCapabilitiesDirectoryStore), Eq(_LOCAL_ADDRESS)))
            .Times(AtLeast(2))
            .WillRepeatedly(ReleaseSemaphore(gcdSemaphore));
    // set short reAddInterval for testing
    _reAddInterval = std::chrono::milliseconds(500);
    finalizeTestSetupAfterMockExpectationsAreDone();

    EXPECT_TRUE(gcdSemaphore->waitFor(std::chrono::milliseconds(1000)));
    EXPECT_FALSE(gcdSemaphore->waitFor(std::chrono::milliseconds(400)));
    EXPECT_TRUE(gcdSemaphore->waitFor(std::chrono::milliseconds(400)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addLocalEntry_callsMockStorage)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);
    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                insertInLocalCapabilitiesStorage(DiscoveryEntryMatcher(_entry), _KNOWN_GBIDS))
            .Times(1);
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore, insertInGlobalLookupCache(_, _)).Times(0);
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupLocalEntryByParticipantId_callsMockStorage)
{
    boost::optional<joynr::types::DiscoveryEntry> result = boost::none;
    _mockLocallyRegisteredCapabilities->setLookupByParticipantIdResult(result);
    std::vector<std::string> gbids{_KNOWN_GBIDS[1]};
    initializeMockLocalCapabilitiesDirectoryStore();

    EXPECT_CALL(
            *_mockLocalCapabilitiesDirectoryStore,
            getLocalAndCachedCapabilities(
                    Matcher<const std::string&>(_dummyParticipantIdsVector[0]), _, Eq(gbids), _))
            .Times(1)
            .WillOnce(Return(true));
    finalizeTestSetupAfterMockExpectationsAreDone();

    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    _localCapabilitiesDirectoryWithMockCapStorage->lookup(
            _dummyParticipantIdsVector[0],
            _discoveryQos,
            gbids,
            createUnexpectedLookupParticipantIdSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT));
}

TEST_F(LocalCapabilitiesDirectoryTest, addGlobalCapSucceeds_NextAddShallAddGlobalAgain)
{
    const bool awaitGlobalRegistration = true;

    Sequence s;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(_entry)), _,
                    _, _, _, _))
            .InSequence(s)
            .WillOnce(InvokeArgument<3>());
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(_entry)), _,
                    _, _, _, _))
            .InSequence(s)
            .WillOnce(InvokeArgument<3>());
    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry,
                                     awaitGlobalRegistration,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->add(_entry,
                                     awaitGlobalRegistration,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbidsIsProperlyRejected_invalidGbid)
{
    testAddWithGbidsIsProperlyRejected(types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbidsIsProperlyRejected_unknownGbid)
{
    testAddWithGbidsIsProperlyRejected(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbidsIsProperlyRejected_internalError)
{
    testAddWithGbidsIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddIsProperlyRejected_invalidGbid)
{
    testAddIsProperlyRejected(types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddIsProperlyRejected_unknownGbid)
{
    testAddIsProperlyRejected(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddIsProperlyRejected_internalError)
{
    testAddIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbids_unknownGbid)
{
    const std::vector<std::string>& gbids{_KNOWN_GBIDS[0], "unknownGbid"};
    testGbidValidationOnAdd(gbids, types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbids_invalidGbid_emptyGbid)
{
    const std::vector<std::string>& gbids{""};
    testGbidValidationOnAdd(gbids, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddWithGbids_invalidGbid_duplicateGbid)
{
    const std::vector<std::string>& gbids{_KNOWN_GBIDS[1], _KNOWN_GBIDS[1]};
    testGbidValidationOnAdd(gbids, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, addSameGbidTwiceInARow)
{
    const bool awaitGlobalRegistration = true;
    const std::vector<std::string>& gbids{_KNOWN_GBIDS[0]};
    const std::vector<std::string>& expectedGbids = gbids;

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(_entry)), _,
                    Eq(expectedGbids), _, _, _))
            .Times(2)
            .WillRepeatedly(InvokeArgument<3>());

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry,
                                     awaitGlobalRegistration,
                                     gbids,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->add(_entry,
                                     awaitGlobalRegistration,
                                     gbids,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addDifferentGbidsAfterEachOther)
{
    const bool awaitGlobalRegistration = true;
    const std::vector<std::string>& gbids1{_KNOWN_GBIDS[0]};
    const std::vector<std::string>& expectedGbids1 = gbids1;
    const std::vector<std::string>& gbids2{_KNOWN_GBIDS[1]};
    const std::vector<std::string>& expectedGbids2 = gbids2;

    Sequence s;
    checkAddToGcdClient(expectedGbids1, s);
    checkAddToGcdClient(expectedGbids2, s);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry,
                                     awaitGlobalRegistration,
                                     gbids1,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->add(_entry,
                                     awaitGlobalRegistration,
                                     gbids2,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // provider is now registered for both GBIDs
    _localCapabilitiesDirectory->lookup(_entry.getParticipantId(),
                                        _discoveryQos,
                                        gbids1,
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->lookup(_entry.getParticipantId(),
                                        _discoveryQos,
                                        gbids2,
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddKnownLocalEntryDoesNothing)
{
    const bool awaitGlobalRegistration = false;
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);
    types::DiscoveryEntry newDiscoveryEntry(_entry);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _, _))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry,
                                     awaitGlobalRegistration,
                                     _defaultOnSuccess,
                                     _defaultProviderRuntimeExceptionError);

    _localCapabilitiesDirectory->add(newDiscoveryEntry,
                                     awaitGlobalRegistration,
                                     _defaultOnSuccess,
                                     _defaultProviderRuntimeExceptionError);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAll)
{
    const std::vector<std::string>& expectedGbids = _KNOWN_GBIDS;
    const bool awaitGlobalRegistration = true;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(_entry)), _,
                    Eq(expectedGbids), _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<3>());

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->addToAll(_entry, awaitGlobalRegistration,
                                          createVoidOnSuccessFunction(),
                                          _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllLocal)
{
    const bool awaitGlobalRegistration = true;
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _, _))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->addToAll(_entry,
                                          awaitGlobalRegistration,
                                          createVoidOnSuccessFunction(),
                                          _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(2)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllIsProperlyRejected_internalError)
{
    testAddToAllIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllIsProperlyRejected_invalidGbid)
{
    testAddToAllIsProperlyRejected(types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllIsProperlyRejected_unknownGbid)
{
    testAddToAllIsProperlyRejected(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllIsProperlyRejected_exception)
{
    const bool awaitGlobalRegistration = true;

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(_entry)), _,
                    Eq(_KNOWN_GBIDS), _, _, _))
            .Times(1)
            .WillOnce(Invoke(
                    this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddWithException));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->addToAll(
            _entry,
            awaitGlobalRegistration,
            createUnexpectedAddOnSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbids_emptyDomainsVector_throwsProviderRuntimeException)
{
    const std::vector<std::string>& emptyDomains{};

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();
    EXPECT_THROW(_localCapabilitiesDirectory->lookup(emptyDomains,
                                                     _INTERFACE_1_NAME,
                                                     _discoveryQos,
                                                     _KNOWN_GBIDS,
                                                     createUnexpectedLookupSuccessFunction(),
                                                     _unexpectedOnDiscoveryErrorFunction),
                 exceptions::ProviderRuntimeException);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbids_globalOnly_noLocalButRemoteCachedEntries_doesNotInvokeGcd_returnsFilteredResult)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    const std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList =
            getGlobalDiscoveryEntries(_KNOWN_GBIDS.size());
    std::vector<types::DiscoveryEntryWithMetaInfo> expectedEntries;
    for (const auto& entry : discoveryEntryResultList) {
        expectedEntries.push_back(LCDUtil::convert(false, entry));
    }

    // simulate global capability directory would store three entries.
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    // the following special expectation hits first and retires on saturation, i.e.
    // after this the earlier specified expecatations are used
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList))
            .RetiresOnSaturation();

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    std::vector<types::DiscoveryEntryWithMetaInfo> result;
    _localCapabilitiesDirectory->lookup(
            {_DOMAIN_1_NAME},
            _INTERFACE_1_NAME,
            _discoveryQos,
            _KNOWN_GBIDS,
            createLookupSuccessFunction(discoveryEntryResultList.size(), result),
            _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    for (const auto& expectedEntry : expectedEntries) {
        ASSERT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntry) != result.cend());
    }

    result = {};
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        {_KNOWN_GBIDS[1]},
                                        createLookupSuccessFunction(1, result),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntries[1]) != result.cend());

    result = {};
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        {_KNOWN_GBIDS[0]},
                                        createLookupSuccessFunction(1, result),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntries[0]) != result.cend());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbids_globalOnly_localGlobalEntries_returnsFilteredResult)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setQos(providerQos);

    types::GlobalDiscoveryEntry globalEntry =
                LCDUtil::toGlobalDiscoveryEntry(_entry, _LOCAL_ADDRESS);
    const std::vector<types::GlobalDiscoveryEntry>& globalEntryVec = {globalEntry};

    const types::DiscoveryEntryWithMetaInfo expectedEntry = LCDUtil::convert(false, _entry);

    Sequence s;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(DiscoveryEntryMatcher(_entry), _, Eq(_KNOWN_GBIDS), _, _, _))
            .Times(1)
            .InSequence(s)
            .WillOnce(InvokeArgument<3>());
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
                .Times(2)
                .WillRepeatedly(InvokeArgument<4>(globalEntryVec));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry, true, _KNOWN_GBIDS, createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // local entry is now registered for all GBIDs with the same participantId

    std::vector<types::DiscoveryEntryWithMetaInfo> result;
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1, result),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    ASSERT_EQ(1, result.size());
    EXPECT_TRUE(compareDiscoveryEntries(expectedEntry, result.front()));

    // search in the 3rd backend for global entries. There is only one entry
    result = {};
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        std::vector<std::string>{_KNOWN_GBIDS[2]},
                                        createLookupSuccessFunction(1, result),
                                        _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    ASSERT_EQ(1, result.size());
    EXPECT_TRUE(compareDiscoveryEntries(expectedEntry, result.front()));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbids_globalOnly_localGlobalEntries_doesNotInvokeGcd_returnsFilteredResult)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setQos(providerQos);

    const types::DiscoveryEntryWithMetaInfo expectedEntry = LCDUtil::convert(false, _entry);

    Sequence s;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(DiscoveryEntryMatcher(_entry), _, Eq(_KNOWN_GBIDS), _, _, _))
            .Times(1)
            .InSequence(s)
            .WillOnce(InvokeArgument<3>());
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry, true, _KNOWN_GBIDS, createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // local entry is now registered and cached for all GBIDs with the same participantId

    auto onSuccess = [this, expectedEntry](const types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_TRUE(compareDiscoveryEntries(expectedEntry, result));
        _semaphore->notify();
    };

    std::vector<std::string> gbids{_KNOWN_GBIDS[1]};
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        _discoveryQos,
                                        gbids,
                                        onSuccess,
                                        _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    gbids = {_KNOWN_GBIDS[0]};
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        _discoveryQos,
                                        gbids,
                                        onSuccess,
                                        _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        _discoveryQos,
                                        _KNOWN_GBIDS,
                                        onSuccess,
                                        _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbids_globalOnly_multipleGbids_allCached)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList =
            getGlobalDiscoveryEntries(6);

    // simulate global capability directory would store five entries
    // globalEntry_withParticipantId0_in_gbid0, globalEntry_withParticipantId1_in_gbid1,
    // globalEntry_withParticipantId2_in_gbid2, globalEntry_withParticipantId3_in_gbid0,
    // globalEntry_withParticipantId4_in_gbid0, globalEntry_withParticipantId5_in_gbid0
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList))
            .RetiresOnSaturation();

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->lookup(
            {_DOMAIN_1_NAME},
            _INTERFACE_1_NAME,
            _discoveryQos,
            _KNOWN_GBIDS,
            createLookupSuccessFunction(discoveryEntryResultList.size()),
            _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // 6 enries are now in cache

    const std::vector<std::string>& multipleGbids{_KNOWN_GBIDS[0], _KNOWN_GBIDS[2]};
    const std::uint8_t expectedReturnedGlobalEntries = 5;
    std::vector<types::DiscoveryEntryWithMetaInfo> result;
    _localCapabilitiesDirectory->lookup(
            {_DOMAIN_1_NAME},
            _INTERFACE_1_NAME,
            _discoveryQos,
            multipleGbids,
            createLookupSuccessFunction(expectedReturnedGlobalEntries, result),
            _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    auto getPredicate = [](const std::string& participantId) {
        return [participantId](const types::DiscoveryEntryWithMetaInfo& entry) {
            return entry.getParticipantId() == participantId;
        };
    };
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(),
                             getPredicate(_dummyParticipantIdsVector[0])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(),
                             getPredicate(_dummyParticipantIdsVector[2])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(),
                             getPredicate(_dummyParticipantIdsVector[3])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(),
                             getPredicate(_dummyParticipantIdsVector[4])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(),
                             getPredicate(_dummyParticipantIdsVector[5])) != result.cend());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbids_globalOnly_emptyGbidsVector_allCached)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList =
            getGlobalDiscoveryEntries(6);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    // the following special expectation hits first and retires on saturation, i.e.
    // after this the earlier specified expecatations are used
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList))
            .RetiresOnSaturation();

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->lookup(
            {_DOMAIN_1_NAME},
            _INTERFACE_1_NAME,
            _discoveryQos,
            _KNOWN_GBIDS,
            createLookupSuccessFunction(discoveryEntryResultList.size()),
            _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    const std::vector<std::string>& emptyGbids{};
    const std::uint8_t expectedReturnedGlobalEntries = 6;
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        emptyGbids,
                                        createLookupSuccessFunction(expectedReturnedGlobalEntries),
                                        _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbids_globalOnly_multipleGbids_noneCached)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList =
            getGlobalDiscoveryEntries(2);
    std::vector<std::string> multipleGbids{_KNOWN_GBIDS[0], _KNOWN_GBIDS[1]};

    // no entries cached, lookup of GlobalCapabilitiesDirectoryClient will be called
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(multipleGbids), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    const std::uint8_t& expectedReturnedGlobalEntries = 2;
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        multipleGbids,
                                        createLookupSuccessFunction(expectedReturnedGlobalEntries),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbids_globalOnly_emptyGbidsVector_noneCached)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList =
            getGlobalDiscoveryEntries(2);

    // no entries cached, lookup of GlobalCapabilitiesDirectoryClient will be called
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(_KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    const std::uint8_t expectedReturnedGlobalEntries = 2;
    std::vector<std::string> emptyGbidsVector{};
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        emptyGbidsVector,
                                        createLookupSuccessFunction(expectedReturnedGlobalEntries),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdGbids_globalOnly_localOnlyEntry_invokesGcd_returnsNoEntryForParticipant)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    std::vector<std::string> gbidsForLookup{_KNOWN_GBIDS[2], _KNOWN_GBIDS[0]};
    std::vector<std::string> expectedGbids = gbidsForLookup;

    types::GlobalDiscoveryEntry remoteEntry(_entry.getProviderVersion(),
                                            _entry.getDomain(),
                                            _entry.getInterfaceName(),
                                            _entry.getParticipantId(),
                                            _entry.getQos(),
                                            _entry.getLastSeenDateMs() + 42,
                                            _entry.getExpiryDateMs() + 10,
                                            _entry.getPublicKeyId(),
                                            _EXTERNAL_ADDRESSES_VECTOR[0]);

    // register local entry
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);

    // will be ignored because there is a local entry with the same participantId
    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult{remoteEntry};
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(Eq(_dummyParticipantIdsVector[0]),
                                                            Eq(expectedGbids),
                                                            Eq(_discoveryQos.getDiscoveryTimeout()),
                                                            _,
                                                            _,
                                                            _))
            .Times(1)
            .WillOnce(InvokeArgument<3>(onSuccessResult));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry,
                                     true,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->lookup(
            _dummyParticipantIdsVector[0],
            _discoveryQos,
            gbidsForLookup,
            createUnexpectedLookupParticipantIdSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT));
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbids_globalOnly_multipleGbids_allCached_1)
{
    std::vector<std::string> gbids{_KNOWN_GBIDS[0], _KNOWN_GBIDS[2]};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbids);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbids_globalOnly_multipleGbids_allCached_2)
{
    std::vector<std::string> gbids{_KNOWN_GBIDS[1], _KNOWN_GBIDS[2]};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbids);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbids_globalOnly_multipleGbids_noneCached_1)
{
    std::vector<std::string> gbidsForLookup{_KNOWN_GBIDS[0], _KNOWN_GBIDS[2]};
    std::vector<std::string> expectedGbids{gbidsForLookup};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(
            gbidsForLookup, expectedGbids);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbids_globalOnly_multipleGbids_noneCached_2)
{
    std::vector<std::string> gbidsForLookup{_KNOWN_GBIDS[1], _KNOWN_GBIDS[2]};
    std::vector<std::string> expectedGbids{gbidsForLookup};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(
            gbidsForLookup, expectedGbids);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbids_globalOnly_emptyGbidsVector_allCached)
{
    std::vector<std::string> gbidsForLookup{};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbidsForLookup);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbids_globalOnly_emptyGbidsVector_noneCached)
{
    std::vector<std::string> gbidsForLookup{};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(
            gbidsForLookup, _KNOWN_GBIDS);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbidsIsProperlyRejected_exception)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::
                                           fakeCapabilitiesClientLookupWithDiscoveryException));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->lookup(
            {_DOMAIN_1_NAME},
            _INTERFACE_1_NAME,
            _discoveryQos,
            _KNOWN_GBIDS,
            createUnexpectedLookupSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbidsIsProperlyRejected_invalidGbid)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::Enum::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbidsIsProperlyRejected_unknownGbid)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::Enum::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbidsIsProperlyRejected_internalError)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbidsIsProperlyRejected_noEntryForSelectedBackend)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(
            types::DiscoveryError::NO_ENTRY_FOR_SELECTED_BACKENDS);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_exception)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    exceptions::DiscoveryException fakeDiscoveryException("fakeDiscoveryException");
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(Eq(_dummyParticipantIdsVector[1]), _,
                       Eq(_discoveryQos.getDiscoveryTimeout()), _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<5>(fakeDiscoveryException));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->lookup(
            _dummyParticipantIdsVector[1],
            _discoveryQos,
            _KNOWN_GBIDS,
            createUnexpectedLookupParticipantIdSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_invalidGbid)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_unknownGbid)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbidsIsProperlyRejected_internalError)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbidsIsProperlyRejected_noEntryForSelectedBackend)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(
            types::DiscoveryError::NO_ENTRY_FOR_SELECTED_BACKENDS);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbidsIsProperlyRejected_noEntryForParticipant)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(
            types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_unknownGbids)
{
    const std::vector<std::string>& unknownGbids{"unknownGbid", _KNOWN_GBIDS[0]};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(
            unknownGbids, types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_unknownGbids)
{
    const std::vector<std::string>& unknownGbids{"unknownGbid", _KNOWN_GBIDS[0]};
    testLookupByParticipantIdWithGbids_gbidValidationFails(
            unknownGbids, types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_invalidGbid_emptyGbid)
{
    const std::vector<std::string>& emptyGbid{""};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(
            emptyGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_invalidGbid_emptyGbid)
{
    const std::vector<std::string>& emptyGbid{""};
    testLookupByParticipantIdWithGbids_gbidValidationFails(
            emptyGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_invalidGbid_duplicateGbid)
{
    const std::vector<std::string>& duplicateGbid{
            _KNOWN_GBIDS[1], _KNOWN_GBIDS[0], _KNOWN_GBIDS[1]};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(
            duplicateGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_invalidGbid_duplicateGbid)
{
    const std::vector<std::string>& duplicateGbid{
            _KNOWN_GBIDS[1], _KNOWN_GBIDS[0], _KNOWN_GBIDS[1]};

    initializeMockLocalCapabilitiesDirectoryStore();
    testLookupByParticipantIdWithGbids_gbidValidationFails(
            duplicateGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterface_globalOnly_emptyCache_invokesGcd)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    const std::vector<types::GlobalDiscoveryEntry>& discoveryEntriesResultList{};
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(_KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    auto onSuccess = [this](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(0, result.size());
        _semaphore->notify();
    };

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        onSuccess,
                                        _unexpectedProviderRuntimeExceptionFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterface_globalOnly_remoteEntryInCache_doesNotInvokeGcd)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    const std::vector<types::GlobalDiscoveryEntry> discoveryEntriesResultList =
            getGlobalDiscoveryEntries(2);
    const std::vector<types::GlobalDiscoveryEntry> expectedResult = discoveryEntriesResultList;

    auto onSuccess =
            [this, &expectedResult](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
                EXPECT_EQ(expectedResult.size(), result.size());
                _semaphore->notify();
            };

    EXPECT_CALL(*_mockMessageRouter, addNextHop(_, _, _, _, _, _, _)).Times(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    // the following special expectation hits first and retires on saturation, i.e.
    // after this the earlier specified expecatations are used
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(_KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList))
            .RetiresOnSaturation();

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    // add DiscoveryEntries to cache
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        onSuccess,
                                        _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        onSuccess,
                                        _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterface_localThenGlobal_emptyCache_invokesGcd)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    const std::vector<types::GlobalDiscoveryEntry>& discoveryEntriesResultList{};
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(_KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    auto onSuccess = [this](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(0, result.size());
        _semaphore->notify();
    };

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        onSuccess,
                                        _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterface_localThenGlobal_localEntryCached_doesNotInvokeGcd)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    // add local capability
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setQos(providerQos);
    const types::DiscoveryEntryWithMetaInfo expectedEntry = LCDUtil::convert(true, _entry);
    _localCapabilitiesDirectory->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // do actual lookup
    auto onSuccess =
            [this, &expectedEntry](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
                ASSERT_EQ(1, result.size());
                EXPECT_TRUE(compareDiscoveryEntries(expectedEntry, result[0]));
                _semaphore->notify();
            };

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        onSuccess,
                                        _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterface_localThenGlobal_remoteEntryCached_doesNotInvokeGcd)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    const std::vector<types::GlobalDiscoveryEntry> discoveryEntriesResultList =
            getGlobalDiscoveryEntries(1);
    const types::DiscoveryEntryWithMetaInfo expectedResult =
            LCDUtil::convert(false, discoveryEntriesResultList[0]);

    auto onSuccess =
            [this, &expectedResult](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
                ASSERT_EQ(1, result.size());
                EXPECT_EQ(expectedResult, result[0]);
                _semaphore->notify();
            };

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    // the following special expectation hits first and retires on saturation, i.e.
    // after this the earlier specified expecatations are used
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(_KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList))
            .RetiresOnSaturation();

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    // add DiscoveryEntries to cache
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        onSuccess,
                                        _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        onSuccess,
                                        _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantId_localThenGlobal_localEntry_doesNotInvokeGcdAndReturnsLocalEntry)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    // add local capability
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);
    const types::DiscoveryEntryWithMetaInfo expectedEntry = LCDUtil::convert(true, _entry);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();
    _localCapabilitiesDirectory->add(
            _entry, createVoidOnSuccessFunction(), _defaultProviderRuntimeExceptionError);

    auto onSuccess = [this, &expectedEntry](const types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_TRUE(compareDiscoveryEntries(expectedEntry, result));
        _semaphore->notify();
    };

    _localCapabilitiesDirectory->lookup(
            _dummyParticipantIdsVector[0], onSuccess, _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, removeCapabilities_invokesGcdClient)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    types::DiscoveryEntry entry1(_defaultProviderVersion,
                                 _DOMAIN_1_NAME,
                                 _INTERFACE_1_NAME,
                                 _dummyParticipantIdsVector[0],
                                 providerQos,
                                 _lastSeenDateMs,
                                 _lastSeenDateMs + _defaultExpiryIntervalMs,
                                 _PUBLIC_KEY_ID);

    Sequence s;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(1)
            .InSequence(s);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                remove(Eq(_dummyParticipantIdsVector[0]), Eq(_localCapabilitiesDirectoryStore), _,
                       _, _))
            .Times(1)
            .InSequence(s);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            _entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], _defaultOnSuccess,
                                        _defaultProviderRuntimeExceptionError);
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveGlobal_participantNotRegisteredNoGbids_GcdCalled)
{
    _localCapabilitiesDirectoryStore->clear();
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                remove(Eq(_dummyParticipantIdsVector[0]), _, _, _, _))
            .Times(1);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0],
                                        createVoidOnSuccessFunction(),
                                        _defaultProviderRuntimeExceptionError);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemove_localProvider_GcdNotCalled)
{
    _localCapabilitiesDirectoryStore->clear();
    types::ProviderQos providerQos = types::ProviderQos();
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                remove(Eq(_dummyParticipantIdsVector[0]), _, _, _, _))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0],
                                        createVoidOnSuccessFunction(),
                                        _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveUsesSameGbidOrderAsAdd_1)
{
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[0]});
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveUsesSameGbidOrderAsAdd_2)
{
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[1]});
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveUsesSameGbidOrderAsAdd_3)
{
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[0], _KNOWN_GBIDS[1]});
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveUsesSameGbidOrderAsAdd_4)
{
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[1], _KNOWN_GBIDS[0]});
}

TEST_F(LocalCapabilitiesDirectoryTest,
       testRemoveGlobal_onApplicationErrorCalled_NoEntryForSelectedBackends)
{
    auto semaphore = std::make_shared<Semaphore>(0);
    boost::optional<types::DiscoveryEntry> optionalEntry = boost::none;
    _mockLocallyRegisteredCapabilities->setLookupByParticipantIdResult(optionalEntry);

    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .WillOnce(DoAll(
                    InvokeArgument<3>(types::DiscoveryError::Enum::NO_ENTRY_FOR_SELECTED_BACKENDS),
                    ReleaseSemaphore(semaphore)));

    EXPECT_CALL(*_mockGlobalLookupCache, removeByParticipantId(_dummyParticipantIdsVector[0]))
            .Times(1);
    EXPECT_CALL(*_mockLocallyRegisteredCapabilities,
                removeByParticipantId(_dummyParticipantIdsVector[0]))
            .Times(1);

    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                eraseParticipantIdToGbidMapping(Eq(_dummyParticipantIdsVector[0]), _))
            .Times(1);

    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->remove(_dummyParticipantIdsVector[0],
                                                          _defaultOnSuccess,
                                                          _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       testRemoveGlobal_onApplicationErrorCalled_NoEntryForParticipant)
{
    auto semaphore = std::make_shared<Semaphore>(0);
    boost::optional<types::DiscoveryEntry> optionalEntry = boost::none;
    _mockLocallyRegisteredCapabilities->setLookupByParticipantIdResult(optionalEntry);

    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .WillOnce(
                    DoAll(InvokeArgument<3>(types::DiscoveryError::Enum::NO_ENTRY_FOR_PARTICIPANT),
                          ReleaseSemaphore(semaphore)));

    EXPECT_CALL(*_mockGlobalLookupCache, removeByParticipantId(_dummyParticipantIdsVector[0]))
            .Times(1);
    EXPECT_CALL(*_mockLocallyRegisteredCapabilities,
                removeByParticipantId(_dummyParticipantIdsVector[0]))
            .Times(1);

    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                eraseParticipantIdToGbidMapping(Eq(_dummyParticipantIdsVector[0]), _))
            .Times(1);

    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->remove(_dummyParticipantIdsVector[0],
                                                          _defaultOnSuccess,
                                                          _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveGlobal_onApplicationErrorCalled_InvalidGbid)
{
    auto semaphore = std::make_shared<Semaphore>(0);
    boost::optional<types::DiscoveryEntry> optionalEntry = boost::none;
    _mockLocallyRegisteredCapabilities->setLookupByParticipantIdResult(optionalEntry);

    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .WillOnce(DoAll(InvokeArgument<3>(types::DiscoveryError::Enum::INVALID_GBID),
                            ReleaseSemaphore(semaphore)));

    EXPECT_CALL(*_mockGlobalLookupCache, removeByParticipantId(_dummyParticipantIdsVector[0]))
            .Times(0);
    EXPECT_CALL(*_mockLocallyRegisteredCapabilities,
                removeByParticipantId(_dummyParticipantIdsVector[0]))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                eraseParticipantIdToGbidMapping(Eq(_dummyParticipantIdsVector[0]), _))
            .Times(0);

    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->remove(_dummyParticipantIdsVector[0],
                                                          _defaultOnSuccess,
                                                          _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveGlobal_onApplicationErrorCalled_UnknownGbid)
{
    auto semaphore = std::make_shared<Semaphore>(0);
    boost::optional<types::DiscoveryEntry> optionalEntry = boost::none;
    _mockLocallyRegisteredCapabilities->setLookupByParticipantIdResult(optionalEntry);

    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .WillOnce(DoAll(InvokeArgument<3>(types::DiscoveryError::Enum::UNKNOWN_GBID),
                            ReleaseSemaphore(semaphore)));

    EXPECT_CALL(*_mockGlobalLookupCache, removeByParticipantId(_dummyParticipantIdsVector[0]))
            .Times(0);
    EXPECT_CALL(*_mockLocallyRegisteredCapabilities,
                removeByParticipantId(_dummyParticipantIdsVector[0]))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                eraseParticipantIdToGbidMapping(Eq(_dummyParticipantIdsVector[0]), _))
            .Times(0);

    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->remove(_dummyParticipantIdsVector[0],
                                                          _defaultOnSuccess,
                                                          _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveGlobal_onApplicationErrorCalled_InternalError)
{
    auto semaphore = std::make_shared<Semaphore>(0);
    boost::optional<types::DiscoveryEntry> optionalEntry = boost::none;
    _mockLocallyRegisteredCapabilities->setLookupByParticipantIdResult(optionalEntry);

    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .WillOnce(DoAll(InvokeArgument<3>(types::DiscoveryError::Enum::INTERNAL_ERROR),
                            ReleaseSemaphore(semaphore)));

    EXPECT_CALL(*_mockGlobalLookupCache, removeByParticipantId(_dummyParticipantIdsVector[0]))
            .Times(0);
    EXPECT_CALL(*_mockLocallyRegisteredCapabilities,
                removeByParticipantId(_dummyParticipantIdsVector[0]))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                eraseParticipantIdToGbidMapping(Eq(_dummyParticipantIdsVector[0]), _))
            .Times(0);

    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->remove(_dummyParticipantIdsVector[0],
                                                          _defaultOnSuccess,
                                                          _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, reregisterGlobalCapabilities)
{
    types::DiscoveryEntry entry1(_defaultProviderVersion,
                                 _DOMAIN_1_NAME,
                                 _INTERFACE_1_NAME,
                                 _dummyParticipantIdsVector[0],
                                 types::ProviderQos(),
                                 _lastSeenDateMs,
                                 _lastSeenDateMs + _defaultExpiryIntervalMs,
                                 _PUBLIC_KEY_ID);
    types::DiscoveryEntry expectedEntry1(entry1);

    types::DiscoveryEntry entry2(_defaultProviderVersion,
                                 _DOMAIN_2_NAME,
                                 _INTERFACE_2_NAME,
                                 _dummyParticipantIdsVector[1],
                                 types::ProviderQos(),
                                 _lastSeenDateMs,
                                 _lastSeenDateMs + _defaultExpiryIntervalMs,
                                 _PUBLIC_KEY_ID);
    types::DiscoveryEntry expectedEntry2(entry2);

    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(expectedEntry2)),
                _,
                _,
                _,
                _,
                _))
            .Times(1);
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(expectedEntry1)),
                _,
                _,
                _,
                _,
                _))
            .Times(1);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(entry2)),
                    _,
                    _,
                    _,
                    _,
                    _))
            .Times(1)
            .RetiresOnSaturation();
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(entry1)),
                    _,
                    _,
                    _,
                    _,
                    _))
            .Times(1)
            .RetiresOnSaturation();

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry1, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    _localCapabilitiesDirectory->add(
            entry2, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    // sleep to get new values of lastSeenDateMs and expiryDateMs in
    // triggerGlobalProviderReregistration
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const std::int64_t newLastSeenDateMs = TimePoint::now().toMilliseconds();
    const std::int64_t newExpiryDateMs = newLastSeenDateMs + _defaultExpiryIntervalMs;

    expectedEntry1.setLastSeenDateMs(newLastSeenDateMs);
    expectedEntry1.setExpiryDateMs(newExpiryDateMs);
    expectedEntry2.setLastSeenDateMs(newLastSeenDateMs);
    expectedEntry2.setExpiryDateMs(newExpiryDateMs);

    bool onSuccessCalled = false;
    _localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const exceptions::ProviderRuntimeException&) { FAIL(); });

    EXPECT_TRUE(onSuccessCalled);

    // check that the dates are also updated in local store and global cache
    auto onSuccess = [this, expectedEntry1](const types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_TRUE(compareDiscoveryEntries(expectedEntry1, result));
        _semaphore->notify();
    };

    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        _discoveryQos,
                                        std::vector<std::string>{},
                                        onSuccess,
                                        nullptr);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        _discoveryQos,
                                        std::vector<std::string>{},
                                        onSuccess,
                                        nullptr);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       doNotReregisterDiscoveryEntriesFromGlobalCapabilitiesDirectory)
{
    types::DiscoveryEntry entry1(_defaultProviderVersion,
                                 _DOMAIN_1_NAME,
                                 _INTERFACE_1_NAME,
                                 _dummyParticipantIdsVector[0],
                                 types::ProviderQos(),
                                 _lastSeenDateMs,
                                 _lastSeenDateMs + _defaultExpiryIntervalMs,
                                 _PUBLIC_KEY_ID);
    types::DiscoveryEntry expectedEntry1(entry1);

    types::DiscoveryEntry entry2(_defaultProviderVersion,
                                 _DOMAIN_2_NAME,
                                 _INTERFACE_2_NAME,
                                 _dummyParticipantIdsVector[1],
                                 types::ProviderQos(),
                                 _lastSeenDateMs,
                                 _lastSeenDateMs + _defaultExpiryIntervalMs,
                                 _PUBLIC_KEY_ID);

    Sequence s;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(entry1)),
                    _,
                    _,
                    _,
                    _,
                    _))
            .Times(1)
            .InSequence(s);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(AllOf(Property(&joynr::types::GlobalDiscoveryEntry::getParticipantId,
                                   Eq(entry1.getParticipantId())),
                          Matcher<const types::GlobalDiscoveryEntry&>(
                                  DiscoveryEntryMatcher(expectedEntry1))),
                    _,
                    _,
                    _,
                    _,
                    _))
            .Times(1)
            .InSequence(s);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Property(&joynr::types::GlobalDiscoveryEntry::getParticipantId,
                             Eq(entry2.getParticipantId())),
                    _,
                    _,
                    _,
                    _,
                    _))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry1, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    _localCapabilitiesDirectory->registerReceivedCapabilities(
            {{_EXTERNAL_ADDRESSES_VECTOR[0], entry2}});

    // sleep to get new values of lastSeenDateMs and expiryDateMs in
    // triggerGlobalProviderReregistration
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const std::int64_t newLastSeenDateMs = TimePoint::now().toMilliseconds();
    const std::int64_t newExpiryDateMs = newLastSeenDateMs + _defaultExpiryIntervalMs;

    expectedEntry1.setLastSeenDateMs(newLastSeenDateMs);
    expectedEntry1.setExpiryDateMs(newExpiryDateMs);

    bool onSuccessCalled = false;
    _localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const exceptions::ProviderRuntimeException&) { FAIL(); });

    EXPECT_TRUE(onSuccessCalled);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       reregisterGlobalCapabilities_BackendNotCalledIfNoGlobalProvidersArePresent)
{
    types::ProviderQos localProviderQos({}, 1, types::ProviderScope::LOCAL, false);
    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                localProviderQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    // sleep to get new values of lastSeenDateMs and expiryDateMs in
    // triggerGlobalProviderReregistration
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    bool onSuccessCalled = false;
    _localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const exceptions::ProviderRuntimeException&) { FAIL(); });

    EXPECT_TRUE(onSuccessCalled);
}

TEST_F(LocalCapabilitiesDirectoryTest,
       reregisterGlobalCapabilities_updatesExpiryDateAndLastSeenDateOfLocalEntries)
{
    types::ProviderQos localProviderQos({}, 1, types::ProviderScope::LOCAL, false);
    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                localProviderQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    types::DiscoveryEntry expectedEntry(entry);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    // sleep to get new values of lastSeenDateMs and expiryDateMs in
    // triggerGlobalProviderReregistration
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const std::int64_t newLastSeenDateMs = TimePoint::now().toMilliseconds();
    const std::int64_t newExpiryDateMs = newLastSeenDateMs + _defaultExpiryIntervalMs;
    expectedEntry.setLastSeenDateMs(newLastSeenDateMs);
    expectedEntry.setExpiryDateMs(newExpiryDateMs);

    bool onSuccessCalled = false;
    _localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const exceptions::ProviderRuntimeException&) { FAIL(); });

    EXPECT_TRUE(onSuccessCalled);

    auto onSuccess = [this, expectedEntry](const types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_TRUE(compareDiscoveryEntries(expectedEntry, result));
        _semaphore->notify();
    };

    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        _discoveryQos,
                                        std::vector<std::string>{},
                                        onSuccess,
                                        nullptr);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addAddsToCache)
{
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(_dummyParticipantIdsVector[0],
                       _,
                       _,
                       A<std::function<void(
                               const std::vector<types::GlobalDiscoveryEntry>& capabilities)>>(),
                       _,
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(1);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                types::ProviderQos(),
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(
            _entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    const types::DiscoveryQos localDiscoveryQos;
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addLocallyDoesNotCallCapabilitiesClient)
{
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    const types::DiscoveryQos localDiscoveryQos;
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressReturnsCachedValues)
{
    // simulate global capability directory would store two entries.
    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(onSuccessResult))
            .RetiresOnSaturation();

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(2),
                                        _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    // enries are now in cache, _globalCapabilitiesDirectoryClient should not be called.
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(2),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressDelegatesToCapabilitiesClient)
{
    // simulate global capability directory would store two entries.
    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(onSuccessResult));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    auto onSuccess = [this](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(2, result.size());

        // check that the results contain the two channel ids
        bool firstParticipantIdFound = false;
        bool secondParticipantIdFound = false;
        for (std::uint16_t i = 0; i < result.size(); i++) {
            types::DiscoveryEntryWithMetaInfo entry = result.at(i);
            EXPECT_EQ(_DOMAIN_1_NAME, entry.getDomain());
            EXPECT_EQ(_INTERFACE_1_NAME, entry.getInterfaceName());
            std::string participantId = entry.getParticipantId();
            if (participantId == _dummyParticipantIdsVector[0]) {
                firstParticipantIdFound = true;
            } else if (participantId == _dummyParticipantIdsVector[1]) {
                secondParticipantIdFound = true;
            }
        }

        EXPECT_TRUE(firstParticipantIdFound);
        EXPECT_TRUE(secondParticipantIdFound);
        _semaphore->notify();
    };

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        _KNOWN_GBIDS,
                                        onSuccess,
                                        _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForParticipantIdReturnsCachedValues)
{
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Invoke(
                    this, &LocalCapabilitiesDirectoryTest::fakeLookupByParticipantIdWithResult))
            .RetiresOnSaturation();

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForParticipantIdReturnsNoCapability)
{
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(
                    Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupNoEntryForParticipant));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _localCapabilitiesDirectory->lookup(
            _dummyParticipantIdsVector[0],
            localDiscoveryQos,
            _KNOWN_GBIDS,
            createUnexpectedLookupParticipantIdSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT));
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForParticipantIdDelegatesToCapabilitiesClient)
{

    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_dummyParticipantIdsVector[0],
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Invoke(
                    this, &LocalCapabilitiesDirectoryTest::fakeLookupByParticipantIdWithResult));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, clearRemovesEntries)
{
    Sequence s;
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .InSequence(s)
            .WillOnce(Invoke(
                    this, &LocalCapabilitiesDirectoryTest::fakeLookupByParticipantIdWithResult));
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .InSequence(s);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectoryStore->clear();
    // retrieving capabilities will force a call to the backend as the cache is empty
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createUnexpectedLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerMultipleGlobalCapabilitiesCheckIfTheyAreMerged)
{

    types::ProviderQos qos;

    types::GlobalDiscoveryEntry globalDiscoveryEntryInfo1(
            _defaultProviderVersion,
            _DOMAIN_1_NAME,
            _INTERFACE_1_NAME,
            _dummyParticipantIdsVector[0],
            qos,
            _lastSeenDateMs,
            _lastSeenDateMs + _defaultExpiryIntervalMs,
            _PUBLIC_KEY_ID,
            _LOCAL_ADDRESS);

    types::GlobalDiscoveryEntry globalDiscoveryEntryInfo2(
            _defaultProviderVersion,
            _DOMAIN_2_NAME,
            _INTERFACE_1_NAME,
            _dummyParticipantIdsVector[1],
            qos,
            _lastSeenDateMs,
            _lastSeenDateMs + _defaultExpiryIntervalMs,
            _PUBLIC_KEY_ID,
            _LOCAL_ADDRESS);

    {
        InSequence inSequence;
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    add(GlobalDiscoveryEntryMatcher(globalDiscoveryEntryInfo1), _, _, _, _, _))
                .Times(1);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    add(GlobalDiscoveryEntryMatcher(globalDiscoveryEntryInfo2), _, _, _, _, _))
                .Times(1);
    }

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                qos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(
            _entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    types::DiscoveryEntry entry2(_defaultProviderVersion,
                                 _DOMAIN_2_NAME,
                                 _INTERFACE_1_NAME,
                                 _dummyParticipantIdsVector[1],
                                 qos,
                                 _lastSeenDateMs,
                                 _lastSeenDateMs + _defaultExpiryIntervalMs,
                                 _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(
            entry2, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
}

TEST_F(LocalCapabilitiesDirectoryTest, removeLocalCapabilityByParticipantId)
{
    types::ProviderQos providerQos = types::ProviderQos();
    providerQos.setScope(types::ProviderScope::LOCAL);
    Sequence s;
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0)
            .InSequence(s);
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .InSequence(s)
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

    EXPECT_THROW(_localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                                     localDiscoveryQos,
                                                     _KNOWN_GBIDS,
                                                     createLookupParticipantIdSuccessFunction(),
                                                     _unexpectedOnDiscoveryErrorFunction),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(0),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocalThenGlobal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // disable cache
    localDiscoveryQos.setCacheMaxAge(0);
    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_THROW(_localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                                     _INTERFACE_1_NAME,
                                                     localDiscoveryQos,
                                                     _KNOWN_GBIDS,
                                                     createLookupSuccessFunction(1),
                                                     _unexpectedOnDiscoveryErrorFunction),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocalAndGlobal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessZeroResult{};
    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    Sequence s;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(2)
            .InSequence(s)
            .WillRepeatedly(InvokeArgument<4>(onSuccessZeroResult));
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .InSequence(s)
            .WillOnce(InvokeArgument<4>(onSuccessResult));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(0),
                                        _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // disable cache
    localDiscoveryQos.setCacheMaxAge(0);
    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(2),
                                        _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectoryStore->clear();

    localDiscoveryQos.setCacheMaxAge(4000);
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalPendingLocalEntryAdded_ReturnsLocalEntry)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    auto gcdSemaphore = std::make_shared<Semaphore>(0);
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(
                    DoAll(AcquireSemaphore(gcdSemaphore), InvokeArgument<4>(onSuccessResult)));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    auto thread = std::thread([&]() {
        _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                                            _INTERFACE_1_NAME,
                                            localDiscoveryQos,
                                            _KNOWN_GBIDS,
                                            createLookupSuccessFunction(1),
                                            _unexpectedOnDiscoveryErrorFunction);
    });

    EXPECT_FALSE(_semaphore->waitFor(std::chrono::milliseconds(100)));
    EXPECT_TRUE(_localCapabilitiesDirectory->hasPendingLookups());

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());

    // _globalCapabilitiesDirectoryClient.lookup should return;
    gcdSemaphore->notify();
    thread.join();

    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalSucceedsNoLocalEntries_ReturnsGlobalEntries)
{
    auto gcdSemaphore = std::make_shared<Semaphore>(0);
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(
                    DoAll(AcquireSemaphore(gcdSemaphore), InvokeArgument<4>(onSuccessResult)));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    auto thread = std::thread([&]() {
        _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                                            _INTERFACE_1_NAME,
                                            localDiscoveryQos,
                                            _KNOWN_GBIDS,
                                            createLookupSuccessFunction(2),
                                            _unexpectedOnDiscoveryErrorFunction);
    });

    EXPECT_FALSE(_semaphore->waitFor(std::chrono::milliseconds(100)));
    EXPECT_TRUE(_localCapabilitiesDirectory->hasPendingLookups());

    // _globalCapabilitiesDirectoryClient.lookup should return;
    gcdSemaphore->notify();
    thread.join();

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    auto gcdSemaphore = std::make_shared<Semaphore>(0);
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(DoAll(
                    AcquireSemaphore(gcdSemaphore),
                    Invoke(this, &LocalCapabilitiesDirectoryTest::
                                         fakeCapabilitiesClientLookupWithDiscoveryException)));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    auto thread = std::thread([&]() {
        _localCapabilitiesDirectory->lookup(
                {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                _INTERFACE_1_NAME,
                localDiscoveryQos,
                _KNOWN_GBIDS,
                createUnexpectedLookupSuccessFunction(),
                createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    });

    EXPECT_FALSE(_semaphore->waitFor(std::chrono::milliseconds(100)));
    EXPECT_TRUE(_localCapabilitiesDirectory->hasPendingLookups());

    // _globalCapabilitiesDirectoryClient.lookup should return;
    gcdSemaphore->notify();
    thread.join();

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_LocalEntriesNoGlobalLookup_ReturnsLocalEntry)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos localDiscoveryQosryQos;
    localDiscoveryQosryQos.setCacheMaxAge(5000);
    localDiscoveryQosryQos.setDiscoveryTimeout(5000);
    localDiscoveryQosryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQosryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalSucceedsNoLocalEntries_ReturnsGlobalEntries)
{
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(InvokeArgument<4>(onSuccessResult));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(2),
                                        _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(
                    Invoke(this, &LocalCapabilitiesDirectoryTest::
                                         fakeCapabilitiesClientLookupWithDiscoveryException));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->lookup(
            {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
            _INTERFACE_1_NAME,
            localDiscoveryQos,
            _KNOWN_GBIDS,
            createUnexpectedLookupSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalSucceedsLocalEntries_ReturnsLocalAndGlobalEntries)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[2],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(InvokeArgument<4>(onSuccessResult));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(3),
                                        _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalFailsLocalEntries_ReturnsNoEntries)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(
                    Invoke(this, &LocalCapabilitiesDirectoryTest::
                                         fakeCapabilitiesClientLookupWithDiscoveryException));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->lookup(
            {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
            _INTERFACE_1_NAME,
            localDiscoveryQos,
            _KNOWN_GBIDS,
            createUnexpectedLookupSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupGlobalOnly_GlobalSucceedsNoLocalEntries_ReturnsGlobalEntries)
{
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(InvokeArgument<4>(onSuccessResult));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(2),
                                        _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest, removeGlobalExpiredEntries_ReturnNonExpiredGlobalEntries)
{
    // make sure that there is only one runtime that is periodically calling touch
    test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);
    configureShortCapabilitiesFreshnessUpdateInterval();
    configureShortPurgeExpiredDiscoveryEntriesInterval();
    // add a few (remote) entries to the global cache
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    std::int64_t longerExpiryDateMs =
            _lastSeenDateMs + _purgeExpiredDiscoveryEntriesIntervalMs * 10;
    std::int64_t shorterExpiryDateMs =
            _lastSeenDateMs + _purgeExpiredDiscoveryEntriesIntervalMs / 3;

    types::GlobalDiscoveryEntry entry1(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       providerQos,
                                       _lastSeenDateMs,
                                       longerExpiryDateMs,
                                       _PUBLIC_KEY_ID,
                                       _EXTERNAL_ADDRESSES_VECTOR[0]);

    types::GlobalDiscoveryEntry entry2(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[1],
                                       providerQos,
                                       _lastSeenDateMs,
                                       shorterExpiryDateMs,
                                       _PUBLIC_KEY_ID,
                                       _EXTERNAL_ADDRESSES_VECTOR[0]);

    types::GlobalDiscoveryEntry entry3(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_3_NAME,
                                       _dummyParticipantIdsVector[2],
                                       providerQos,
                                       _lastSeenDateMs,
                                       shorterExpiryDateMs,
                                       _PUBLIC_KEY_ID,
                                       _EXTERNAL_ADDRESSES_VECTOR[1]);

    std::vector<types::GlobalDiscoveryEntry> expectedResultInterface1 = {entry1, entry2};
    std::vector<types::GlobalDiscoveryEntry> expectedResultInterface3 = {entry3};
    std::vector<types::GlobalDiscoveryEntry> emptyExpectedResult = {};

    const std::vector<std::string> domainVector = {_DOMAIN_1_NAME};

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(10000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    // lookups for _INTERFACE_1_NAME
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(Eq(domainVector), Eq(_INTERFACE_1_NAME), _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(expectedResultInterface1))
            .RetiresOnSaturation();

    // lookups for _INTERFACE_3_NAME
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, lookup(_, Eq(_INTERFACE_3_NAME), _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(emptyExpectedResult));
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(Eq(domainVector), Eq(_INTERFACE_3_NAME), _, _, _, _, _))
            .WillOnce(InvokeArgument<4>(expectedResultInterface3))
            .RetiresOnSaturation();

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->lookup(domainVector,
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(2),
                                        _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->lookup(domainVector,
                                        _INTERFACE_3_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(_purgeExpiredDiscoveryEntriesIntervalMs * 2));
    // cache should now only contain entry1

    // 1 cached entry still available for interface 1
    _localCapabilitiesDirectory->lookup(domainVector,
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // The only entry for interface 3 is expired by now, so the GCD is called again
    _localCapabilitiesDirectory->lookup(domainVector,
                                        _INTERFACE_3_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(0),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupGlobalOnly_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(
                    Invoke(this, &LocalCapabilitiesDirectoryTest::
                                         fakeCapabilitiesClientLookupWithDiscoveryException));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->lookup(
            {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
            _INTERFACE_1_NAME,
            localDiscoveryQos,
            _KNOWN_GBIDS,
            createUnexpectedLookupSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterface_globalOnly_localOnlyEntry_gcdSucceeds_returnsFilteredGlobalEntries)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);

    // the remote entry with the same participantId as the local entry will be ignored
    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(InvokeArgument<4>(onSuccessResult));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupGlobalOnly_GlobalFailsLocalEntries_ReturnsNoEntries)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(
                    Invoke(this, &LocalCapabilitiesDirectoryTest::
                                         fakeCapabilitiesClientLookupWithDiscoveryException));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->lookup(
            {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
            _INTERFACE_1_NAME,
            localDiscoveryQos,
            _KNOWN_GBIDS,
            createUnexpectedLookupSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupMultipeDomainsReturnsResultForMultipleDomains)
{
    std::string multipleDomainName1 = "multipleDomainName1";
    std::string multipleDomainName2 = "multipleDomainName2";
    std::string multipleDomainName3 = "multipleDomainName3";
    std::string multipleDomainName1ParticipantId = util::createUuid();
    std::string multipleDomainName2ParticipantId = util::createUuid();
    std::string multipleDomainName3ParticipantId1 = util::createUuid();
    std::string multipleDomainName3ParticipantId2 = util::createUuid();
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryEntry entry1(_defaultProviderVersion,
                                 multipleDomainName1,
                                 _INTERFACE_1_NAME,
                                 multipleDomainName1ParticipantId,
                                 providerQos,
                                 _lastSeenDateMs,
                                 _lastSeenDateMs + _defaultExpiryIntervalMs,
                                 _PUBLIC_KEY_ID);
    types::DiscoveryEntry entry2(_defaultProviderVersion,
                                 multipleDomainName2,
                                 _INTERFACE_1_NAME,
                                 multipleDomainName2ParticipantId,
                                 providerQos,
                                 _lastSeenDateMs,
                                 _lastSeenDateMs + _defaultExpiryIntervalMs,
                                 _PUBLIC_KEY_ID);
    types::DiscoveryEntry entry31(_defaultProviderVersion,
                                  multipleDomainName3,
                                  _INTERFACE_1_NAME,
                                  multipleDomainName3ParticipantId1,
                                  providerQos,
                                  _lastSeenDateMs,
                                  _lastSeenDateMs + _defaultExpiryIntervalMs,
                                  _PUBLIC_KEY_ID);
    types::DiscoveryEntry entry32(_defaultProviderVersion,
                                  multipleDomainName3,
                                  _INTERFACE_1_NAME,
                                  multipleDomainName3ParticipantId2,
                                  providerQos,
                                  _lastSeenDateMs,
                                  _lastSeenDateMs + _defaultExpiryIntervalMs,
                                  _PUBLIC_KEY_ID);

    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry1, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->add(
            entry2, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->add(
            entry31, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->add(
            entry32, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    std::vector<std::string> domains = {
            multipleDomainName1, multipleDomainName2, multipleDomainName3};

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    _localCapabilitiesDirectory->lookup(domains,
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(4),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupMultipleDomains_localThenGlobal_oneLocalGlobalOneCached_sameParticipantIdsRemote)
{
    const std::uint64_t ONE_DAY_IN_MS = 1 * 24 * 60 * 60 * 1000;
    const std::string localDomain = "localDomain";
    const std::string cachedDomain = "cachedDomain";
    const std::string remoteDomain = "remoteDomain";
    const std::string participantId1 = "participantId1";
    const std::string participantId2 = "participantId2";
    const std::string participantId3 = "participantId3";
    std::vector<std::string> domains = {localDomain, cachedDomain, remoteDomain};

    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

    // local entry for participantId1 and domain1
    types::DiscoveryEntry discoveryEntry(_defaultProviderVersion,
                                         localDomain,
                                         _INTERFACE_1_NAME,
                                         participantId1,
                                         types::ProviderQos(),
                                         _lastSeenDateMs,
                                         _lastSeenDateMs + _defaultExpiryIntervalMs,
                                         _PUBLIC_KEY_ID);

    const std::vector<InterfaceAddress>& interfaceAddresses =
            LCDUtil::getInterfaceAddresses(domains, _INTERFACE_1_NAME);

    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                getLocalAndCachedCapabilities(
                        Matcher<const std::vector<InterfaceAddress>&>(interfaceAddresses),
                        _,
                        Eq(_KNOWN_GBIDS),
                        _))
            .Times(1)
            .WillOnce(Return(false));

    std::vector<types::DiscoveryEntry> localEntries = {discoveryEntry};
    EXPECT_CALL(
            *_mockLocalCapabilitiesDirectoryStore,
            getLocalCapabilities(Matcher<const std::vector<InterfaceAddress>&>(interfaceAddresses)))
            .Times(1)
            .WillOnce(Return(localEntries));

    // cached entry for local provider and cached entry for participantId2 for cachedDomain
    types::GlobalDiscoveryEntry cachedLocalEntry =
            LCDUtil::toGlobalDiscoveryEntry(discoveryEntry, _LOCAL_ADDRESS);
    types::GlobalDiscoveryEntry cachedRemoteEntry;
    cachedRemoteEntry.setParticipantId(participantId2);
    cachedRemoteEntry.setInterfaceName(_INTERFACE_1_NAME);
    cachedRemoteEntry.setDomain(cachedDomain);
    cachedRemoteEntry.setAddress(_LOCAL_ADDRESS);

    // remote entries for local provider and for remoteDomain for participantIds 2 and 3
    types::GlobalDiscoveryEntry remoteEntry1(cachedLocalEntry);
    remoteEntry1.setDomain(remoteDomain);
    types::GlobalDiscoveryEntry remoteEntry2 = types::GlobalDiscoveryEntry(cachedRemoteEntry);
    remoteEntry2.setDomain(remoteDomain);
    types::GlobalDiscoveryEntry remoteEntry3 = types::GlobalDiscoveryEntry(cachedRemoteEntry);
    remoteEntry3.setParticipantId(participantId3);
    remoteEntry3.setDomain(remoteDomain);

    std::vector<types::GlobalDiscoveryEntry> globalLookupResult = {
            remoteEntry1, remoteEntry2, remoteEntry3};

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(Eq(domains), Eq(_INTERFACE_1_NAME), _, _, _, _, _))
            .WillOnce(InvokeArgument<4>(globalLookupResult));

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                getLocalCapabilities(Matcher<const std::string&>(participantId1)))
            .Times(1)
            .WillOnce(Return(localEntries));

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                getLocalCapabilities(Matcher<const std::string&>(participantId2)))
            .Times(1)
            .WillOnce(Return(std::vector<types::DiscoveryEntry>{}));

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                getLocalCapabilities(Matcher<const std::string&>(participantId3)))
            .Times(1)
            .WillOnce(Return(std::vector<types::DiscoveryEntry>{}));

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                insertInGlobalLookupCache(
                        Property(&joynr::types::GlobalDiscoveryEntry::getParticipantId,
                                 Eq(remoteEntry1.getParticipantId())),
                        _))
            .Times(0);

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                insertInGlobalLookupCache(
                        Property(&joynr::types::GlobalDiscoveryEntry::getParticipantId,
                                 Eq(remoteEntry2.getParticipantId())),
                        _))
            .Times(1);

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                insertInGlobalLookupCache(
                        Property(&joynr::types::GlobalDiscoveryEntry::getParticipantId,
                                 Eq(remoteEntry3.getParticipantId())),
                        _))
            .Times(1);

    system::RoutingTypes::MqttAddress mqttAddress(_KNOWN_GBIDS[0], "localTopic");
    EXPECT_CALL(*_mockMessageRouter, addNextHop(Eq(participantId1), _, _, _, _, _, _)).Times(0);

    EXPECT_CALL(*_mockMessageRouter,
                addNextHop(participantId2, pointerToMqttAddress(mqttAddress), _, _, _, _, _))
            .Times(1);

    EXPECT_CALL(*_mockMessageRouter,
                addNextHop(participantId3, pointerToMqttAddress(mqttAddress), _, _, _, _, _))
            .Times(1);

    const int expectedNumberOfEntries = 3;
    auto onSuccess = [this, expectedNumberOfEntries, discoveryEntry, remoteEntry2, remoteEntry3,
                      participantId1, participantId2, participantId3, localDomain,
                      remoteDomain](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(expectedNumberOfEntries, result.size());
        bool discoveryEntryFound = false;
        bool remoteEntry2Found = false;
        bool remoteEntry3Found = false;
        for (auto entry : result) {
            if (entry.getParticipantId() == participantId1 && entry.getDomain() == localDomain &&
                entry.getIsLocal() == true) {
                discoveryEntryFound = true;
            }
            if (entry.getParticipantId() == participantId2 && entry.getDomain() == remoteDomain &&
                entry.getIsLocal() == false) {
                remoteEntry2Found = true;
            }
            if (entry.getParticipantId() == participantId3 && entry.getDomain() == remoteDomain &&
                entry.getIsLocal() == false) {
                remoteEntry3Found = true;
            }
        }

        EXPECT_TRUE(discoveryEntryFound && remoteEntry2Found && remoteEntry3Found);
        _semaphore->notify();
    };

    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->lookup(domains,
                                                          _INTERFACE_1_NAME,
                                                          discoveryQos,
                                                          _KNOWN_GBIDS,
                                                          onSuccess,
                                                          _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceGbids_localAndGlobal_localGlobalEntry_invokesGcd_filtersCombinedResult)
{
    const std::string localDomain = "localDomain";
    const std::string participantId1 = "participantId1";
    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);
    discoveryQos.setCacheMaxAge(10000);
    discoveryQos.setDiscoveryTimeout(10000);
    types::DiscoveryEntry discoveryEntry(_defaultProviderVersion,
                                         localDomain,
                                         _INTERFACE_1_NAME,
                                         participantId1,
                                         types::ProviderQos(),
                                         _lastSeenDateMs,
                                         _lastSeenDateMs + _defaultExpiryIntervalMs,
                                         _PUBLIC_KEY_ID);

    types::GlobalDiscoveryEntry globalDiscoveryEntry =
            LCDUtil::toGlobalDiscoveryEntry(discoveryEntry, _LOCAL_ADDRESS);

    std::vector<std::string> domains = {discoveryEntry.getDomain()};
    std::vector<types::DiscoveryEntry> localDiscoveryEntries = {discoveryEntry};
    std::vector<types::GlobalDiscoveryEntry> globalDiscoveryEntries = {globalDiscoveryEntry};

    const std::vector<InterfaceAddress>& interfaceAddresses =
            LCDUtil::getInterfaceAddresses(domains, _INTERFACE_1_NAME);

    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                getLocalAndCachedCapabilities(
                        Matcher<const std::vector<InterfaceAddress>&>(interfaceAddresses),
                        _,
                        Eq(_KNOWN_GBIDS),
                        _))
            .Times(1)
            .WillOnce(Return(false));

    EXPECT_CALL(
            *_mockLocalCapabilitiesDirectoryStore,
            getLocalCapabilities(Matcher<const std::vector<InterfaceAddress>&>(interfaceAddresses)))
            .Times(1)
            .WillOnce(Return(localDiscoveryEntries));

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                getLocalCapabilities(Matcher<const std::string&>(participantId1)))
            .Times(1)
            .WillOnce(Return(localDiscoveryEntries));

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore, insertInGlobalLookupCache(_, _)).Times(0);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(Eq(domains), Eq(_INTERFACE_1_NAME), _, _, _, _, _))
            .WillOnce(InvokeArgument<4>(globalDiscoveryEntries));

    EXPECT_CALL(*_mockMessageRouter, addNextHop(_, _, _, _, _, _, _)).Times(0);

    const int expectedNumberOfEntries = 1;
    auto onSuccess = [this, expectedNumberOfEntries, discoveryEntry, participantId1,
                      localDomain](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(expectedNumberOfEntries, result.size());
        EXPECT_EQ(result[0].getParticipantId(), participantId1);
        EXPECT_EQ(result[0].getDomain(), localDomain);
        EXPECT_TRUE(result[0].getIsLocal());

        _semaphore->notify();
    };

    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->lookup(domains,
                                                          _INTERFACE_1_NAME,
                                                          discoveryQos,
                                                          _KNOWN_GBIDS,
                                                          onSuccess,
                                                          _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceGbids_globalOnly_localGlobalEntry_invokesGcd_ignoresGlobalDuplicate)
{
    const std::string localDomain = "localDomain";
    const std::string participantId1 = "participantId1";

    types::DiscoveryEntry discoveryEntry(_defaultProviderVersion,
                                         localDomain,
                                         _INTERFACE_1_NAME,
                                         participantId1,
                                         types::ProviderQos(),
                                         _lastSeenDateMs,
                                         _lastSeenDateMs + _defaultExpiryIntervalMs,
                                         _PUBLIC_KEY_ID);

    std::vector<std::string> domains = {discoveryEntry.getDomain()};

    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    discoveryQos.setCacheMaxAge(10000);
    discoveryQos.setDiscoveryTimeout(5000);

    const std::vector<InterfaceAddress>& interfaceAddresses =
            LCDUtil::getInterfaceAddresses(domains, _INTERFACE_1_NAME);

    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                getLocalAndCachedCapabilities(
                        Matcher<const std::vector<InterfaceAddress>&>(interfaceAddresses),
                        _,
                        Eq(_KNOWN_GBIDS),
                        _))
            .Times(1)
            .WillOnce(Return(false));

    std::vector<types::DiscoveryEntry> localEntries = {discoveryEntry};
    EXPECT_CALL(
            *_mockLocalCapabilitiesDirectoryStore,
            getLocalCapabilities(Matcher<const std::vector<InterfaceAddress>&>(interfaceAddresses)))
            .Times(1)
            .WillOnce(Return(localEntries));

    types::GlobalDiscoveryEntry globalDiscoveryEntry =
            LCDUtil::toGlobalDiscoveryEntry(discoveryEntry, _LOCAL_ADDRESS);

    std::vector<types::GlobalDiscoveryEntry> globalDiscoveryEntries = {globalDiscoveryEntry};

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(Eq(domains),
                                                            Eq(_INTERFACE_1_NAME),
                                                            _,
                                                            Eq(discoveryQos.getDiscoveryTimeout()),
                                                            _,
                                                            _,
                                                            _))
            .WillOnce(InvokeArgument<4>(globalDiscoveryEntries));

    std::vector<types::DiscoveryEntry> localDiscoveryEntries = {discoveryEntry};
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                getLocalCapabilities(Matcher<const std::string&>(participantId1)))
            .Times(1)
            .WillOnce(Return(localDiscoveryEntries));

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore, insertInGlobalLookupCache(_, _)).Times(0);

    EXPECT_CALL(*_mockMessageRouter, addNextHop(_, _, _, _, _, _, _)).Times(0);

    const int expectedNumberOfEntries = 1;
    auto onSuccess = [this, expectedNumberOfEntries, participantId1,
                      localDomain](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(expectedNumberOfEntries, result.size());
        EXPECT_EQ(result[0].getParticipantId(), participantId1);
        EXPECT_EQ(result[0].getDomain(), localDomain);
        EXPECT_TRUE(result[0].getIsLocal());
        _semaphore->notify();
    };

    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->lookup(domains,
                                                          _INTERFACE_1_NAME,
                                                          discoveryQos,
                                                          _KNOWN_GBIDS,
                                                          onSuccess,
                                                          _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdGbids_globalOnly_localGlobalEntry_invokesGcd_ignoresGlobalDuplicate)
{
    const std::string localDomain = "localDomain";
    const std::string participantId1 = "participantId1";

    types::DiscoveryEntry discoveryEntry(_defaultProviderVersion,
                                         localDomain,
                                         _INTERFACE_1_NAME,
                                         participantId1,
                                         types::ProviderQos(),
                                         _lastSeenDateMs,
                                         _lastSeenDateMs + _defaultExpiryIntervalMs,
                                         _PUBLIC_KEY_ID);

    std::vector<std::string> domains = {discoveryEntry.getDomain()};

    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    discoveryQos.setCacheMaxAge(10000);
    discoveryQos.setDiscoveryTimeout(5000);

    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                getLocalAndCachedCapabilities(
                        Matcher<const std::string&>(participantId1), _, Eq(_KNOWN_GBIDS), _))
            .Times(1)
            .WillOnce(Return(false));

    std::vector<types::DiscoveryEntry> localEntries = {discoveryEntry};
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                getLocalCapabilities(Matcher<const std::string&>(participantId1)))
            .Times(2)
            .WillRepeatedly(Return(localEntries));

    types::GlobalDiscoveryEntry globalDiscoveryEntry =
            LCDUtil::toGlobalDiscoveryEntry(discoveryEntry, _LOCAL_ADDRESS);

    std::vector<types::GlobalDiscoveryEntry> globalDiscoveryEntries = {globalDiscoveryEntry};

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(Eq(discoveryEntry.getParticipantId()),
                                                            Eq(_KNOWN_GBIDS),
                                                            Eq(discoveryQos.getDiscoveryTimeout()),
                                                            _,
                                                            _,
                                                            _))
            .WillOnce(InvokeArgument<3>(globalDiscoveryEntries));

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore, insertInGlobalLookupCache(_, _)).Times(0);

    EXPECT_CALL(*_mockMessageRouter, addNextHop(_, _, _, _, _, _, _)).Times(0);

    auto onSuccess = [this, participantId1,
                      localDomain](const types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_EQ(result.getParticipantId(), participantId1);
        EXPECT_EQ(result.getDomain(), localDomain);
        EXPECT_TRUE(result.getIsLocal());

        _semaphore->notify();
    };

    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->lookup(discoveryEntry.getParticipantId(),
                                                          discoveryQos,
                                                          _KNOWN_GBIDS,
                                                          onSuccess,
                                                          _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupGlobalOnly)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);

    EXPECT_CALL(*_mockMessageRouter, addNextHop(_, _, _, _, _, _, _)).Times(1);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(0);
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(2)
            .WillRepeatedly(
                    InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    EXPECT_THROW(_localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                                     _INTERFACE_1_NAME,
                                                     localDiscoveryQos,
                                                     _KNOWN_GBIDS,
                                                     createUnexpectedLookupSuccessFunction(),
                                                     _unexpectedOnDiscoveryErrorFunction),
                 exceptions::JoynrTimeOutException);

    // register the external capability
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));
    // get the global entry
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

    // disable cache
    localDiscoveryQos.setCacheMaxAge(0);
    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_THROW(_localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                                     _INTERFACE_1_NAME,
                                                     localDiscoveryQos,
                                                     _KNOWN_GBIDS,
                                                     createUnexpectedLookupSuccessFunction(),
                                                     _unexpectedOnDiscoveryErrorFunction),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupLocal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(1);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .Times(1);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupLocalThenGlobal)
{
    // make sure that there is only one runtime that is periodically calling touch
    test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);
    configureShortCapabilitiesFreshnessUpdateInterval();
    configureShortPurgeExpiredDiscoveryEntriesInterval();
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(100);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(1);
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .WillOnce(InvokeArgument<2>());
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));

    // get the local entry
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

    // get the global entry
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(_purgeExpiredDiscoveryEntriesIntervalMs * 2));
    // get the global, but timeout occured
    EXPECT_THROW(_localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                                     _INTERFACE_1_NAME,
                                                     localDiscoveryQos,
                                                     _KNOWN_GBIDS,
                                                     createUnexpectedLookupSuccessFunction(),
                                                     _unexpectedOnDiscoveryErrorFunction),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupGlobalOnly)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(100);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);

    types::GlobalDiscoveryEntry globalEntry =
              LCDUtil::toGlobalDiscoveryEntry(entry, _LOCAL_ADDRESS);
    const std::vector<types::GlobalDiscoveryEntry>& globalEntryVec = {globalEntry};

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(1);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).
            Times(1)
           .WillOnce(InvokeArgument<4>(globalEntryVec));
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _))
            .Times(1);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // recieve a global entry
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(2),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerReceivedCapabilites_registerMqttAddress)
{
    const std::string& addressType = "mqtt";
    const std::string& topic = "mqtt_TEST_channelId";
    system::RoutingTypes::MqttAddress mqttAddress("brokerUri", topic);
    registerReceivedCapabilities(addressType, serializer::serializeToJson(mqttAddress));
}

TEST_F(LocalCapabilitiesDirectoryTest, persistencyTest)
{
    // the mock is used by two different localCapabiltiesDirectories
    // this is dangerous and should not be done
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _))
            .WillRepeatedly(InvokeArgument<4>(types::DiscoveryError::INTERNAL_ERROR));

    initializeMockLocalCapabilitiesDirectoryStore();

    _localCapabilitiesDirectory = std::make_shared<LocalCapabilitiesDirectory>(
            _clusterControllerSettingsForPersistencyTests,
            _globalCapabilitiesDirectoryClient,
            _localCapabilitiesDirectoryStoreForPersistencyTests,
            _LOCAL_ADDRESS,
            _mockMessageRouter,
            _singleThreadedIOService->getIOService(),
            _clusterControllerId,
            _KNOWN_GBIDS,
            _defaultExpiryIntervalMs);
    _localCapabilitiesDirectory->init();

    // Attempt loading (action usually performed by cluster-controller runtime)
    _localCapabilitiesDirectory->loadPersistedFile();

    // add few entries
    const std::string& DOMAIN_NAME = "LocalCapabilitiesDirectorySerializerTest_Domain";
    const std::string& INTERFACE_NAME = "LocalCapabilitiesDirectorySerializerTest_InterfaceName";

    std::vector<std::string> participantIds{
            util::createUuid(), util::createUuid(), util::createUuid()};

    types::ProviderQos localProviderQos;
    localProviderQos.setScope(types::ProviderScope::LOCAL);
    types::ProviderQos globalProviderQos;
    globalProviderQos.setScope(types::ProviderScope::GLOBAL);

    const types::DiscoveryEntry entry1(_defaultProviderVersion,
                                       DOMAIN_NAME,
                                       INTERFACE_NAME,
                                       participantIds[0],
                                       localProviderQos,
                                       TimePoint::now().toMilliseconds(),
                                       _lastSeenDateMs + _defaultExpiryIntervalMs,
                                       _PUBLIC_KEY_ID);
    const types::DiscoveryEntry entry2(_defaultProviderVersion,
                                       DOMAIN_NAME,
                                       INTERFACE_NAME,
                                       participantIds[1],
                                       globalProviderQos,
                                       TimePoint::now().toMilliseconds(),
                                       _lastSeenDateMs + _defaultExpiryIntervalMs,
                                       _PUBLIC_KEY_ID);
    const types::DiscoveryEntry entry3(_defaultProviderVersion,
                                       DOMAIN_NAME,
                                       INTERFACE_NAME,
                                       participantIds[2],
                                       globalProviderQos,
                                       TimePoint::now().toMilliseconds(),
                                       _lastSeenDateMs + _defaultExpiryIntervalMs,
                                       _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(entry1,
                                     false,
                                     {_KNOWN_GBIDS[0]},
                                     _defaultOnSuccess,
                                     _unexpectedOnDiscoveryErrorFunction);
    _localCapabilitiesDirectory->add(entry2,
                                     false,
                                     {_KNOWN_GBIDS[0]},
                                     _defaultOnSuccess,
                                     _unexpectedOnDiscoveryErrorFunction);
    _localCapabilitiesDirectory->add(entry3,
                                     false,
                                     {_KNOWN_GBIDS[1], _KNOWN_GBIDS[2]},
                                     _defaultOnSuccess,
                                     _unexpectedOnDiscoveryErrorFunction);

    // create a new object
    const std::int64_t defaultExpiryDateMs = 600000;
    auto localCapabilitiesDirectory2 = std::make_shared<LocalCapabilitiesDirectory>(
            _clusterControllerSettingsForPersistencyTests,
            _globalCapabilitiesDirectoryClient,
            _localCapabilitiesDirectoryStoreForPersistencyTests,
            _LOCAL_ADDRESS,
            _mockMessageRouter,
            _singleThreadedIOService->getIOService(),
            _clusterControllerId,
            _KNOWN_GBIDS,
            defaultExpiryDateMs);
    localCapabilitiesDirectory2->init();

    // load persistency
    localCapabilitiesDirectory2->loadPersistedFile();

    // check all entries are there
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    for (auto& participantID : participantIds) {
        localCapabilitiesDirectory2->lookup(participantID,
                                            localDiscoveryQos,
                                            _KNOWN_GBIDS,
                                            createLookupParticipantIdSuccessFunction(),
                                            _unexpectedOnDiscoveryErrorFunction);
        EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    // global cached entries wont be persisted
    auto cachedGlobalDiscoveryEntries =
            localCapabilitiesDirectory2->getCachedGlobalDiscoveryEntries();
    EXPECT_EQ(0, cachedGlobalDiscoveryEntries.size());

    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    // check entry2
    localCapabilitiesDirectory2->lookup(participantIds[1],
                                        localDiscoveryQos,
                                        {_KNOWN_GBIDS[0]},
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    localCapabilitiesDirectory2->lookup(
            participantIds[1],
            localDiscoveryQos,
            {_KNOWN_GBIDS[1], _KNOWN_GBIDS[2]},
            createLookupParticipantIdSuccessFunction(),
            _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // check entry3
    localCapabilitiesDirectory2->lookup(
            participantIds[2],
            localDiscoveryQos,
            {_KNOWN_GBIDS[0]},
            createLookupParticipantIdSuccessFunction(),
            _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    localCapabilitiesDirectory2->lookup(participantIds[2],
                                        localDiscoveryQos,
                                        {_KNOWN_GBIDS[1]},
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    localCapabilitiesDirectory2->lookup(participantIds[2],
                                        localDiscoveryQos,
                                        {_KNOWN_GBIDS[2]},
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, loadCapabilitiesFromFile)
{
    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();
    const std::string& fileName = "test-resources/ListOfCapabilitiesToInject.json";
    _localCapabilitiesDirectory->injectGlobalCapabilitiesFromFile(fileName);

    // Verify that all entries present in the file have indeed been loaded
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _localCapabilitiesDirectory->lookup("notReachableInterface_Schroedinger",
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->lookup("notReachableInterface_Heisenberg",
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, throwExceptionOnEmptyDomainsVector)
{
    initializeMockLocalCapabilitiesDirectoryStore();
    // there are no expectations for the mocks of _localCapabilitiesDirectory
    finalizeTestSetupAfterMockExpectationsAreDone();

    const std::vector<std::string>& zeroDomains = {};
    const std::vector<std::string>& twoDomains = {_DOMAIN_1_NAME, _DOMAIN_2_NAME};

    // use different MockCallback objects since EXPECT_CALL must be done
    // prior to injecting object, any later modifications are forbidden
    MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>> mockCallback1;
    auto onSuccess1 =
            std::bind(&MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>>::onSuccess,
                      &mockCallback1,
                      std::placeholders::_1);
    auto onError1 =
            std::bind(&MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>>::onError,
                      &mockCallback1,
                      std::placeholders::_1);

    EXPECT_CALL(mockCallback1, onFatalRuntimeError(_)).Times(0);
    EXPECT_CALL(mockCallback1, onError(_)).Times(0);
    EXPECT_CALL(mockCallback1, onSuccess(_)).Times(0);
    EXPECT_THROW(_localCapabilitiesDirectory->lookup(
                         zeroDomains, _INTERFACE_1_NAME, _discoveryQos, onSuccess1, onError1),
                 exceptions::ProviderRuntimeException);

    MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>> mockCallback2;
    auto onSuccess2 =
            std::bind(&MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>>::onSuccess,
                      &mockCallback2,
                      std::placeholders::_1);
    auto onError2 =
            std::bind(&MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>>::onError,
                      &mockCallback2,
                      std::placeholders::_1);
    EXPECT_CALL(mockCallback2, onError(_)).Times(0);
    EXPECT_CALL(mockCallback2, onSuccess(_)).Times(0);
    EXPECT_NO_THROW(_localCapabilitiesDirectory->lookup(
            twoDomains, _INTERFACE_1_NAME, _discoveryQos, onSuccess2, onError2));
}

TEST_F(LocalCapabilitiesDirectoryTest, localAndGlobalDoesNotReturnDuplicateEntriesCacheEnabled)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);
    const types::DiscoveryEntryWithMetaInfo expectedEntry = LCDUtil::convert(true, entry);

    types::GlobalDiscoveryEntry globalEntry =
            LCDUtil::toGlobalDiscoveryEntry(entry, _LOCAL_ADDRESS);
    const std::vector<types::GlobalDiscoveryEntry>& globalEntryVec = {globalEntry};

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(1);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(globalEntryVec));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            _entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    auto onSuccess =
            [this, expectedEntry](const std::vector<types::DiscoveryEntryWithMetaInfo>& results) {
                EXPECT_EQ(1, results.size());
                const types::DiscoveryEntryWithMetaInfo& result = results.at(0);
                EXPECT_TRUE(compareDiscoveryEntries(expectedEntry, result));
                _semaphore->notify();
            };

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        onSuccess,
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, localAndGlobalDoesNotReturnDuplicateEntriesCacheDisabled)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                providerQos,
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);

    const types::DiscoveryEntryWithMetaInfo& expectedEntry = LCDUtil::convert(true, entry);
    const types::GlobalDiscoveryEntry& globalEntry =
            types::GlobalDiscoveryEntry(entry.getProviderVersion(),
                                        entry.getDomain(),
                                        entry.getInterfaceName(),
                                        entry.getParticipantId(),
                                        entry.getQos(),
                                        entry.getLastSeenDateMs(),
                                        entry.getExpiryDateMs(),
                                        entry.getPublicKeyId(),
                                        _EXTERNAL_ADDRESSES_VECTOR[0]);

    const std::vector<types::GlobalDiscoveryEntry>& globalEntryVec = {globalEntry};

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
            .Times(1);

    EXPECT_CALL(*_mockGlobalLookupCache, insert(_)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(globalEntryVec));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(0);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    auto onSuccess =
            [this, expectedEntry](const std::vector<types::DiscoveryEntryWithMetaInfo>& results) {
                EXPECT_EQ(1, results.size());
                const types::DiscoveryEntryWithMetaInfo& result = results.at(0);
                EXPECT_TRUE(compareDiscoveryEntries(expectedEntry, result));
                _semaphore->notify();
            };
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        onSuccess,
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, callTouchPeriodically)
{
    // make sure that there is only one runtime that is periodically calling touch
    initializeMockLocalCapabilitiesDirectoryStore();
    test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);
    configureShortCapabilitiesFreshnessUpdateInterval();

    std::string participantId1 = "participantId1";

    // set gbid, it should be one of the known
    std::string gbid = _KNOWN_GBIDS[1];
    std::vector<std::string> gbids{gbid};

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    _entry.setLastSeenDateMs(0);
    _entry.setExpiryDateMs(0);
    _entry.setQos(providerQos);
    _entry.setParticipantId(participantId1);

    std::vector<std::string> expectedParticipantIds{participantId1};
    auto gcdSemaphore = std::make_shared<Semaphore>(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                touch(Eq(_clusterControllerId), UnorderedElementsAreArray(expectedParticipantIds),
                      Eq(gbid), _, _))
            .Times(2)
            .WillRepeatedly(ReleaseSemaphore(gcdSemaphore));

    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry,
                                     false,
                                     gbids,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    // wait for add call
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // verify that touch has not been invoked yet
    EXPECT_FALSE(gcdSemaphore->waitFor(std::chrono::milliseconds(1)));
    EXPECT_TRUE(gcdSemaphore->waitFor(std::chrono::milliseconds(250)));
    EXPECT_FALSE(gcdSemaphore->waitFor(std::chrono::milliseconds(150)));
    EXPECT_TRUE(gcdSemaphore->waitFor(std::chrono::milliseconds(100)));
}

TEST_F(LocalCapabilitiesDirectoryTest, touchNotCalled_noParticipantIdsToTouch_entryHasLocalScope)
{
    // make sure that there is only one runtime that is periodically calling touch
    initializeMockLocalCapabilitiesDirectoryStore();
    test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);
    configureShortCapabilitiesFreshnessUpdateInterval();

    std::string participantId1 = "participantId1";
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    _entry.setLastSeenDateMs(0);
    _entry.setExpiryDateMs(0);
    _entry.setQos(providerQos);
    _entry.setParticipantId(participantId1);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, touch(_, _, _, _, _)).Times(0);
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    // wait for add call
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(LocalCapabilitiesDirectoryTest, touchCalledOnce_multipleParticipantIdsForSingleGbid)
{
    // make sure that there is only one runtime that is periodically calling touch
    initializeMockLocalCapabilitiesDirectoryStore();
    test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);
    configureShortCapabilitiesFreshnessUpdateInterval();

    std::string participantId1 = "participantId1";
    std::string participantId2 = "participantId2";

    // set gbid, it should be one of the known
    std::string gbid = _KNOWN_GBIDS[1];
    std::vector<std::string> gbids{gbid};

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    _entry.setLastSeenDateMs(0);
    _entry.setExpiryDateMs(0);
    _entry.setQos(providerQos);
    _entry.setParticipantId(participantId1);

    auto touchSemaphore = std::make_shared<joynr::Semaphore>(0);
    std::vector<std::string> capturedParticipantIds;
    std::string capturedGbid;

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, touch(Eq(_clusterControllerId), _, _, _, _))
            .WillOnce(DoAll(SaveArg<1>(&capturedParticipantIds), SaveArg<2>(&capturedGbid),
                            ReleaseSemaphore(touchSemaphore)));
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry,
                                     false,
                                     gbids,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    types::DiscoveryEntry entry2(_entry);
    entry2.setParticipantId(participantId2);
    _localCapabilitiesDirectory->add(entry2,
                                     false,
                                     gbids,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    std::vector<std::string> expectedParticipantIds{participantId1, participantId2};

    // wait for 2 add calls
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(touchSemaphore->waitFor(std::chrono::milliseconds(250)));

    // Compare captured results with the given ones
    bool foundParticipantId1 = false;
    bool foundParticipantId2 = false;

    for (const auto& actualParticipantId : capturedParticipantIds) {
        if (actualParticipantId == participantId1) {
            foundParticipantId1 = true;
        }
        if (actualParticipantId == participantId2) {
            foundParticipantId2 = true;
        }
    }

    EXPECT_EQ(capturedParticipantIds.size(), 2);
    EXPECT_EQ(capturedGbid, gbid);
    EXPECT_TRUE(foundParticipantId1);
    EXPECT_TRUE(foundParticipantId2);
}

TEST_F(LocalCapabilitiesDirectoryTest, touchCalledOnce_singleParticipantIdForMultipleGbids)
{
    // make sure that there is only one runtime that is periodically calling touch
    initializeMockLocalCapabilitiesDirectoryStore();
    test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);
    configureShortCapabilitiesFreshnessUpdateInterval();

    std::string participantId1 = "participantId1";

    // set gbid, it should be one of the known
    std::string gbid1 = _KNOWN_GBIDS[0];
    std::string gbid2 = _KNOWN_GBIDS[1];
    std::vector<std::string> gbids{gbid1, gbid2};

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    _entry.setLastSeenDateMs(0);
    _entry.setExpiryDateMs(0);
    _entry.setQos(providerQos);
    _entry.setParticipantId(participantId1);

    auto touchSemaphore = std::make_shared<Semaphore>(0);
    std::vector<std::string> capturedParticipantIds;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                touch(Eq(_clusterControllerId), _, Eq(gbid1), _, _))
            .WillOnce(DoAll(SaveArg<1>(&capturedParticipantIds), ReleaseSemaphore(touchSemaphore)));
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry,
                                     false,
                                     gbids,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    std::vector<std::string> expectedParticipantIds{participantId1};

    // wait for add call
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(touchSemaphore->waitFor(std::chrono::milliseconds(399)));

    EXPECT_EQ(capturedParticipantIds.size(), 1);
    EXPECT_EQ(capturedParticipantIds[0], participantId1);
}

TEST_F(LocalCapabilitiesDirectoryTest, touchCalledTwice_twoParticipantIdsForDifferentGbids)
{
    // make sure that there is only one runtime that is periodically calling touch
    initializeMockLocalCapabilitiesDirectoryStore();
    test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);
    configureShortCapabilitiesFreshnessUpdateInterval();

    std::string participantId1 = "participantId1";
    std::string participantId2 = "participantId2";

    // set gbid, it should be one of the known
    std::string gbid1 = _KNOWN_GBIDS[0];
    std::string gbid2 = _KNOWN_GBIDS[1];
    std::vector<std::string> gbids1{gbid1};
    std::vector<std::string> gbids2{gbid2};

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    _entry.setLastSeenDateMs(0);
    _entry.setExpiryDateMs(0);
    _entry.setQos(providerQos);
    _entry.setParticipantId(participantId1);

    auto touchSemaphore = std::make_shared<Semaphore>(0);
    std::string actualGbid1;
    std::string actualGbid2;
    std::vector<std::string> participantIds1;
    std::vector<std::string> participantIds2;

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, touch(Eq(_clusterControllerId), _, _, _, _))
            .WillOnce(DoAll(SaveArg<1>(&participantIds1), SaveArg<2>(&actualGbid1),
                            ReleaseSemaphore(touchSemaphore)))
            .WillOnce(DoAll(SaveArg<1>(&participantIds2), SaveArg<2>(&actualGbid2),
                            ReleaseSemaphore(touchSemaphore)));
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(_entry,
                                     false,
                                     gbids1,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    types::DiscoveryEntry entry2(_entry);
    entry2.setParticipantId(participantId2);
    _localCapabilitiesDirectory->add(entry2,
                                     false,
                                     gbids2,
                                     createVoidOnSuccessFunction(),
                                     _unexpectedOnDiscoveryErrorFunction);

    // wait for 2 add calls
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(touchSemaphore->waitFor(std::chrono::milliseconds(250)));
    EXPECT_TRUE(touchSemaphore->waitFor(std::chrono::milliseconds(250)));

    ASSERT_EQ(1, participantIds1.size());
    ASSERT_EQ(1, participantIds2.size());
    if (gbid1 == actualGbid1) {
        EXPECT_EQ(participantId1, participantIds1[0]);
        EXPECT_EQ(participantId2, participantIds2[0]);
        EXPECT_EQ(gbid2, actualGbid2);
    } else if (gbid2 == actualGbid1) {
        EXPECT_EQ(participantId2, participantIds1[0]);
        EXPECT_EQ(participantId1, participantIds2[0]);
        EXPECT_EQ(gbid1, actualGbid2);
    } else {
        FAIL() << "Test of twoParticipantIdsForDifferentGbids failed!";
    }
}

TEST_F(LocalCapabilitiesDirectoryTest, touchRefreshesAllEntries_GcdTouchOnlyUsesGlobalOnes)
{
    // make sure that there is only one runtime that is periodically calling touch
    initializeMockLocalCapabilitiesDirectoryStore();
    test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);
    configureShortCapabilitiesFreshnessUpdateInterval();

    // Fill the LCD
    const std::string participantId1 = "participantId1";
    const std::string participantId2 = "participantId2";
    const std::string participantId3 = "participantId3";
    const std::vector<std::string> expectedParticipantIds{participantId2, participantId3};
    // entry1 (local, touch will update entry in local store only)
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);
    _entry.setParticipantId(participantId1);
    const std::int64_t oldLastSeenDateMs1 = _entry.getLastSeenDateMs();
    const std::int64_t oldExpiryDateMs1 = _entry.getExpiryDateMs();

    auto gcdSemaphore = std::make_shared<Semaphore>(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                touch(Eq(_clusterControllerId), UnorderedElementsAreArray(expectedParticipantIds),
                      _, _, _))
            .WillOnce(ReleaseSemaphore(gcdSemaphore));
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    // entry2 (global, touch will update entry in local store and call GCD)
    const std::int64_t oldLastSeenDateMs2 = 0;
    const std::int64_t oldExpiryDateMs2 = oldLastSeenDateMs2;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setLastSeenDateMs(oldLastSeenDateMs2);
    _entry.setExpiryDateMs(oldExpiryDateMs2);
    _entry.setQos(providerQos);
    _entry.setParticipantId(participantId2);
    _localCapabilitiesDirectory->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    // entry3 (global, with large expiryDateMs and large lastSeenDateMs,
    // touch will not update the expiryDateMs to not reduce it,
    // lastSeenDateMs is replaced during add and will be updated)
    const std::int64_t oldLastSeenDateMs3 =
            TimePoint::now().toMilliseconds() + 2 * _defaultExpiryIntervalMs;
    const std::int64_t oldExpiryDateMs3 = oldLastSeenDateMs3;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setLastSeenDateMs(oldLastSeenDateMs3);
    _entry.setExpiryDateMs(oldExpiryDateMs3);
    _entry.setQos(providerQos);
    _entry.setParticipantId(participantId3);
    _localCapabilitiesDirectory->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    // wait for 3 add calls
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    const std::int64_t minLastSeenDateMs = TimePoint::now().toMilliseconds();
    const std::int64_t minExpiryDateMs = minLastSeenDateMs + _defaultExpiryIntervalMs;
    // check test setup
    ASSERT_GE(minLastSeenDateMs, oldLastSeenDateMs1);
    ASSERT_GT(minLastSeenDateMs, oldLastSeenDateMs2);
    ASSERT_LT(minLastSeenDateMs, oldLastSeenDateMs3);
    ASSERT_GE(minExpiryDateMs, oldExpiryDateMs1);
    ASSERT_GT(minExpiryDateMs, oldExpiryDateMs2);
    ASSERT_LT(minExpiryDateMs, oldExpiryDateMs3);
    // no touch yet

    // check GCD touch
    EXPECT_TRUE(gcdSemaphore->waitFor(std::chrono::milliseconds(350)));

    // check entry1
    EXPECT_LT(minLastSeenDateMs,
              _localCapabilitiesDirectoryStore->getLocalCapabilities(participantId1)[0]
                      .getLastSeenDateMs());
    EXPECT_LT(minExpiryDateMs,
              _localCapabilitiesDirectoryStore->getLocalCapabilities(participantId1)[0]
                      .getExpiryDateMs());
    std::unique_lock<std::recursive_mutex> cacheLock(
            _localCapabilitiesDirectoryStore->getCacheLock());
    ASSERT_FALSE(_localCapabilitiesDirectoryStore->getGlobalLookupCache(cacheLock)
                         ->lookupByParticipantId(participantId1));
    cacheLock.unlock();

    // check entry2
    const std::int64_t lastSeenDateMs2 =
            _localCapabilitiesDirectoryStore->getLocalCapabilities(participantId2)[0]
                    .getLastSeenDateMs();
    const std::int64_t expiryDateMs2 =
            _localCapabilitiesDirectoryStore->getLocalCapabilities(participantId2)[0]
                    .getExpiryDateMs();
    EXPECT_LT(minLastSeenDateMs, lastSeenDateMs2);
    EXPECT_LT(minExpiryDateMs, expiryDateMs2);
    cacheLock.lock();
    ASSERT_FALSE(_localCapabilitiesDirectoryStore->getGlobalLookupCache(cacheLock)
                         ->lookupByParticipantId(participantId2));

    // check entry3
    const std::int64_t lastSeenDateMs3 =
            _localCapabilitiesDirectoryStore->getLocalCapabilities(participantId3)[0]
                    .getLastSeenDateMs();
    EXPECT_LT(minLastSeenDateMs, lastSeenDateMs3);
    EXPECT_EQ(oldExpiryDateMs3,
              _localCapabilitiesDirectoryStore->getLocalCapabilities(participantId3)[0]
                      .getExpiryDateMs());
    ASSERT_FALSE(_localCapabilitiesDirectoryStore->getGlobalLookupCache(cacheLock)
                        ->lookupByParticipantId(participantId3));
}

TEST_F(LocalCapabilitiesDirectoryTest, addMultipleTimesSameProviderAwaitForGlobal)
{
    types::ProviderQos qos;
    qos.setScope(types::ProviderScope::GLOBAL);
    const types::DiscoveryEntry entry(_defaultProviderVersion,
                                      _DOMAIN_1_NAME,
                                      _INTERFACE_1_NAME,
                                      _dummyParticipantIdsVector[0],
                                      qos,
                                      _lastSeenDateMs,
                                      _lastSeenDateMs + _defaultExpiryIntervalMs,
                                      _PUBLIC_KEY_ID);

    // trigger a failure on the first call and a success on the second

    // 1st call
    auto gcdSemaphore = std::make_shared<Semaphore>(0);
    auto& exp1 =
            EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                        add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _, _))
                    .Times(1)
                    .WillOnce(DoAll(Invoke(this, &LocalCapabilitiesDirectoryTest::
                                                         fakeCapabilitiesClientAddWithException),
                                    ReleaseSemaphore(gcdSemaphore))); // invoke onError
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _, _))
            .Times(1)
            .After(exp1)
            .WillOnce(DoAll(
                    Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddSuccess),
                    ReleaseSemaphore(gcdSemaphore))); // invoke onSuccess

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            _entry, true, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    // wait for it...
    EXPECT_TRUE(gcdSemaphore->waitFor(std::chrono::seconds(1)));

    // 2nd call
    _localCapabilitiesDirectory->add(
            _entry, true, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    // wait for it...
    EXPECT_TRUE(gcdSemaphore->waitFor(std::chrono::seconds(1)));

    // do a lookup to make sure the entry still exists
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
}

class LocalCapabilitiesDirectoryACMockTest
        : public LocalCapabilitiesDirectoryTest,
          public ::testing::WithParamInterface<std::tuple<bool, bool>>
{
public:
    LocalCapabilitiesDirectoryACMockTest()
            : _ENABLE_ACCESS_CONTROL(std::get<0>(GetParam())),
              _HAS_PERMISSION(std::get<1>(GetParam()))
    {
        _clusterControllerSettings.setEnableAccessController(_ENABLE_ACCESS_CONTROL);
    }

protected:
    const bool _ENABLE_ACCESS_CONTROL;
    const bool _HAS_PERMISSION;
};

TEST_P(LocalCapabilitiesDirectoryACMockTest, checkPermissionToRegisterWithMock)
{
    auto mockAccessController = std::make_shared<MockAccessController>();
    ON_CALL(*mockAccessController, hasProviderPermission(_, _, _, _))
            .WillByDefault(Return(this->_HAS_PERMISSION));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->setAccessController(util::as_weak_ptr(mockAccessController));

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                _DOMAIN_1_NAME,
                                _INTERFACE_1_NAME,
                                _dummyParticipantIdsVector[0],
                                types::ProviderQos(),
                                _lastSeenDateMs,
                                _lastSeenDateMs + _defaultExpiryIntervalMs,
                                _PUBLIC_KEY_ID);

    try {
        _localCapabilitiesDirectory->add(
                entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    } catch (const exceptions::ProviderRuntimeException&) {
    }

    const types::DiscoveryQos localDiscoveryQos;
    if (!this->_ENABLE_ACCESS_CONTROL || this->_HAS_PERMISSION) {
        _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                            localDiscoveryQos,
                                            _KNOWN_GBIDS,
                                            createLookupParticipantIdSuccessFunction(),
                                            _unexpectedOnDiscoveryErrorFunction);
    } else {
        _localCapabilitiesDirectory->lookup(
                _dummyParticipantIdsVector[0],
                localDiscoveryQos,
                _KNOWN_GBIDS,
                createUnexpectedLookupParticipantIdSuccessFunction(),
                createExpectedDiscoveryErrorFunction(
                        types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT));
    }
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

std::tuple<bool, bool> const LCDWithAC_UseCases[] = {
        // Access controller enabled/disabled: tuple[0]
        // Emulation of "Has and not have" permission: tuple[1]
        make_tuple(false, false), make_tuple(false, true), make_tuple(true, false),
        make_tuple(true, true)};

INSTANTIATE_TEST_SUITE_P(WithAC,
                         LocalCapabilitiesDirectoryACMockTest,
                         ::testing::ValuesIn(LCDWithAC_UseCases));

class LocalCapabilitiesDirectoryACTest : public LocalCapabilitiesDirectoryTest
{
public:
    LocalCapabilitiesDirectoryACTest()
    {
        _clusterControllerSettings.setEnableAccessController(true);
        auto localDomainAccessStore = std::make_shared<LocalDomainAccessStore>(
                "test-resources/LDAS_checkPermissionToAdd.json");
        auto localDomainAccessController =
                std::make_shared<LocalDomainAccessController>(localDomainAccessStore);
        finalizeTestSetupAfterMockExpectationsAreDone();
        _accessController = std::make_shared<AccessController>(
                _localCapabilitiesDirectory, localDomainAccessController);
        _localCapabilitiesDirectory->setAccessController(util::as_weak_ptr(_accessController));

        localDomainAccessStore->logContent();
    }

protected:
    std::shared_ptr<IAccessController> _accessController;
};

TEST_F(LocalCapabilitiesDirectoryACTest, checkPermissionToAdd)
{
    types::DiscoveryEntry OK_entry(_defaultProviderVersion,
                                   "domain-1234",
                                   "my/favourite/interface/Name",
                                   _dummyParticipantIdsVector[0],
                                   types::ProviderQos(),
                                   _lastSeenDateMs,
                                   _lastSeenDateMs + _defaultExpiryIntervalMs,
                                   _PUBLIC_KEY_ID);

    types::DiscoveryEntry NOT_OK_entry_1(
            _defaultProviderVersion,
            "domain-1234",                // domain is OK
            "my/favourite/interface/Nam", // interfaceName is a substring of the allowed one
            _dummyParticipantIdsVector[0],
            types::ProviderQos(),
            _lastSeenDateMs,
            _lastSeenDateMs + _defaultExpiryIntervalMs,
            _PUBLIC_KEY_ID);

    types::DiscoveryEntry NOT_OK_entry_2(_defaultProviderVersion,
                                         "domain-123", // domain is a substring of the allowed one
                                         "my/favourite/interface/Name", // interfaceName is OK
                                         _dummyParticipantIdsVector[0],
                                         types::ProviderQos(),
                                         _lastSeenDateMs,
                                         _lastSeenDateMs + _defaultExpiryIntervalMs,
                                         _PUBLIC_KEY_ID);

    std::string principal = "testUser";
    CallContext callContext;
    callContext.setPrincipal(principal);
    CallContextStorage::set(std::move(callContext));

    _localCapabilitiesDirectory->add(
            OK_entry,
            []() { SUCCEED() << "OK"; },
            [](const exceptions::ProviderRuntimeException& ex) { FAIL() << ex.getMessage(); });

    EXPECT_THROW(_localCapabilitiesDirectory->add(
                         NOT_OK_entry_1,
                         []() { FAIL(); },
                         [](const exceptions::ProviderRuntimeException& ex) {
                             FAIL() << ex.getMessage();
                         }),
                 exceptions::ProviderRuntimeException);

    EXPECT_THROW(_localCapabilitiesDirectory->add(
                         NOT_OK_entry_2,
                         []() { FAIL(); },
                         [](const exceptions::ProviderRuntimeException& ex) {
                             FAIL() << ex.getMessage();
                         }),
                 exceptions::ProviderRuntimeException);

    CallContextStorage::invalidate();
}

class LocalCapabilitiesDirectoryWithProviderScope
        : public LocalCapabilitiesDirectoryTest,
          public ::testing::WithParamInterface<types::ProviderScope::Enum>
{
};

TEST_P(LocalCapabilitiesDirectoryWithProviderScope,
       registerCapabilitiesMultipleTimesDoesNotDuplicate)
{
    types::ProviderQos providerQos;
    providerQos.setScope(GetParam());

    const int numberOfDuplicatedEntriesToAdd = 3;
    const bool testingGlobalScope = GetParam() == types::ProviderScope::GLOBAL;
    if (testingGlobalScope) {
        // simulate capabilities client cannot connect to global directory
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _, _))
                .Times(numberOfDuplicatedEntriesToAdd)
                .WillRepeatedly(Invoke(
                        this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddSuccess));
    }

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    for (int i = 0; i < numberOfDuplicatedEntriesToAdd; ++i) {
        // change expiryDate and lastSeen so that entries are not exactly equal
        _lastSeenDateMs++;
        _defaultExpiryIntervalMs++;

        types::DiscoveryEntry entry(_defaultProviderVersion,
                                    _DOMAIN_1_NAME,
                                    _INTERFACE_1_NAME,
                                    _dummyParticipantIdsVector[0],
                                    providerQos,
                                    _lastSeenDateMs,
                                    _lastSeenDateMs + _defaultExpiryIntervalMs,
                                    _PUBLIC_KEY_ID);
        _localCapabilitiesDirectory->add(
                _entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    }

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveStaleProvidersOfClusterController)
{
    std::int64_t maxLastSeenMs = 100000;
    for (const auto& gbid : _KNOWN_GBIDS) {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    removeStale(Eq(_clusterControllerId), Eq(maxLastSeenMs), Eq(gbid), _, _))
                .WillOnce(Invoke(
                        this,
                        &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientRemoveStaleSuccess));
    }

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();
    _localCapabilitiesDirectory->removeStaleProvidersOfClusterController(maxLastSeenMs);
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveStaleProvidersOfClusterControllerRetry)
{
    std::int64_t maxLastSeenMs = TimePoint::now().toMilliseconds() - 10000;
    for (const auto& gbid : _KNOWN_GBIDS) {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    removeStale(Eq(_clusterControllerId), Eq(maxLastSeenMs), Eq(gbid), _, _))
                .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::
                                               fakeCapabilitiesClientRemoveStaleWithException))
                .WillOnce(Invoke(
                        this,
                        &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientRemoveStaleSuccess));
    }

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();
    _localCapabilitiesDirectory->removeStaleProvidersOfClusterController(maxLastSeenMs);
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveStaleProvidersOfClusterControllerNoRetry)
{
    std::int64_t maxLastSeenMs = TimePoint::now().toMilliseconds() - 3600100;
    for (const auto& gbid : _KNOWN_GBIDS) {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    removeStale(Eq(_clusterControllerId), Eq(maxLastSeenMs), Eq(gbid), _, _))
                .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::
                                               fakeCapabilitiesClientRemoveStaleWithException));
    }

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();
    _localCapabilitiesDirectory->removeStaleProvidersOfClusterController(maxLastSeenMs);
}

INSTANTIATE_TEST_SUITE_P(changeProviderScope,
                         LocalCapabilitiesDirectoryWithProviderScope,
                         ::testing::Values(types::ProviderScope::LOCAL,
                                           types::ProviderScope::GLOBAL));
