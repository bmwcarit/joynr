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
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../libjoynrclustercontroller/access-control/AccessController.h"
#include "../../libjoynrclustercontroller/access-control/LocalDomainAccessController.h"
#include "../../libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

#include "joynr/CallContext.h"
#include "joynr/CallContextStorage.h"
#include "joynr/CapabilitiesStorage.h"
#include "joynr/CapabilityUtils.h"
#include "joynr/ClusterControllerDirectories.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
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

using namespace ::testing;
using namespace joynr;

MATCHER_P2(pointerToAddressWithSerializedAddress, addressType, serializedAddress, "")
{
    if (arg == nullptr) {
        return false;
    }
    if (addressType == "mqtt") {
        auto mqttAddress =
                std::dynamic_pointer_cast<const system::RoutingTypes::MqttAddress>(arg);
        if (mqttAddress == nullptr) {
            return false;
        }
        return serializer::serializeToJson(*mqttAddress) == serializedAddress;
    } else if (addressType == "http") {
        auto httpAddress =
                std::dynamic_pointer_cast<const system::RoutingTypes::ChannelAddress>(arg);
        if (httpAddress == nullptr) {
            return false;
        }
        return serializer::serializeToJson(*httpAddress) == serializedAddress;
    } else {
        return false;
    }
}

MATCHER_P(AnConvertedGlobalDiscoveryEntry, other, "")
{
    return other.getDomain() == arg.getDomain() &&
           other.getInterfaceName() == arg.getInterfaceName() &&
           other.getParticipantId() == arg.getParticipantId() && other.getQos() == arg.getQos() &&
           other.getLastSeenDateMs() == arg.getLastSeenDateMs() &&
           other.getProviderVersion() == arg.getProviderVersion() &&
           other.getExpiryDateMs() == arg.getExpiryDateMs() &&
           other.getPublicKeyId() == arg.getPublicKeyId();
}
bool compareConvertedGlobalDiscoveryEntryWithExtendedDates(const types::DiscoveryEntry& arg,
                                                           const types::DiscoveryEntry& expected,
                                                           const std::int64_t toleranceMs)
{
    return expected.getDomain() == arg.getDomain() &&
           expected.getInterfaceName() == arg.getInterfaceName() &&
           expected.getParticipantId() == arg.getParticipantId() &&
           expected.getQos() == arg.getQos() &&
           expected.getLastSeenDateMs() <= arg.getLastSeenDateMs() &&
           arg.getLastSeenDateMs() <= (expected.getLastSeenDateMs() + toleranceMs) &&
           expected.getProviderVersion() == arg.getProviderVersion() &&
           expected.getExpiryDateMs() <= arg.getExpiryDateMs() &&
           arg.getExpiryDateMs() <= (expected.getExpiryDateMs() + toleranceMs) &&
           expected.getPublicKeyId() == arg.getPublicKeyId();
}

MATCHER_P2(AnConvertedGlobalDiscoveryEntryWithExtendedDates, expected, toleranceMs, "")
{
    return compareConvertedGlobalDiscoveryEntryWithExtendedDates(arg, expected, toleranceMs);
}

