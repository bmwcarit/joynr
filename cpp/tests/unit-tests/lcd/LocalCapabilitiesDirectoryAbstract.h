/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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

#ifndef LocalCapabilitiesDirectoryAbstract
#define LocalCapabilitiesDirectoryAbstract

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

#include "../../libjoynrclustercontroller/access-control/AccessController.h"
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

using ::testing::InSequence;
using ::testing::Matcher;
using ::testing::Sequence;
using namespace ::testing;
using namespace joynr;

static constexpr std::int64_t conversionDelayToleranceMs = 2000;

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

inline bool compareDiscoveryEntries(const types::DiscoveryEntry& expected,
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

class AbstractLocalCapabilitiesDirectoryTest : public ::testing::Test
{
public:
    AbstractLocalCapabilitiesDirectoryTest()
            : _settings(),
              _clusterControllerSettings(_settings),
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

        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        _discoveryQos.setCacheMaxAge(10000);

        // init a capentry received from the global capabilities directory
        types::ProviderQos qos;
        qos.setScope(types::ProviderScope::GLOBAL);
        types::GlobalDiscoveryEntry globalCapEntry(_defaultProviderVersion,
                                                   _DOMAIN_1_NAME,
                                                   _INTERFACE_1_NAME,
                                                   _dummyParticipantIdsVector[2],
                                                   qos,
                                                   _lastSeenDateMs,
                                                   _lastSeenDateMs + 10000,
                                                   _PUBLIC_KEY_ID,
                                                   _EXTERNAL_ADDRESSES_VECTOR[0]);

        _globalCapEntries.push_back(globalCapEntry);
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

    ~AbstractLocalCapabilitiesDirectoryTest() override
    {
        _singleThreadedIOService->stop();
        test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectory);
        test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);

        test::util::removeFileInCurrentDirectory(".*\\.settings");
        test::util::removeFileInCurrentDirectory(".*\\.persist");
    }
protected:    
    Settings _settings;
    ClusterControllerSettings _clusterControllerSettings;
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
    std::vector<types::GlobalDiscoveryEntry> _globalCapEntries;
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
};

#endif /* LocalCapabilitiesDirectoryAbstract */