class LocalCapabilitiesDirectoryTest : public ::testing::Test
{
public:
    LocalCapabilitiesDirectoryTest()
            : _settings(),
              _settingsForPersistencyTests(),
              _clusterControllerSettings(_settings),
              _clusterControllerSettingsForPersistencyTests(_settingsForPersistencyTests),
              _purgeExpiredDiscoveryEntriesIntervalMs(1000),
              _globalCapabilitiesDirectoryClient(std::make_shared<MockGlobalCapabilitiesDirectoryClient>()),
              _localCapabilitiesDirectoryStore(std::make_shared<LocalCapabilitiesDirectoryStore>()),
              _localCapabilitiesDirectoryStoreForPersistencyTests(std::make_shared<LocalCapabilitiesDirectoryStore>()),
              _mockLocallyRegisteredCapabilities(std::make_shared<capabilities::MockStorage>()),
              _mockLocalCapabilitiesDirectoryStore(std::make_shared<MockLocalCapabilitiesDirectoryStore>()),
              _mockGlobalLookupCache(std::make_shared<capabilities::MockCachingStorage>()),
              _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _mockMessageRouter(
                      std::make_shared<MockMessageRouter>(_singleThreadedIOService->getIOService())),
              _clusterControllerId("clusterControllerId"),
              _localCapabilitiesDirectory(),
              _localCapabilitiesDirectoryWithMockCapStorage(),
              _lastSeenDateMs(std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch()).count()),
              _defaultExpiryDateMs(60 * 60 * 1000),
              _toleranceMs(50),
              _dummyParticipantIdsVector{util::createUuid(), util::createUuid(), util::createUuid()},
              _defaultOnSuccess([]() {}),
              _defaultProviderRuntimeExceptionError([](const exceptions::ProviderRuntimeException&) {}),
              _unexpectedProviderRuntimeExceptionFunction([](const exceptions::ProviderRuntimeException& exception) {
                  FAIL() << "Got unexpected ProviderRuntimeException: " + exception.getMessage(); }
              ),
              _unexpectedOnDiscoveryErrorFunction([](const types::DiscoveryError::Enum& errorEnum) {
                  FAIL() << "Unexpected onError call: " + types::DiscoveryError::getLiteral(errorEnum);}
              ),
              _defaultProviderVersion(26, 05),
              _semaphore(0)
    {
        _singleThreadedIOService->start();
        _clusterControllerSettings.setPurgeExpiredDiscoveryEntriesIntervalMs(
                _purgeExpiredDiscoveryEntriesIntervalMs);
        _settings.set(ClusterControllerSettings::SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS(),
                     200);
        _settings.set(ClusterControllerSettings::
                             SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED(),
                     false);
        _settingsForPersistencyTests.set(ClusterControllerSettings::
                             SETTING_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCY_ENABLED(),
                     true);
        _localCapabilitiesDirectory = std::make_shared<LocalCapabilitiesDirectory>(
                _clusterControllerSettings,
                _globalCapabilitiesDirectoryClient,
                _localCapabilitiesDirectoryStore,
                _LOCAL_ADDRESS,
                _mockMessageRouter,
                _singleThreadedIOService->getIOService(),
                _clusterControllerId,
                _KNOWN_GBIDS,
                _defaultExpiryDateMs);
        _localCapabilitiesDirectory->init();
        _localCapabilitiesDirectoryWithMockCapStorage = std::make_shared<LocalCapabilitiesDirectory>(
                _clusterControllerSettings,
                _globalCapabilitiesDirectoryClient,
                _mockLocalCapabilitiesDirectoryStore,
                _LOCAL_ADDRESS,
                _mockMessageRouter,
                _singleThreadedIOService->getIOService(),
                _clusterControllerId,
                _KNOWN_GBIDS,
                _defaultExpiryDateMs);
        _localCapabilitiesDirectoryWithMockCapStorage->init();
        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        _discoveryQos.setCacheMaxAge(10000);

        // init a capentry recieved from the global capabilities directory
        types::ProviderQos qos;
        qos.setScope(types::ProviderScope::GLOBAL);
        types::DiscoveryEntry globalCapEntry(_defaultProviderVersion,
                                             _DOMAIN_1_NAME,
                                             _INTERFACE_1_NAME,
                                             _dummyParticipantIdsVector[2],
                                             qos,
                                             10000,
                                             10000,
                                             _PUBLIC_KEY_ID);
        _globalCapEntryMap.insert({_EXTERNAL_ADDRESSES_VECTOR[0], globalCapEntry});
        _entry = types::DiscoveryEntry(_defaultProviderVersion,
                                      _DOMAIN_1_NAME,
                                      _INTERFACE_1_NAME,
                                      _dummyParticipantIdsVector[0],
                                      types::ProviderQos(),
                                      _lastSeenDateMs,
                                      _defaultExpiryDateMs,
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

    void fakeCapabilitiesClientAddWithException (
            const types::GlobalDiscoveryEntry& entry,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError) {
        std::ignore = entry;
        std::ignore = gbids;
        std::ignore = onSuccess;
        std::ignore = onError;
        exceptions::DiscoveryException fakeDiscoveryException("fakeDiscoveryException");
        onRuntimeError(fakeDiscoveryException);
    }

    void fakeCapabilitiesClientAddSuccess (
            const types::GlobalDiscoveryEntry& entry,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError) {
        std::ignore = entry;
        std::ignore = gbids;
        std::ignore = onError;
        std::ignore = onRuntimeError;
        onSuccess();
    }

    void testRemoveUsesSameGbidOrderAsAdd(const std::vector<std::string>& selectedGbids)
    {
        const bool awaitGlobalRegistration = true;
        const std::vector<std::string>& expectedGbids {selectedGbids};
        types::ProviderQos providerQos;
        providerQos.setScope(types::ProviderScope::GLOBAL);
        _entry.setQos(providerQos);

        checkAddToGcdClient(expectedGbids);

        _localCapabilitiesDirectory->add(
                _entry,
                awaitGlobalRegistration,
                selectedGbids,
                createAddOnSuccessFunction(),
                _unexpectedOnDiscoveryErrorFunction);
        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    remove(Eq(_dummyParticipantIdsVector[0]),
                           Eq(expectedGbids), _, _, _)).Times(1);

        _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

        Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());
    }

    std::vector<types::GlobalDiscoveryEntry> getGlobalDiscoveryEntries(const std::uint8_t numEntries)
    {
        if (numEntries > _dummyParticipantIdsVector.size()) {
            for (std::uint8_t i = _dummyParticipantIdsVector.size(); i <= numEntries; i++) {
                _dummyParticipantIdsVector.push_back(util::createUuid());
            }
        }

        std::vector<types::GlobalDiscoveryEntry> globalDiscoveryEntryList;
        for(std::uint8_t i = 0; i < numEntries; i++) {
            globalDiscoveryEntryList.push_back(types::GlobalDiscoveryEntry(_defaultProviderVersion,
                                                                     _DOMAIN_1_NAME,
                                                                     _INTERFACE_1_NAME,
                                                                     _dummyParticipantIdsVector[i],
                                                                     types::ProviderQos(),
                                                                     _LASTSEEN_MS,
                                                                     _EXPIRYDATE_MS,
                                                                     _PUBLIC_KEY_ID,
                                                                     _EXTERNAL_ADDRESSES_VECTOR[i>2?0:i]));
        }
        return globalDiscoveryEntryList;
    }

    void testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(const std::vector<std::string>& gbidsForLookup,
                                                             const std::vector<std::string>& expectedGbids)
    {
        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    lookup(Eq(_dummyParticipantIdsVector[1]),
                    Eq(expectedGbids), Eq(_discoveryQos.getDiscoveryTimeout()), _, _, _)).Times(1);
        _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[1],
                                           _discoveryQos,
                                           gbidsForLookup,
                                           nullptr,
                                           nullptr);
    }

    void testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(const std::vector<std::string>& gbids)
    {
        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        // caching dummy entry
        types::ProviderQos providerQos;
        providerQos.setScope(types::ProviderScope::GLOBAL);
        _entry.setQos(providerQos);
        const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(false, _entry);

        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    add(_, _, _, _, _)).Times(1)
                    .WillOnce(InvokeArgument<2>());

        _localCapabilitiesDirectory->add(_entry, true, _KNOWN_GBIDS,
                                        createAddOnSuccessFunction(), _unexpectedOnDiscoveryErrorFunction);

        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
        Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

        // Entry is already cached GCD should not be involved
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

        auto onSuccess = [this, expectedEntry] (const types::DiscoveryEntryWithMetaInfo& result) {
            EXPECT_EQ(expectedEntry, result);
            _semaphore.notify();
        };

        _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                           _discoveryQos,
                                           gbids,
                                           onSuccess,
                                           _unexpectedOnDiscoveryErrorFunction);
        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)> createLookupSuccessFunction(
            const int expectedNumberOfEntries)
    {
        return [this, expectedNumberOfEntries] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
            EXPECT_EQ(expectedNumberOfEntries, result.size());
            _semaphore.notify();
        };
    }

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)> createLookupSuccessFunction(
            const int expectedNumberOfEntries,
            std::vector<types::DiscoveryEntryWithMetaInfo>& returnValue)
    {
        return [this, expectedNumberOfEntries, &returnValue] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
            EXPECT_EQ(expectedNumberOfEntries, result.size());
            returnValue = result;
            _semaphore.notify();
        };
    }

    std::function<void()> createAddOnSuccessFunction()
    {
        return [this] () {
            _semaphore.notify();
        };
    }

    std::function<void(const types::DiscoveryEntryWithMetaInfo&)>
    createLookupParticipantIdSuccessFunction()
    {
        return [this] (const types::DiscoveryEntryWithMetaInfo& result) {
            std::ignore = result;
            _semaphore.notify();
        };
    }

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>
    createUnexpectedLookupSuccessFunction()
    {
        return [] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
            FAIL() << "Got result: " + (result.empty() ? "EMPTY" : result.at(0).toString());
        };
    }

    std::function<void(const types::DiscoveryEntryWithMetaInfo&)>
    createUnexpectedLookupParticipantIdSuccessFunction()
    {
        return [] (const types::DiscoveryEntryWithMetaInfo& result) {
            FAIL() << "Got result: " + result.toString();
        };
    }

    std::function<void(const types::DiscoveryError::Enum&)> createExpectedDiscoveryErrorFunction(
            const types::DiscoveryError::Enum& expectedError)
    {
        return [this, expectedError] (const types::DiscoveryError::Enum& error) {
            EXPECT_EQ(expectedError, error);
            _semaphore.notify();
        };
    }

    std::function<void()> createUnexpectedAddOnSuccessFunction() {
        return [] () {
            FAIL();
        };
    }

    std::function<void(const exceptions::ProviderRuntimeException&)>
    createExpectedProviderRuntimeExceptionFunction(
            const joynr::exceptions::ProviderRuntimeException& expectedRuntimeException) {
        return [this, expectedRuntimeException] (const exceptions::ProviderRuntimeException& exception) {
            EXPECT_EQ(expectedRuntimeException, exception);
            _semaphore.notify();
        };
    }

    void testAddToAllIsProperlyRejected(const types::DiscoveryError::Enum& expectedError)
    {
        const bool awaitGlobalRegistration = true;

        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(_entry)),
                _, _, _, _)).Times(1)
                .WillOnce(InvokeArgument<3>(expectedError));

        _localCapabilitiesDirectory->addToAll(
                _entry,
                awaitGlobalRegistration,
                createUnexpectedAddOnSuccessFunction(),
                createExpectedDiscoveryErrorFunction(expectedError));

        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::Enum errorEnum)
    {
        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
                .Times(1)
                .WillOnce(InvokeArgument<5>(errorEnum));

        _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                           _INTERFACE_1_NAME,
                                           _discoveryQos,
                                           {},
                                           createUnexpectedLookupSuccessFunction(),
                                           createExpectedDiscoveryErrorFunction(errorEnum));
        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::Enum errorEnum)
    {
        _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(Eq(_dummyParticipantIdsVector[1]), _, _, _, _, _))
                .Times(1)
                .WillOnce(InvokeArgument<4>(errorEnum));

        _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[1],
                                           _discoveryQos,
                                           _KNOWN_GBIDS,
                                           createUnexpectedLookupParticipantIdSuccessFunction(),
                                           createExpectedDiscoveryErrorFunction(errorEnum));

        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void testLookupByDomainInterfaceWithGbids_gbidValidationFails(
            const std::vector<std::string>& gbids,
            types::DiscoveryError::Enum errorEnum)
    {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);

        _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                           _INTERFACE_1_NAME,
                                           _discoveryQos,
                                           gbids,
                                           createUnexpectedLookupSuccessFunction(),
                                           createExpectedDiscoveryErrorFunction(errorEnum));
        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void testLookupByParticipantIdWithGbids_gbidValidationFails(
            const std::vector<std::string>& gbids,
            types::DiscoveryError::Enum errorEnum)
    {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

        _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[1],
                                           _discoveryQos,
                                           gbids,
                                           createUnexpectedLookupParticipantIdSuccessFunction(),
                                           createExpectedDiscoveryErrorFunction(errorEnum));

        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void testAddWithGbidsIsProperlyRejected(const types::DiscoveryError::Enum& expectedDiscoveryError) {
        const bool awaitGlobalRegistration = true;
        const std::vector<std::string>& gbids = {_KNOWN_GBIDS[0]};
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(1)
                .WillOnce(InvokeArgument<3>(expectedDiscoveryError));

        _localCapabilitiesDirectory->add(
                _entry,
                awaitGlobalRegistration,
                gbids,
                createUnexpectedAddOnSuccessFunction(),
                createExpectedDiscoveryErrorFunction(expectedDiscoveryError));

        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void testAddIsProperlyRejected(const types::DiscoveryError::Enum& expectedDiscoveryError) {
        const bool awaitGlobalRegistration = true;
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(1)
                .WillOnce(InvokeArgument<3>(expectedDiscoveryError));

        joynr::exceptions::ProviderRuntimeException expectedException(
                fmt::format("Error registering provider {} in default backend: {}",
                            _entry.getParticipantId(),
                            types::DiscoveryError::getLiteral(expectedDiscoveryError)));
        _localCapabilitiesDirectory->add(
                _entry,
                awaitGlobalRegistration,
                createUnexpectedAddOnSuccessFunction(),
                createExpectedProviderRuntimeExceptionFunction(expectedException));

        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    void checkAddToGcdClient(const std::vector<std::string>& expectedGbids) {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Eq(_expectedGlobalCapEntry),
                Eq(expectedGbids), _, _, _)).Times(1)
                .WillOnce(InvokeArgument<2>());
    }

    void testGbidValidationOnAdd(
            const std::vector<std::string>& gbids, const types::DiscoveryError::Enum& expectedDiscoveryError)
    {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, add(_, _, _, _, _)).Times(0);
        const bool awaitGlobalRegistration = true;
        _localCapabilitiesDirectory->add(
                _entry,
                awaitGlobalRegistration,
                gbids,
                createUnexpectedAddOnSuccessFunction(),
                createExpectedDiscoveryErrorFunction(expectedDiscoveryError));

        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

protected:
    void registerReceivedCapabilities(
            const std::string& addressType,
            const std::string& serializedAddress)
    {
        const std::string& participantId = "TEST_participantId";
        EXPECT_CALL(
                *_mockMessageRouter,
                addNextHop(participantId,
                           AllOf(Pointee(A<const system::RoutingTypes::Address>()),
                                 pointerToAddressWithSerializedAddress(addressType, serializedAddress)),
                           _,
                           _,
                           _,
                           _,
                           _)).Times(1);
        EXPECT_CALL(*_mockMessageRouter,
                    addNextHop(participantId,
                               AnyOf(Not(Pointee(A<const system::RoutingTypes::Address>())),
                                     Not(pointerToAddressWithSerializedAddress(
                                             addressType, serializedAddress))),
                               _,
                               _,
                               _,
                               _,
                               _)).Times(0);

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
    const int _purgeExpiredDiscoveryEntriesIntervalMs;
    std::shared_ptr<MockGlobalCapabilitiesDirectoryClient> _globalCapabilitiesDirectoryClient;
    std::shared_ptr<LocalCapabilitiesDirectoryStore> _localCapabilitiesDirectoryStore;
    std::shared_ptr<LocalCapabilitiesDirectoryStore> _localCapabilitiesDirectoryStoreForPersistencyTests;
    std::shared_ptr<capabilities::MockStorage> _mockLocallyRegisteredCapabilities;
    std::shared_ptr<MockLocalCapabilitiesDirectoryStore> _mockLocalCapabilitiesDirectoryStore;
    std::shared_ptr<capabilities::MockCachingStorage> _mockGlobalLookupCache;
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> _mockMessageRouter;
    std::string _clusterControllerId;
    std::shared_ptr<LocalCapabilitiesDirectory> _localCapabilitiesDirectory;
    std::shared_ptr<LocalCapabilitiesDirectory> _localCapabilitiesDirectoryWithMockCapStorage;
    std::int64_t _lastSeenDateMs;
    std::int64_t _defaultExpiryDateMs;
    const std::int64_t _toleranceMs;
    std::vector<std::string> _dummyParticipantIdsVector;
    types::DiscoveryQos _discoveryQos;
    std::unordered_multimap<std::string, types::DiscoveryEntry> _globalCapEntryMap;
    types::DiscoveryEntry _entry;
    types::GlobalDiscoveryEntry _expectedGlobalCapEntry;

    std::function<void()> _defaultOnSuccess;
    std::function<void(const exceptions::ProviderRuntimeException&)> _defaultProviderRuntimeExceptionError;
    std::function<void(const exceptions::ProviderRuntimeException&)> _unexpectedProviderRuntimeExceptionFunction;
    std::function<void(const types::DiscoveryError::Enum& errorEnum)> _unexpectedOnDiscoveryErrorFunction;
    types::Version _defaultProviderVersion;
    Semaphore _semaphore;

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

const std::vector<std::string> LocalCapabilitiesDirectoryTest::_KNOWN_GBIDS{"testGbid1", "testGbid2", "testGbid3"};
const std::string LocalCapabilitiesDirectoryTest::_INTERFACE_1_NAME("myInterfaceA");
const std::string LocalCapabilitiesDirectoryTest::_INTERFACE_2_NAME("myInterfaceB");
const std::string LocalCapabilitiesDirectoryTest::_INTERFACE_3_NAME("myInterfaceC");
const std::string LocalCapabilitiesDirectoryTest::_DOMAIN_1_NAME("domainA");
const std::string LocalCapabilitiesDirectoryTest::_DOMAIN_2_NAME("domainB");
const std::string LocalCapabilitiesDirectoryTest::_DOMAIN_3_NAME("domainB");
const std::string LocalCapabilitiesDirectoryTest::_LOCAL_ADDRESS(
        serializer::serializeToJson(system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "localTopic")));
const std::vector<std::string> LocalCapabilitiesDirectoryTest::_EXTERNAL_ADDRESSES_VECTOR{
            serializer::serializeToJson(system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "externalTopic")),
            serializer::serializeToJson(system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[1], "externalTopic")),
            serializer::serializeToJson(system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[2], "externalTopic"))};
const std::int64_t LocalCapabilitiesDirectoryTest::_LASTSEEN_MS(1000);
const std::int64_t LocalCapabilitiesDirectoryTest::_EXPIRYDATE_MS(10000);
const std::string LocalCapabilitiesDirectoryTest::_PUBLIC_KEY_ID("publicKeyId");
const int LocalCapabilitiesDirectoryTest::_TIMEOUT(2000);

TEST_F(LocalCapabilitiesDirectoryTest, add_global_invokesGcd)
{
    checkAddToGcdClient({_KNOWN_GBIDS[0]});

    _localCapabilitiesDirectory->add(_entry, createAddOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addCapabilityWithSingleNonDefaultGbid)
{
    const std::vector<std::string>& gbids{_KNOWN_GBIDS[1]};
    const std::vector<std::string>& expectedGbids = gbids;
    const bool awaitGlobalRegistration = true;
    checkAddToGcdClient(expectedGbids);

    _localCapabilitiesDirectory->add(
            _entry, awaitGlobalRegistration, gbids, createAddOnSuccessFunction(), _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addWithGbids_global_multipleGbids_invokesGcd)
{
    const std::vector<std::string>& gbids{_KNOWN_GBIDS[1], _KNOWN_GBIDS[0]};
    const std::vector<std::string>& expectedGbids = gbids;
    const bool awaitGlobalRegistration = true;
    checkAddToGcdClient(expectedGbids);

    _localCapabilitiesDirectory->add(
                _entry, awaitGlobalRegistration, gbids, createAddOnSuccessFunction(), _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addWithGbids_global_emptyGbidVector_addsToDefaultBackend)
{
    const std::vector<std::string>& gbids{};
    const std::vector<std::string>& expectedGbids {_KNOWN_GBIDS[0]};
    const bool awaitGlobalRegistration = true;
    checkAddToGcdClient(expectedGbids);

    _localCapabilitiesDirectory->add(_entry,
                                    awaitGlobalRegistration,
                                    gbids,
                                    createAddOnSuccessFunction(),
                                    _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addToAll_global_invokesGcd)
{
    const std::vector<std::string>& expectedGbids = _KNOWN_GBIDS;
    const bool awaitGlobalRegistration = true;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
             add(Eq(_expectedGlobalCapEntry), Eq(expectedGbids), _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<2>());

    _localCapabilitiesDirectory->addToAll(
            _entry, awaitGlobalRegistration, createAddOnSuccessFunction(), _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, add_local_doesNotInvokeGcd)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _)).Times(0);

    _localCapabilitiesDirectory->add(
            _entry,
            createAddOnSuccessFunction(),
            _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addGlobalEntry_callsMockStorage)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setQos(providerQos);
    std::vector<std::string> expectedGbids {_KNOWN_GBIDS[0]};

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                insertInLocalCapabilitiesStorage(Eq(_entry)))
               .Times(1);
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore, insertInGlobalLookupCache(Eq(_entry), Eq(expectedGbids))).Times(1);

    _localCapabilitiesDirectoryWithMockCapStorage->add(
            _entry,
            createAddOnSuccessFunction(),
            _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addLocalEntry_callsMockStorage)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                insertInLocalCapabilitiesStorage(Eq(_entry)))
               .Times(1);
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore, insertInGlobalLookupCache(_,_)).Times(0);

    _localCapabilitiesDirectoryWithMockCapStorage->add(
            _entry,
            createAddOnSuccessFunction(),
            _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupLocalEntryByParticipantId_callsMockStorage)
{
    boost::optional<joynr::types::DiscoveryEntry> result = boost::none;
    _mockLocallyRegisteredCapabilities->setLookupByParticipantIdResult(result);
    std::vector<std::string> gbids{_KNOWN_GBIDS[1]};

    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                getLocalAndCachedCapabilities(Eq(_dummyParticipantIdsVector[0]),_,Eq(gbids),_))
                .Times(1).WillOnce(Return(true));

    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    _localCapabilitiesDirectoryWithMockCapStorage->lookup(_dummyParticipantIdsVector[0],
                                       _discoveryQos,
                                       gbids,
                                       createUnexpectedLookupParticipantIdSuccessFunction(),
                                       createExpectedDiscoveryErrorFunction(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT));
}

TEST_F(LocalCapabilitiesDirectoryTest, addGlobalCapSucceeds_NextAddShallAddGlobalAgain)
{
   const bool awaitGlobalRegistration = true;

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
             add(Matcher<const types::GlobalDiscoveryEntry&>(
                     AnConvertedGlobalDiscoveryEntry(_entry)), _, _, _, _))
            .WillOnce(InvokeArgument<2>());

    _localCapabilitiesDirectory->add(
            _entry,
            awaitGlobalRegistration,
            createAddOnSuccessFunction(),
            _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
             add(Matcher<const types::GlobalDiscoveryEntry&>(
                     AnConvertedGlobalDiscoveryEntry(_entry)), _, _, _, _))
            .WillOnce(InvokeArgument<2>());

    _localCapabilitiesDirectory->add(
            _entry,
            awaitGlobalRegistration,
            createAddOnSuccessFunction(),
            _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
              add(Matcher<const types::GlobalDiscoveryEntry&>(
                      AnConvertedGlobalDiscoveryEntry(_entry)), Eq(expectedGbids), _, _, _)).Times(2)
             .WillRepeatedly(InvokeArgument<2>());

     _localCapabilitiesDirectory->add(
             _entry,
             awaitGlobalRegistration,
             gbids,
             createAddOnSuccessFunction(),
             _unexpectedOnDiscoveryErrorFunction);

     EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

     _localCapabilitiesDirectory->add(
             _entry,
             awaitGlobalRegistration,
             gbids,
             createAddOnSuccessFunction(),
             _unexpectedOnDiscoveryErrorFunction);

     EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addDifferentGbidsAfterEachOther)
{
    const bool awaitGlobalRegistration = true;
    const std::vector<std::string>& gbids1{_KNOWN_GBIDS[0]};
    const std::vector<std::string>& expectedGbids1 = gbids1;
    const std::vector<std::string>& gbids2{_KNOWN_GBIDS[1]};
    const std::vector<std::string>& expectedGbids2 = gbids2;

    checkAddToGcdClient(expectedGbids1);
    _localCapabilitiesDirectory->add(
            _entry,
            awaitGlobalRegistration,
            gbids1,
            createAddOnSuccessFunction(),
            _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    checkAddToGcdClient(expectedGbids2);
    _localCapabilitiesDirectory->add(
            _entry,
            awaitGlobalRegistration,
            gbids2,
            createAddOnSuccessFunction(),
            _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // provider is now registered for both GBIDs
    _localCapabilitiesDirectory->lookup(_entry.getParticipantId(),
                                       _discoveryQos,
                                       gbids1,
                                       createLookupParticipantIdSuccessFunction(),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->lookup(_entry.getParticipantId(),
                                       _discoveryQos,
                                       gbids2,
                                       createLookupParticipantIdSuccessFunction(),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddKnownLocalEntryDoesNothing)
{
    const bool awaitGlobalRegistration = false;
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);
    types::DiscoveryEntry newDiscoveryEntry(_entry);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(),
                _, _, _, _)).Times(0);

    _localCapabilitiesDirectory->add(
            _entry,
            awaitGlobalRegistration,
            _defaultOnSuccess,
            _defaultProviderRuntimeExceptionError);

    _localCapabilitiesDirectory->add(
            newDiscoveryEntry,
            awaitGlobalRegistration,
            _defaultOnSuccess,
            _defaultProviderRuntimeExceptionError);
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAll)
{
    const std::vector<std::string>& expectedGbids = _KNOWN_GBIDS;
    const bool awaitGlobalRegistration = true;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(_entry)),
            Eq(expectedGbids), _, _, _)).Times(1)
            .WillOnce(InvokeArgument<2>());

    _localCapabilitiesDirectory->addToAll(
            _entry, awaitGlobalRegistration, createAddOnSuccessFunction(), _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testAddToAllLocal)
{
    const bool awaitGlobalRegistration = true;
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            add(An<const types::GlobalDiscoveryEntry&>(),
            _, _, _, _)).Times(0);

    _localCapabilitiesDirectory->addToAll(
            _entry,
            awaitGlobalRegistration,
            createAddOnSuccessFunction(),
            _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(2)));
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
            add(Matcher<const types::GlobalDiscoveryEntry&>(AnConvertedGlobalDiscoveryEntry(_entry)),
            Eq(_KNOWN_GBIDS), _, _, _)).Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddWithException));

    _localCapabilitiesDirectory->addToAll(
            _entry,
            awaitGlobalRegistration,
            createUnexpectedAddOnSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_emptyDomainsVector_throwsProviderRuntimeException)
{
    const std::vector<std::string>& emptyDomains{};

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
        expectedEntries.push_back(util::convert(false, entry));
    }

    // simulate global capability directory would store three entries.
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    std::vector<types::DiscoveryEntryWithMetaInfo> result;
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(discoveryEntryResultList.size(), result),
                                       _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    for (const auto& expectedEntry : expectedEntries) {
        ASSERT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntry) != result.cend());
    }

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

    // enries are now in cache, _globalCapabilitiesDirectoryClient should not be called.
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    result = {};
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       {_KNOWN_GBIDS[1]},
                                       createLookupSuccessFunction(1, result),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntries[1]) != result.cend());

    result = {};
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       {_KNOWN_GBIDS[0]},
                                       createLookupSuccessFunction(1, result),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntries[0]) != result.cend());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByDomainInterfaceWithGbids_globalOnly_localGlobalEntries_doesNotInvokeGcd_returnsFilteredResult)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setQos(providerQos);

    const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(false, _entry);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            add(AnConvertedGlobalDiscoveryEntry(_entry), Eq(_KNOWN_GBIDS), _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<2>());

    _localCapabilitiesDirectory->add(_entry, true, _KNOWN_GBIDS, createAddOnSuccessFunction(), _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // local entry is now registered and cached for all GBIDs with the same participantId
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    std::vector<types::DiscoveryEntryWithMetaInfo> result;
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1, result),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntry) != result.cend());

    // search in the 3rd backend for global entries. There is only one entry
    result = {};
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       std::vector<std::string>{_KNOWN_GBIDS[2]},
                                       createLookupSuccessFunction(1, result),
                                       _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(std::find(result.cbegin(), result.cend(), expectedEntry) != result.cend());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupByParticipantIdWithGbids_globalOnly_localGlobalEntries_doesNotInvokeGcd_returnsFilteredResult)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setQos(providerQos);

    const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(false, _entry);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            add(AnConvertedGlobalDiscoveryEntry(_entry), Eq(_KNOWN_GBIDS), _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<2>());

    _localCapabilitiesDirectory->add(_entry, true, _KNOWN_GBIDS, createAddOnSuccessFunction(), _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // local entry is now registered and cached for all GBIDs with the same participantId
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    auto onSuccess = [this, expectedEntry] (const types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_EQ(expectedEntry, result);
        _semaphore.notify();
    };

    std::vector<std::string> gbids{_KNOWN_GBIDS[1]};
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       _discoveryQos,
                                       gbids,
                                       onSuccess,
                                       _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    gbids = {_KNOWN_GBIDS[0]};
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       _discoveryQos,
                                       gbids,
                                       onSuccess,
                                       _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       _discoveryQos,
                                       _KNOWN_GBIDS,
                                       onSuccess,
                                       _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_globalOnly_multipleGbids_allCached)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList =
            getGlobalDiscoveryEntries(6);

    // simulate global capability directory would store five entries
    // globalEntry_withParticipantId0_in_gbid0, globalEntry_withParticipantId1_in_gbid1,
    // globalEntry_withParticipantId2_in_gbid2, globalEntry_withParticipantId3_in_gbid0,
    // globalEntry_withParticipantId4_in_gbid0, globalEntry_withParticipantId5_in_gbid0
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(discoveryEntryResultList.size()),
                                       _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

    // 6 enries are now in cache
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    const std::vector<std::string>& multipleGbids{_KNOWN_GBIDS[0], _KNOWN_GBIDS[2]};
    const std::uint8_t expectedReturnedGlobalEntries = 5;
    std::vector<types::DiscoveryEntryWithMetaInfo> result;
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       multipleGbids,
                                       createLookupSuccessFunction(expectedReturnedGlobalEntries, result),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    auto getPredicate = [] (const std::string& participantId) {
        return [participantId](const types::DiscoveryEntryWithMetaInfo& entry) {
            return entry.getParticipantId() == participantId;
        };
    };
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(), getPredicate(_dummyParticipantIdsVector[0])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(), getPredicate(_dummyParticipantIdsVector[2])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(), getPredicate(_dummyParticipantIdsVector[3])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(), getPredicate(_dummyParticipantIdsVector[4])) != result.cend());
    EXPECT_TRUE(std::find_if(result.cbegin(), result.cend(), getPredicate(_dummyParticipantIdsVector[5])) != result.cend());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_globalOnly_emptyGbidsVector_allCached)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList =
            getGlobalDiscoveryEntries(6);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(discoveryEntryResultList.size()),
                                       _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    const std::vector<std::string>& emptyGbids{};
    const std::uint8_t expectedReturnedGlobalEntries = 6;
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       emptyGbids,
                                       createLookupSuccessFunction(expectedReturnedGlobalEntries),
                                       _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_globalOnly_multipleGbids_noneCached)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList = getGlobalDiscoveryEntries(2);
    std::vector<std::string> multipleGbids{_KNOWN_GBIDS[0], _KNOWN_GBIDS[1]};

    // no entries cached, lookup of GlobalCapabilitiesDirectoryClient will be called
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(multipleGbids), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    const std::uint8_t& expectedReturnedGlobalEntries = 2;
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       multipleGbids,
                                       createLookupSuccessFunction(expectedReturnedGlobalEntries),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_globalOnly_emptyGbidsVector_noneCached)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    std::vector<types::GlobalDiscoveryEntry> discoveryEntryResultList = getGlobalDiscoveryEntries(2);

    // no entries cached, lookup of GlobalCapabilitiesDirectoryClient will be called
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(_KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntryResultList));

    const std::uint8_t expectedReturnedGlobalEntries = 2;
    std::vector<std::string> emptyGbidsVector{};
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       emptyGbidsVector,
                                       createLookupSuccessFunction(expectedReturnedGlobalEntries),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_globalOnly_localEntry_invokesGcd_returnsRemoteResultAndNotLocalEntry) {
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    std::vector<std::string> gbidsForLookup {_KNOWN_GBIDS[2], _KNOWN_GBIDS[0]};
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
    types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(false, remoteEntry);

    // register local entry
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->add(
                _entry,
                true,
                createAddOnSuccessFunction(),
                _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult {remoteEntry};

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(
                    Eq(_dummyParticipantIdsVector[0]),
                    Eq(expectedGbids),
                    Eq(_discoveryQos.getDiscoveryTimeout()),
                    _,
                    _,
                    _))
                .Times(1).WillOnce(InvokeArgument<3>(onSuccessResult));

    auto onSuccess = [this, expectedEntry]
            (const joynr::types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_EQ(expectedEntry, result);
        _semaphore.notify();
    };

    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
            _discoveryQos,
            gbidsForLookup,
            onSuccess,
            _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_globalOnly_multipleGbids_allCached) {
    std::vector<std::string> gbids{_KNOWN_GBIDS[0], _KNOWN_GBIDS[2]};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbids);

    gbids = {_KNOWN_GBIDS[1], _KNOWN_GBIDS[2]};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbids);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_globalOnly_multipleGbids_noneCached) {
    std::vector<std::string> gbidsForLookup{_KNOWN_GBIDS[0], _KNOWN_GBIDS[2]};
    std::vector<std::string> expectedGbids{gbidsForLookup};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(gbidsForLookup, expectedGbids);

    gbidsForLookup = {_KNOWN_GBIDS[1], _KNOWN_GBIDS[2]};
    expectedGbids = {gbidsForLookup};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(gbidsForLookup, expectedGbids);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_globalOnly_emptyGbidsVector_allCached) {
    std::vector<std::string> gbidsForLookup{};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbidsForLookup);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_globalOnly_emptyGbidsVector_noneCached) {
    std::vector<std::string> gbidsForLookup{};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(gbidsForLookup, _KNOWN_GBIDS);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbidsIsProperlyRejected_exception)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::
                             fakeCapabilitiesClientLookupWithDiscoveryException));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       _KNOWN_GBIDS,
                                       createUnexpectedLookupSuccessFunction(),
                                       createExpectedDiscoveryErrorFunction(
                                           types::DiscoveryError::INTERNAL_ERROR));
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbidsIsProperlyRejected_invalidGbid)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::Enum::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbidsIsProperlyRejected_unknownGbid)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::Enum::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbidsIsProperlyRejected_internalError)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbidsIsProperlyRejected_noEntryForSelectedBackend)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::NO_ENTRY_FOR_SELECTED_BACKENDS);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_exception)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    exceptions::DiscoveryException fakeDiscoveryException("fakeDiscoveryException");
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            lookup(Eq(_dummyParticipantIdsVector[1]), _, Eq(_discoveryQos.getDiscoveryTimeout()), _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<5>(fakeDiscoveryException));

    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[1],
                                       _discoveryQos,
                                       _KNOWN_GBIDS,
                                       createUnexpectedLookupParticipantIdSuccessFunction(),
                                       createExpectedDiscoveryErrorFunction(
                                           types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_invalidGbid)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_unknownGbid)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_internalError)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_noEntryForSelectedBackend)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::NO_ENTRY_FOR_SELECTED_BACKENDS);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbidsIsProperlyRejected_noEntryForParticipant)
{
    lookupByParticipantIdWithGbidsIsProperlyRejected(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_unknownGbids)
{
    const std::vector<std::string>& unknownGbids{"unknownGbid", _KNOWN_GBIDS[0]};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(unknownGbids, types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_unknownGbids)
{
    const std::vector<std::string>& unknownGbids{"unknownGbid", _KNOWN_GBIDS[0]};
    testLookupByParticipantIdWithGbids_gbidValidationFails(unknownGbids, types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_invalidGbid_emptyGbid)
{
    const std::vector<std::string>& emptyGbid{""};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(emptyGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_invalidGbid_emptyGbid)
{
    const std::vector<std::string>& emptyGbid{""};
    testLookupByParticipantIdWithGbids_gbidValidationFails(emptyGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterfaceWithGbids_invalidGbid_duplicateGbid)
{
    const std::vector<std::string>& duplicateGbid{_KNOWN_GBIDS[1], _KNOWN_GBIDS[0], _KNOWN_GBIDS[1]};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(duplicateGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantIdWithGbids_invalidGbid_duplicateGbid)
{
    const std::vector<std::string>& duplicateGbid{_KNOWN_GBIDS[1], _KNOWN_GBIDS[0], _KNOWN_GBIDS[1]};
    testLookupByParticipantIdWithGbids_gbidValidationFails(duplicateGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterface_globalOnly_emptyCache_invokesGcd)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    const std::vector<types::GlobalDiscoveryEntry>& discoveryEntriesResultList{};
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(_KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList));

    auto onSuccess = [this] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(0, result.size());
        _semaphore.notify();
    };

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       onSuccess,
                                       _unexpectedProviderRuntimeExceptionFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterface_globalOnly_remoteEntryInCache_doesNotInvokeGcd)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    const std::vector<types::GlobalDiscoveryEntry> discoveryEntriesResultList = getGlobalDiscoveryEntries(2);
    const std::vector<types::GlobalDiscoveryEntry> expectedResult = discoveryEntriesResultList;

    auto onSuccess = [this, &expectedResult]
            (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(expectedResult.size(), result.size());
        _semaphore.notify();
    };

    // add DiscoveryEntries to cache
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(_KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       onSuccess,
                                       _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

    // do actual lookup
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       onSuccess,
                                       _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterface_localThenGlobal_emptyCache_invokesGcd)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    const std::vector<types::GlobalDiscoveryEntry>& discoveryEntriesResultList{};
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(_KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList));

    auto onSuccess = [this] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        EXPECT_EQ(0, result.size());
        _semaphore.notify();
    };

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       onSuccess,
                                       _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterface_localThenGlobal_localEntryCached_doesNotInvokeGcd)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);

    // add local capability
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setQos(providerQos);
    const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(true, _entry);
    _localCapabilitiesDirectory->add(_entry, createAddOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // do actual lookup
    auto onSuccess = [this, &expectedEntry]
            (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        ASSERT_EQ(1, result.size());
        EXPECT_EQ(expectedEntry, result[0]);
        _semaphore.notify();
    };

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       onSuccess,
                                       _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByDomainInterface_localThenGlobal_remoteEntryCached_doesNotInvokeGcd)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    const std::vector<types::GlobalDiscoveryEntry> discoveryEntriesResultList = getGlobalDiscoveryEntries(1);
    const types::DiscoveryEntryWithMetaInfo expectedResult = util::convert(false, discoveryEntriesResultList[0]);

    auto onSuccess = [this, &expectedResult]
            (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
        ASSERT_EQ(1, result.size());
        EXPECT_EQ(expectedResult, result[0]);
        _semaphore.notify();
    };

    // add DiscoveryEntries to cache
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
            lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, Eq(_KNOWN_GBIDS), _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(discoveryEntriesResultList));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       onSuccess,
                                       _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       onSuccess,
                                       _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupByParticipantId_localThenGlobal_localEntry_doesNotInvokeGcdAndReturnsLocalEntry)
{
    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    // add local capability
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);
    const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(true, _entry);
    _localCapabilitiesDirectory->add(_entry, createAddOnSuccessFunction(), _defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    auto onSuccess = [this, &expectedEntry] (const types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_EQ(expectedEntry, result);
        _semaphore.notify();
    };

    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       onSuccess,
                                       _defaultProviderRuntimeExceptionError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
                                        _defaultExpiryDateMs,
                                        _PUBLIC_KEY_ID);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    _localCapabilitiesDirectory->add(_entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                remove(Eq(_dummyParticipantIdsVector[0]),
                       Eq(std::vector<std::string>{_KNOWN_GBIDS[0]}), _, _, _)).Times(1);

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0],
            _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemove_GcdNotCalledIfParticipantIsNotRegistered)
{
    // There is no discoveryEntry registered. Gcd won't be invoked. Lcd will call onRuntimeError
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                remove(_, _, _, _, _)).Times(0);

    exceptions::ProviderRuntimeException expectedException(
                    fmt::format("Failed to remove participantId: {}. ParticipantId is not "
                                "registered in cluster controller.",
                                _dummyParticipantIdsVector[0]));
    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0],
            _defaultOnSuccess, createExpectedProviderRuntimeExceptionFunction(expectedException));

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveUsesSameGbidOrderAsAdd)
{
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[0]});
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[1]});
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[0], _KNOWN_GBIDS[1] });
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[1], _KNOWN_GBIDS[0] });
}

TEST_F(LocalCapabilitiesDirectoryTest, reregisterGlobalCapabilities)
{
    const std::int64_t now =
                     std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now().time_since_epoch()).count();
    types::DiscoveryEntry entry1(_defaultProviderVersion,
                                 _DOMAIN_1_NAME,
                                 _INTERFACE_1_NAME,
                                 _dummyParticipantIdsVector[0],
                                 types::ProviderQos(),
                                 _lastSeenDateMs,
                                 now + _defaultExpiryDateMs,
                                 _PUBLIC_KEY_ID);
    types::DiscoveryEntry expectedEntry1(entry1);

    types::DiscoveryEntry entry2(_defaultProviderVersion,
                                 _DOMAIN_2_NAME,
                                 _INTERFACE_2_NAME,
                                 _dummyParticipantIdsVector[1],
                                 types::ProviderQos(),
                                 _lastSeenDateMs,
                                 now + _defaultExpiryDateMs,
                                 _PUBLIC_KEY_ID);
    types::DiscoveryEntry expectedEntry2(entry2);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _,
                    _,
                    _)).Times(1);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry2)),
                    _,
                    _,
                    _,
                    _)).Times(1);

    _localCapabilitiesDirectory->add(entry1, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    _localCapabilitiesDirectory->add(entry2, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

    // sleep to get new values of lastSeenDateMs and expiryDateMs in triggerGlobalProviderReregistration
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const std::int64_t newLastSeenDateMs =
                 std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch()).count();
    const std::int64_t newExpiryDateMs = newLastSeenDateMs + _defaultExpiryDateMs;

    expectedEntry1.setLastSeenDateMs(newLastSeenDateMs);
    expectedEntry1.setExpiryDateMs(newExpiryDateMs);
    expectedEntry2.setLastSeenDateMs(newLastSeenDateMs);
    expectedEntry2.setExpiryDateMs(newExpiryDateMs);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntryWithExtendedDates(expectedEntry1, _toleranceMs)),
                    _,
                    _,
                    _,
                    _)).Times(1);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntryWithExtendedDates(expectedEntry2, _toleranceMs)),
                    _,
                    _,
                    _,
                    _)).Times(1);

    bool onSuccessCalled = false;
    _localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const exceptions::ProviderRuntimeException&) { FAIL(); });

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());
    EXPECT_TRUE(onSuccessCalled);

    // check that the dates are also updated in local store and global cache
    auto onSuccess = [this, expectedEntry1] (const types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_TRUE(compareConvertedGlobalDiscoveryEntryWithExtendedDates(result, expectedEntry1, _toleranceMs));
        _semaphore.notify();
    };

    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
            _discoveryQos,
            std::vector<std::string> {},
            onSuccess,
            nullptr);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
            _discoveryQos,
            std::vector<std::string> {},
            onSuccess,
            nullptr);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       doNotReregisterDiscoveryEntriesFromGlobalCapabilitiesDirectory)
{
    const std::int64_t now =
                     std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now().time_since_epoch()).count();
    types::DiscoveryEntry entry1(_defaultProviderVersion,
                                 _DOMAIN_1_NAME,
                                 _INTERFACE_1_NAME,
                                 _dummyParticipantIdsVector[0],
                                 types::ProviderQos(),
                                 _lastSeenDateMs,
                                 now + _defaultExpiryDateMs,
                                 _PUBLIC_KEY_ID);
    types::DiscoveryEntry expectedEntry1(entry1);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(
                            AnConvertedGlobalDiscoveryEntry(entry1)),
                    _,
                    _,
                    _,
                    _)).Times(1);

    _localCapabilitiesDirectory->add(entry1, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    types::DiscoveryEntry entry2(_defaultProviderVersion,
                                 _DOMAIN_2_NAME,
                                 _INTERFACE_2_NAME,
                                 _dummyParticipantIdsVector[1],
                                 types::ProviderQos(),
                                 _lastSeenDateMs,
                                 _defaultExpiryDateMs,
                                 _PUBLIC_KEY_ID);

    _localCapabilitiesDirectory->registerReceivedCapabilities({{_EXTERNAL_ADDRESSES_VECTOR[0], entry2}});

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

    // sleep to get new values of lastSeenDateMs and expiryDateMs in triggerGlobalProviderReregistration
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const std::int64_t newLastSeenDateMs =
                 std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch()).count();
    const std::int64_t newExpiryDateMs = newLastSeenDateMs + _defaultExpiryDateMs;

    expectedEntry1.setLastSeenDateMs(newLastSeenDateMs);
    expectedEntry1.setExpiryDateMs(newExpiryDateMs);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(AllOf(Property(&joynr::types::GlobalDiscoveryEntry::getParticipantId,
                                   Eq(entry1.getParticipantId())),
                          Matcher<const types::GlobalDiscoveryEntry&>(
                              AnConvertedGlobalDiscoveryEntryWithExtendedDates(expectedEntry1, _toleranceMs))),
                _,
                _,
                _,
                _)).Times(1);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Property(&joynr::types::GlobalDiscoveryEntry::getParticipantId, Eq(entry2.getParticipantId())),
                _,
                _,
                _,
                _)).Times(0);

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
                                _defaultExpiryDateMs,
                                _PUBLIC_KEY_ID);

    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _))
            .Times(0);

    // sleep to get new values of lastSeenDateMs and expiryDateMs in triggerGlobalProviderReregistration
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
                                _defaultExpiryDateMs,
                                _PUBLIC_KEY_ID);
    types::DiscoveryEntry expectedEntry(entry);

    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _))
            .Times(0);

    // sleep to get new values of lastSeenDateMs and expiryDateMs in triggerGlobalProviderReregistration
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const std::int64_t newLastSeenDateMs =
                 std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch()).count();
    const std::int64_t newExpiryDateMs = newLastSeenDateMs + _defaultExpiryDateMs;
    expectedEntry.setLastSeenDateMs(newLastSeenDateMs);
    expectedEntry.setExpiryDateMs(newExpiryDateMs);

    bool onSuccessCalled = false;
    _localCapabilitiesDirectory->triggerGlobalProviderReregistration(
            [&onSuccessCalled]() { onSuccessCalled = true; },
            [](const exceptions::ProviderRuntimeException&) { FAIL(); });

    EXPECT_TRUE(onSuccessCalled);

    auto onSuccess = [this, expectedEntry] (const types::DiscoveryEntryWithMetaInfo& result) {
        EXPECT_TRUE(compareConvertedGlobalDiscoveryEntryWithExtendedDates(result, expectedEntry, _toleranceMs));
        _semaphore.notify();
    };

    _discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
            _discoveryQos,
            std::vector<std::string> {},
            onSuccess,
            nullptr);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addAddsToCache)
{
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_dummyParticipantIdsVector[0],
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& capabilities)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       types::ProviderQos(),
                                       _lastSeenDateMs,
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(_entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    const types::DiscoveryQos localDiscoveryQos;
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, addLocallyDoesNotCallCapabilitiesClient)
{
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(_,
                       _,
                       _,
                       A<std::function<void(const std::vector<types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       _,
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       providerQos,
                                       _lastSeenDateMs,
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    const types::DiscoveryQos localDiscoveryQos;
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressReturnsCachedValues)
{
    // simulate global capability directory would store two entries.
    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(onSuccessResult));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    // enries are now in cache, _globalCapabilitiesDirectoryClient should not be called.
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(_,
                       _,
                       _,
                       _,
                       A<std::function<void(const std::vector<types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       _,
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressDelegatesToCapabilitiesClient)
{
    // simulate global capability directory would store two entries.
    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(ElementsAre(_DOMAIN_1_NAME), _INTERFACE_1_NAME, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(onSuccessResult));

    auto onSuccess = [this] (const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
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
        _semaphore.notify();
    };

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       _KNOWN_GBIDS,
                                       onSuccess,
                                       _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
            .Times(1)
            .WillOnce(Invoke(
                    this, &LocalCapabilitiesDirectoryTest::fakeLookupByParticipantIdWithResult));

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

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
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupNoEntryForParticipant));

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createUnexpectedLookupParticipantIdSuccessFunction(),
                                       createExpectedDiscoveryErrorFunction(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT));
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupByParticipantIdWithResult));

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, clearRemovesEntries)
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
            .WillOnce(Invoke(
                    this, &LocalCapabilitiesDirectoryTest::fakeLookupByParticipantIdWithResult));

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectoryStore->clear();
    // retrieving capabilities will force a call to the backend as the cache is empty
    EXPECT_CALL(
            *_globalCapabilitiesDirectoryClient,
            lookup(_,
                   _,
                   _,
                   A<std::function<void(
                           const std::vector<types::GlobalDiscoveryEntry>& discoveryEntries)>>(),
                   _,
                   A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createUnexpectedLookupParticipantIdSuccessFunction(),
                                       _unexpectedOnDiscoveryErrorFunction);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerMultipleGlobalCapabilitiesCheckIfTheyAreMerged)
{

    types::ProviderQos qos;

    types::GlobalDiscoveryEntry globalDiscoveryEntryInfo1(_defaultProviderVersion,
                                                          _DOMAIN_1_NAME,
                                                          _INTERFACE_1_NAME,
                                                          _dummyParticipantIdsVector[0],
                                                          qos,
                                                          _lastSeenDateMs,
                                                          _defaultExpiryDateMs,
                                                          _PUBLIC_KEY_ID,
                                                          _LOCAL_ADDRESS);

    types::GlobalDiscoveryEntry globalDiscoveryEntryInfo2(_defaultProviderVersion,
                                                          _DOMAIN_2_NAME,
                                                          _INTERFACE_1_NAME,
                                                          _dummyParticipantIdsVector[1],
                                                          qos,
                                                          _lastSeenDateMs,
                                                          _defaultExpiryDateMs,
                                                          _PUBLIC_KEY_ID,
                                                          _LOCAL_ADDRESS);

    {
        InSequence inSequence;
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, add(globalDiscoveryEntryInfo1, _, _, _, _)).Times(1);
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient, add(globalDiscoveryEntryInfo2, _, _, _, _)).Times(1);
    }

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       qos,
                                       _lastSeenDateMs,
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(_entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    types::DiscoveryEntry entry2(_defaultProviderVersion,
                                        _DOMAIN_2_NAME,
                                        _INTERFACE_1_NAME,
                                        _dummyParticipantIdsVector[1],
                                        qos,
                                        _lastSeenDateMs,
                                        _defaultExpiryDateMs,
                                        _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(entry2, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
}

TEST_F(LocalCapabilitiesDirectoryTest, removeLocalCapabilityByParticipantId)
{
    types::ProviderQos qos;
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

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       qos,
                                       _lastSeenDateMs,
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(_entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

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
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    // JoynrTimeOutException timeoutException;
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
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       providerQos,
                                       _lastSeenDateMs,
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(0),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // disable cache
    localDiscoveryQos.setCacheMaxAge(0);
    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    EXPECT_THROW(_localCapabilitiesDirectory->lookup(
                     {_DOMAIN_1_NAME},
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
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    // _localCapabilitiesDirectory->registerReceivedCapabilities(_globalCapEntryMap);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessZeroResult{};
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(2)
            //            .WillOnce(InvokeWithoutArgs(this,
            //            &LocalCapabilitiesDirectoryTest::simulateTimeout));
            .WillRepeatedly(InvokeArgument<4>(onSuccessZeroResult));
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(0),
                                       _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // disable cache
    localDiscoveryQos.setCacheMaxAge(0);
    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeArgument<4>(onSuccessResult));
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    _localCapabilitiesDirectoryStore->clear();

    localDiscoveryQos.setCacheMaxAge(4000);
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalPendingLocalEntryAdded_ReturnsLocalEntry)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    Semaphore gcdSemaphore(0);
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
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&gcdSemaphore),
                  InvokeArgument<4>(onSuccessResult)));

    auto thread = std::thread([&]() {
        _localCapabilitiesDirectory->lookup(
                    {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                    _INTERFACE_1_NAME,
                    localDiscoveryQos,
                    _KNOWN_GBIDS,
                    createLookupSuccessFunction(1),
                    _unexpectedOnDiscoveryErrorFunction);
    });

    EXPECT_FALSE(_semaphore.waitFor(std::chrono::milliseconds(100)));
    EXPECT_TRUE(_localCapabilitiesDirectory->hasPendingLookups());

    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(_localCapabilitiesDirectory->hasPendingLookups());

    // _globalCapabilitiesDirectoryClient.lookup should return;
    gcdSemaphore.notify();
    thread.join();

    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalSucceedsNoLocalEntries_ReturnsGlobalEntries)
{
    Semaphore gcdSemaphore(0);
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&gcdSemaphore),
                  InvokeArgument<4>(onSuccessResult)));

    auto thread = std::thread([&]() {
        _localCapabilitiesDirectory->lookup(
                    {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                    _INTERFACE_1_NAME,
                    localDiscoveryQos,
                    _KNOWN_GBIDS,
                    createLookupSuccessFunction(2),
                    _unexpectedOnDiscoveryErrorFunction);
    });

    EXPECT_FALSE(_semaphore.waitFor(std::chrono::milliseconds(100)));
    EXPECT_TRUE(_localCapabilitiesDirectory->hasPendingLookups());

    // _globalCapabilitiesDirectoryClient.lookup should return;
    gcdSemaphore.notify();
    thread.join();

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalThenGlobal_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    Semaphore gcdSemaphore(0);
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            DoAll(AcquireSemaphore(&gcdSemaphore),
                  Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithDiscoveryException)));

    auto thread = std::thread([&]() {
        _localCapabilitiesDirectory->lookup(
                    {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                    _INTERFACE_1_NAME,
                    localDiscoveryQos,
                    _KNOWN_GBIDS,
                    createUnexpectedLookupSuccessFunction(),
                    createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    });

    EXPECT_FALSE(_semaphore.waitFor(std::chrono::milliseconds(100)));
    EXPECT_TRUE(_localCapabilitiesDirectory->hasPendingLookups());

    // _globalCapabilitiesDirectoryClient.lookup should return;
    gcdSemaphore.notify();
    thread.join();

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->lookup(
                {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                _INTERFACE_1_NAME,
                localDiscoveryQosryQos,
                _KNOWN_GBIDS,
                createLookupSuccessFunction(1),
                _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            InvokeArgument<4>(onSuccessResult));

    _localCapabilitiesDirectory->lookup(
                {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                _INTERFACE_1_NAME,
                localDiscoveryQos,
                _KNOWN_GBIDS,
                createLookupSuccessFunction(2),
                _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupLocalAndGlobal_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithDiscoveryException));

    _localCapabilitiesDirectory->lookup(
                {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                _INTERFACE_1_NAME,
                localDiscoveryQos,
                _KNOWN_GBIDS,
                createUnexpectedLookupSuccessFunction(),
                createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            InvokeArgument<4>(onSuccessResult));

    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->lookup(
                {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                _INTERFACE_1_NAME,
                localDiscoveryQos,
                _KNOWN_GBIDS,
                createLookupSuccessFunction(3),
                _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithDiscoveryException));

    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->lookup(
                {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                _INTERFACE_1_NAME,
                localDiscoveryQos,
                _KNOWN_GBIDS,
                createUnexpectedLookupSuccessFunction(),
                createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            InvokeArgument<4>(onSuccessResult));

    _localCapabilitiesDirectory->lookup(
                {_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                _INTERFACE_1_NAME,
                localDiscoveryQos,
                _KNOWN_GBIDS,
                createLookupSuccessFunction(2),
                _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest, removeGlobalExpiredEntries_ReturnNonExpiredGlobalEntries)
{
    // add a few (remote) entries to the global cache
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    _defaultExpiryDateMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch()).count() +
                   _purgeExpiredDiscoveryEntriesIntervalMs;

    std::int64_t longerExpiryDateMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count() +
            _purgeExpiredDiscoveryEntriesIntervalMs * 10;

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
                                        _defaultExpiryDateMs,
                                        _PUBLIC_KEY_ID,
                                        _EXTERNAL_ADDRESSES_VECTOR[0]);

    types::GlobalDiscoveryEntry entry3(_defaultProviderVersion,
                                        _DOMAIN_1_NAME,
                                        _INTERFACE_3_NAME,
                                        _dummyParticipantIdsVector[2],
                                        providerQos,
                                        _lastSeenDateMs,
                                        _defaultExpiryDateMs,
                                        _PUBLIC_KEY_ID,
                                        _EXTERNAL_ADDRESSES_VECTOR[1]);

    std::vector<types::GlobalDiscoveryEntry> expectedResultInterface1 = { entry1, entry2 };
    std::vector<types::GlobalDiscoveryEntry> expectedResultInterface3 = { entry3 };
    std::vector<types::GlobalDiscoveryEntry> emptyExpectedResult = {};

    const std::vector<std::string> domainVector = {_DOMAIN_1_NAME};


    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(10000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(Eq(domainVector), Eq(_INTERFACE_1_NAME), _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeArgument<4>(expectedResultInterface1));
    _localCapabilitiesDirectory->lookup(domainVector,
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       _unexpectedOnDiscoveryErrorFunction);

    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(Eq(domainVector), Eq(_INTERFACE_3_NAME), _, _, _, _, _))
            .WillOnce(InvokeArgument<4>(expectedResultInterface3));
    _localCapabilitiesDirectory->lookup(domainVector,
                                       _INTERFACE_3_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(_purgeExpiredDiscoveryEntriesIntervalMs * 2));
    // cache should now only contain entry1

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

    // 1 cached entry still available for interface 1
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(Eq(domainVector), Eq(_INTERFACE_1_NAME), _, _, _, _, _))
            .Times(0);
    _localCapabilitiesDirectory->lookup(domainVector,
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());

    // The only entry for interface 3 is expired by now, so the GCD is called again
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, Eq(_INTERFACE_3_NAME), _, _, _, _, _))
            .WillOnce(InvokeArgument<4>(emptyExpectedResult));
    _localCapabilitiesDirectory->lookup(domainVector,
                                       _INTERFACE_3_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(0),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupGlobalOnly_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithDiscoveryException));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createUnexpectedLookupSuccessFunction(),
                                       createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_FALSE(_localCapabilitiesDirectory->hasPendingLookups());
}

TEST_F(LocalCapabilitiesDirectoryTest,
       lookupGlobalOnly_GlobalSucceedsLocalEntries_ReturnsGlobalEntries)
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
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    const std::vector<types::GlobalDiscoveryEntry>& onSuccessResult = getGlobalDiscoveryEntries(2);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            InvokeArgument<4>(onSuccessResult));

    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       _unexpectedOnDiscoveryErrorFunction);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillRepeatedly(
            Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientLookupWithDiscoveryException));

    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME, _DOMAIN_2_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createUnexpectedLookupSuccessFunction(),
                                       createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
                                        _defaultExpiryDateMs,
                                        _PUBLIC_KEY_ID);
    types::DiscoveryEntry entry2(_defaultProviderVersion,
                                        multipleDomainName2,
                                        _INTERFACE_1_NAME,
                                        multipleDomainName2ParticipantId,
                                        providerQos,
                                        _lastSeenDateMs,
                                        _defaultExpiryDateMs,
                                        _PUBLIC_KEY_ID);
    types::DiscoveryEntry entry31(_defaultProviderVersion,
                                         multipleDomainName3,
                                         _INTERFACE_1_NAME,
                                         multipleDomainName3ParticipantId1,
                                         providerQos,
                                         _lastSeenDateMs,
                                         _defaultExpiryDateMs,
                                         _PUBLIC_KEY_ID);
    types::DiscoveryEntry entry32(_defaultProviderVersion,
                                         multipleDomainName3,
                                         _INTERFACE_1_NAME,
                                         multipleDomainName3ParticipantId2,
                                         providerQos,
                                         _lastSeenDateMs,
                                         _defaultExpiryDateMs,
                                         _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(entry1, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->add(entry2, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->add(entry31, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->add(entry32, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                lookup(_,
                       _,
                       _,
                       A<std::function<void(const std::vector<types::GlobalDiscoveryEntry>&
                                                    discoveryEntries)>>(),
                       _,
                       A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(0);

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
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupGlobalOnly)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(0);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       providerQos,
                                       _lastSeenDateMs,
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    // JoynrTimeOutException timeoutException;
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
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

    // disable cache
    localDiscoveryQos.setCacheMaxAge(0);
    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
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
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       providerQos,
                                       _lastSeenDateMs,
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(_entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _)).Times(1);
    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupLocalThenGlobal)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(100);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       providerQos,
                                       _lastSeenDateMs,
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(_entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));

    // get the local entry
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _)).Times(1);
    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);

    // get the global entry
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // wait for cleanup timer to run
    std::this_thread::sleep_for(
            std::chrono::milliseconds(_purgeExpiredDiscoveryEntriesIntervalMs * 2));

    // get the global, but timeout occured
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    EXPECT_THROW(_localCapabilitiesDirectory->lookup(
                     {_DOMAIN_1_NAME},
                     _INTERFACE_1_NAME,
                     localDiscoveryQos,
                     _KNOWN_GBIDS,
                     createUnexpectedLookupSuccessFunction(),
                     _unexpectedOnDiscoveryErrorFunction),
                 exceptions::JoynrTimeOutException);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerCachedGlobalCapability_lookupGlobalOnly)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(100);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    // JoynrTimeOutException timeoutException;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       providerQos,
                                       _lastSeenDateMs,
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    _localCapabilitiesDirectory->add(_entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // recieve a global entry
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntryMap));
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(2),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, remove(_dummyParticipantIdsVector[0], _, _, _, _)).Times(1);
    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerReceivedCapabilites_registerMqttAddress)
{
    const std::string& addressType = "mqtt";
    const std::string& topic = "mqtt_TEST_channelId";
    system::RoutingTypes::MqttAddress mqttAddress("brokerUri", topic);
    registerReceivedCapabilities(addressType, serializer::serializeToJson(mqttAddress));
}

TEST_F(LocalCapabilitiesDirectoryTest, registerReceivedCapabilites_registerHttpAddress)
{
    const std::string& addressType = "http";
    const std::string& channelID = "TEST_channelId";
    const std::string& endPointUrl = "TEST_endPointUrl";
    const system::RoutingTypes::ChannelAddress channelAddress(endPointUrl, channelID);
    registerReceivedCapabilities(addressType, serializer::serializeToJson(channelAddress));
}

TEST_F(LocalCapabilitiesDirectoryTest, persistencyTest)
{
    _localCapabilitiesDirectory = std::make_shared<LocalCapabilitiesDirectory>(
            _clusterControllerSettingsForPersistencyTests,
            _globalCapabilitiesDirectoryClient,
            _localCapabilitiesDirectoryStoreForPersistencyTests,
            _LOCAL_ADDRESS,
            _mockMessageRouter,
            _singleThreadedIOService->getIOService(),
            _clusterControllerId,
            _KNOWN_GBIDS,
            _defaultExpiryDateMs);
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
                                              _lastSeenDateMs,
                                              _defaultExpiryDateMs,
                                              _PUBLIC_KEY_ID);
    const types::DiscoveryEntry entry2(_defaultProviderVersion,
                                              DOMAIN_NAME,
                                              INTERFACE_NAME,
                                              participantIds[1],
                                              globalProviderQos,
                                              _lastSeenDateMs,
                                              _defaultExpiryDateMs,
                                              _PUBLIC_KEY_ID);
    const types::DiscoveryEntry entry3(_defaultProviderVersion,
                                              DOMAIN_NAME,
                                              INTERFACE_NAME,
                                              participantIds[2],
                                              globalProviderQos,
                                              _lastSeenDateMs,
                                              _defaultExpiryDateMs,
                                              _PUBLIC_KEY_ID);

    _localCapabilitiesDirectory->add(entry1, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->add(entry2, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    _localCapabilitiesDirectory->add(entry3,
                                    false,
                                    {_KNOWN_GBIDS[1], _KNOWN_GBIDS[2]},
                                    _defaultOnSuccess,
                                    _unexpectedOnDiscoveryErrorFunction);

    // create a new object
    const std::int64_t defaultExpiryDateMs = 600000;
    auto localCapabilitiesDirectory2 =
            std::make_shared<LocalCapabilitiesDirectory>(_clusterControllerSettingsForPersistencyTests,
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
        EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    }

    auto cachedGlobalDiscoveryEntries = localCapabilitiesDirectory2->getCachedGlobalDiscoveryEntries();
    EXPECT_EQ(2, cachedGlobalDiscoveryEntries.size());
    EXPECT_EQ(entry2, cachedGlobalDiscoveryEntries[0]);
    EXPECT_EQ(entry3, cachedGlobalDiscoveryEntries[1]);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _))
            .WillRepeatedly(InvokeArgument<4>(types::DiscoveryError::INTERNAL_ERROR));
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    // check entry2 is registered only in backend 0
    localCapabilitiesDirectory2->lookup(participantIds[1],
                                        localDiscoveryQos,
                                        {_KNOWN_GBIDS[0]},
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    localCapabilitiesDirectory2->lookup(participantIds[1],
                                        localDiscoveryQos,
                                        {_KNOWN_GBIDS[1], _KNOWN_GBIDS[2]},
                                        createUnexpectedLookupParticipantIdSuccessFunction(),
                                        createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    // check entry3 is registered only in backend 1 and backend 2
    localCapabilitiesDirectory2->lookup(participantIds[2],
                                        localDiscoveryQos,
                                        {_KNOWN_GBIDS[0]},
                                        createUnexpectedLookupParticipantIdSuccessFunction(),
                                        createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    localCapabilitiesDirectory2->lookup(participantIds[2],
                                        localDiscoveryQos,
                                        {_KNOWN_GBIDS[1]},
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    localCapabilitiesDirectory2->lookup(participantIds[2],
                                        localDiscoveryQos,
                                        {_KNOWN_GBIDS[2]},
                                        createLookupParticipantIdSuccessFunction(),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, loadCapabilitiesFromFile)
{
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
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->lookup("notReachableInterface_Heisenberg",
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupParticipantIdSuccessFunction(),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, throwExceptionOnEmptyDomainsVector)
{
    const std::vector<std::string>& zeroDomains = {};
    const std::vector<std::string>& twoDomains = {_DOMAIN_1_NAME, _DOMAIN_2_NAME};
    MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>> mockCallback;
    auto onSuccess = std::bind(
            &MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>>::onSuccess,
            &mockCallback,
            std::placeholders::_1);
    auto onError =
            std::bind(&MockCallback<std::vector<types::DiscoveryEntryWithMetaInfo>>::onError,
                      &mockCallback,
                      std::placeholders::_1);

    EXPECT_CALL(mockCallback, onFatalRuntimeError(_)).Times(0);
    EXPECT_CALL(mockCallback, onError(_)).Times(0);
    EXPECT_CALL(mockCallback, onSuccess(_)).Times(0);
    EXPECT_THROW(_localCapabilitiesDirectory->lookup(
                     zeroDomains,
                     _INTERFACE_1_NAME,
                     _discoveryQos,
                     onSuccess,
                     onError),
                 exceptions::ProviderRuntimeException);

    EXPECT_CALL(mockCallback, onError(_)).Times(0);
    EXPECT_CALL(mockCallback, onSuccess(_)).Times(0);
    EXPECT_NO_THROW(_localCapabilitiesDirectory->lookup(
                     twoDomains,
                     _INTERFACE_1_NAME,
                     _discoveryQos,
                     onSuccess,
                     onError));
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
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    const types::DiscoveryEntryWithMetaInfo expectedEntry = util::convert(true, entry);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    _localCapabilitiesDirectory->add(_entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);

    auto onSuccess = [this, expectedEntry] (const std::vector<types::DiscoveryEntryWithMetaInfo>& results) {
        EXPECT_EQ(1, results.size());
        const types::DiscoveryEntryWithMetaInfo& result = results.at(0);
        EXPECT_EQ(expectedEntry, result);
        _semaphore.notify();
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
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
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
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);
    const types::DiscoveryEntryWithMetaInfo& expectedEntry = util::convert(true, entry);
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
                add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _)).Times(1);

    _localCapabilitiesDirectory->add(_entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(1).WillOnce(
            InvokeArgument<4>(globalEntryVec));

    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(0);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    // wait some time to be sure that the cached entry is not used
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    auto onSuccess = [this, expectedEntry] (const std::vector<types::DiscoveryEntryWithMetaInfo>& results) {
        EXPECT_EQ(1, results.size());
        const types::DiscoveryEntryWithMetaInfo& result = results.at(0);
        EXPECT_EQ(expectedEntry, result);
        _semaphore.notify();
    };
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       localDiscoveryQos,
                                       _KNOWN_GBIDS,
                                       onSuccess,
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, callTouchPeriodically)
{
    // make sure that there is only one runtime that is periodically calling touch
    test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, touch(_, _, _, _)).Times(0);
    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());
    Semaphore gcdSemaphore(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, touch(Eq(_clusterControllerId), _, _, _))
            .Times(2)
            .WillRepeatedly(ReleaseSemaphore(&gcdSemaphore));
    EXPECT_TRUE(gcdSemaphore.waitFor(std::chrono::milliseconds(250)));
    EXPECT_FALSE(gcdSemaphore.waitFor(std::chrono::milliseconds(150)));
    EXPECT_TRUE(gcdSemaphore.waitFor(std::chrono::milliseconds(100)));
}

TEST_F(LocalCapabilitiesDirectoryTest, callTouchWithOnlyGlobalParticipantIds_RefreshesEntries)
{
    // make sure that there is only one runtime that is periodically calling touch
    test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);
    // Fill the LCD
    std::string participantId1 = "participantId1";
    std::string participantId2 = "participantId2";
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);
    _entry.setParticipantId(participantId1);
    _localCapabilitiesDirectory->add(
            _entry,
            createAddOnSuccessFunction(),
            _unexpectedProviderRuntimeExceptionFunction);

    std::int64_t oldLastSeenDate = 0;
    std::int64_t oldExpiryDate = 0;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setLastSeenDateMs(oldLastSeenDate);
    _entry.setExpiryDateMs(oldLastSeenDate);
    _entry.setQos(providerQos);
    _entry.setParticipantId(participantId2);
    _localCapabilitiesDirectory->add(
            _entry,
            createAddOnSuccessFunction(),
            _unexpectedProviderRuntimeExceptionFunction);

    std::vector<std::string> expectedParticipantIds { participantId2 };

    // wait for 2 add calls
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, touch(_, _, _, _)).Times(0);
    Mock::VerifyAndClearExpectations(_globalCapabilitiesDirectoryClient.get());
    Semaphore gcdSemaphore(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, touch(Eq(_clusterControllerId), Eq(expectedParticipantIds), _, _))
            .WillOnce(ReleaseSemaphore(&gcdSemaphore));
    EXPECT_TRUE(gcdSemaphore.waitFor(std::chrono::milliseconds(250)));

    ASSERT_TRUE(oldLastSeenDate < _localCapabilitiesDirectoryStore->getCachedLocalCapabilities(participantId2)[0].getLastSeenDateMs());
    ASSERT_TRUE(oldExpiryDate < _localCapabilitiesDirectoryStore->getCachedLocalCapabilities(participantId2)[0].getExpiryDateMs());
    ASSERT_TRUE(oldLastSeenDate <
                _localCapabilitiesDirectoryStore->getGlobalLookupCache()->lookupByParticipantId(participantId2).get().getLastSeenDateMs());
    ASSERT_TRUE(oldExpiryDate <
                _localCapabilitiesDirectoryStore->getGlobalLookupCache()->lookupByParticipantId(participantId2).get().getExpiryDateMs());
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
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);

    // trigger a failure on the first call and a success on the second

    // 1st call
    Semaphore gcdSemaphore(0);
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _))
            .WillOnce(DoAll(
                          Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddWithException),
                          ReleaseSemaphore(&gcdSemaphore)
                      )); // invoke onError
    _localCapabilitiesDirectory->add(_entry, true, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    // wait for it...
    EXPECT_TRUE(gcdSemaphore.waitFor(std::chrono::seconds(1)));

    // 2nd call
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _))
            .WillOnce(DoAll(
                          Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddSuccess),
                          ReleaseSemaphore(&gcdSemaphore)
                      )); // invoke onSuccess
    _localCapabilitiesDirectory->add(_entry, true, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    // wait for it...
    EXPECT_TRUE(gcdSemaphore.waitFor(std::chrono::seconds(1)));

    // do a lookup to make sure the entry still exists
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _)).Times(0);
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

    _localCapabilitiesDirectory->setAccessController(util::as_weak_ptr(mockAccessController));

    types::DiscoveryEntry entry(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       types::ProviderQos(),
                                       _lastSeenDateMs,
                                       _defaultExpiryDateMs,
                                       _PUBLIC_KEY_ID);

    try {
        _localCapabilitiesDirectory->add(_entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
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
        _localCapabilitiesDirectory->lookup(_dummyParticipantIdsVector[0],
                                           localDiscoveryQos,
                                           _KNOWN_GBIDS,
                                           createUnexpectedLookupParticipantIdSuccessFunction(),
                                           createExpectedDiscoveryErrorFunction(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT));
    }
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

std::tuple<bool, bool> const LCDWithAC_UseCases[] = {
        // Access controller enabled/disabled: tuple[0]
        // Emulation of "Has and not have" permission: tuple[1]
        make_tuple(false, false),
        make_tuple(false, true),
        make_tuple(true, false),
        make_tuple(true, true)};

INSTANTIATE_TEST_CASE_P(WithAC,
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
                std::make_shared<LocalDomainAccessController>(localDomainAccessStore, true);
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
                                          _defaultExpiryDateMs,
                                          _PUBLIC_KEY_ID);

    types::DiscoveryEntry NOT_OK_entry_1(
            _defaultProviderVersion,
            "domain-1234",                // domain is OK
            "my/favourite/interface/Nam", // interfaceName is a substring of the allowed one
            _dummyParticipantIdsVector[0],
            types::ProviderQos(),
            _lastSeenDateMs,
            _defaultExpiryDateMs,
            _PUBLIC_KEY_ID);

    types::DiscoveryEntry NOT_OK_entry_2(
            _defaultProviderVersion,
            "domain-123",                  // domain is a substring of the allowed one
            "my/favourite/interface/Name", // interfaceName is OK
            _dummyParticipantIdsVector[0],
            types::ProviderQos(),
            _lastSeenDateMs,
            _defaultExpiryDateMs,
            _PUBLIC_KEY_ID);

    std::string principal = "testUser";
    CallContext callContext;
    callContext.setPrincipal(principal);
    CallContextStorage::set(std::move(callContext));

    _localCapabilitiesDirectory->add(
                     OK_entry,
                     []() { SUCCEED() << "OK"; },
                     [](const exceptions::ProviderRuntimeException& ex) {
                         FAIL() << ex.getMessage();
                     });

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
                    add(Matcher<const types::GlobalDiscoveryEntry&>(_), _, _, _, _))
                .Times(numberOfDuplicatedEntriesToAdd)
                .WillRepeatedly(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeCapabilitiesClientAddSuccess));
    }

    for (int i = 0; i < numberOfDuplicatedEntriesToAdd; ++i) {
        // change expiryDate and lastSeen so that entries are not exactly equal
        _lastSeenDateMs++;
        _defaultExpiryDateMs++;

        types::DiscoveryEntry entry(_defaultProviderVersion,
                                           _DOMAIN_1_NAME,
                                           _INTERFACE_1_NAME,
                                           _dummyParticipantIdsVector[0],
                                           providerQos,
                                           _lastSeenDateMs,
                                           _defaultExpiryDateMs,
                                           _PUBLIC_KEY_ID);
            _localCapabilitiesDirectory->add(_entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);
    }

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                       _INTERFACE_1_NAME,
                                       _discoveryQos,
                                       _KNOWN_GBIDS,
                                       createLookupSuccessFunction(1),
                                       _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryTest, testRemoveStaleProvidersOfClusterController)
{
    std::int64_t maxLastSeenMs = 100000.0;
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, removeStale(Eq(_clusterControllerId), maxLastSeenMs, _, _)).Times(1);
    _localCapabilitiesDirectory->removeStaleProvidersOfClusterController(maxLastSeenMs);
}

INSTANTIATE_TEST_CASE_P(changeProviderScope,
                        LocalCapabilitiesDirectoryWithProviderScope,
                        ::testing::Values(types::ProviderScope::LOCAL,
                                          types::ProviderScope::GLOBAL));
