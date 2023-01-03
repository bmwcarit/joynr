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

#include "tests/unit-tests/lcd/LocalCapabilitiesDirectoryAbstract.h"

using ::testing::InSequence;
using ::testing::Matcher;
using ::testing::Sequence;
using namespace ::testing;
using namespace joynr;

class LocalCapabilitiesDirectoryOtherTest : public AbstractLocalCapabilitiesDirectoryTest
{
public:
    LocalCapabilitiesDirectoryOtherTest()
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

    ~LocalCapabilitiesDirectoryOtherTest() override
    {
        _singleThreadedIOService->stop();
        test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectory);
        test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);

        test::util::removeFileInCurrentDirectory(".*\\.settings");
        test::util::removeFileInCurrentDirectory(".*\\.persist");
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
    
    std::function<void(const types::DiscoveryEntryWithMetaInfo&)>
    createUnexpectedLookupParticipantIdSuccessFunction()
    {
        return [](const types::DiscoveryEntryWithMetaInfo& result) {
            FAIL() << "Got result: " + result.toString();
        };
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

    std::function<void()> createUnexpectedAddOnSuccessFunction()
    {
        return []() { FAIL(); };
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

        std::vector<std::string> capturedGbids;
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    remove(Eq(_dummyParticipantIdsVector[0]), _, _, _, _))
                .InSequence(s)
                .WillOnce(DoAll(SaveArg<1>(&capturedGbids), ReleaseSemaphore(_semaphore)));

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
        EXPECT_TRUE(expectedGbids.size() > 0 && capturedGbids.size() > 0);
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

        void checkAddToGcdClient(const std::vector<std::string>& expectedGbids, Sequence& s)
    {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    add(GlobalDiscoveryEntryMatcher(_expectedGlobalCapEntry), _, Eq(expectedGbids),
                        _, _, _))
                .Times(1)
                .InSequence(s)
                .WillOnce(InvokeArgument<3>());
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

    std::function<void(const types::DiscoveryError::Enum&)> createExpectedDiscoveryErrorFunction(
            const types::DiscoveryError::Enum& expectedError)
    {
        return [this, expectedError](const types::DiscoveryError::Enum& error) {
            EXPECT_EQ(expectedError, error);
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

        types::GlobalDiscoveryEntry capEntry;
        capEntry.setParticipantId(participantId);
        capEntry.setAddress(serializedAddress);
        auto validGlobalEntries =
                _localCapabilitiesDirectory->registerReceivedCapabilities({capEntry});
        ASSERT_EQ(validGlobalEntries.size(), 1);
        ASSERT_TRUE(validGlobalEntries[0].getParticipantId() == capEntry.getParticipantId());
    }

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

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectoryOtherTest);
};

const std::vector<std::string> LocalCapabilitiesDirectoryOtherTest::_KNOWN_GBIDS{
        "testGbid1", "testGbid2", "testGbid3"};
const std::string LocalCapabilitiesDirectoryOtherTest::_INTERFACE_1_NAME("myInterfaceA");
const std::string LocalCapabilitiesDirectoryOtherTest::_INTERFACE_2_NAME("myInterfaceB");
const std::string LocalCapabilitiesDirectoryOtherTest::_INTERFACE_3_NAME("myInterfaceC");
const std::string LocalCapabilitiesDirectoryOtherTest::_DOMAIN_1_NAME("domainA");
const std::string LocalCapabilitiesDirectoryOtherTest::_DOMAIN_2_NAME("domainB");
const std::string LocalCapabilitiesDirectoryOtherTest::_DOMAIN_3_NAME("domainB");
const std::string LocalCapabilitiesDirectoryOtherTest::_LOCAL_ADDRESS(serializer::serializeToJson(
        system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "localTopic")));
const std::vector<std::string> LocalCapabilitiesDirectoryOtherTest::_EXTERNAL_ADDRESSES_VECTOR{
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "externalTopic")),
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[1], "externalTopic")),
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[2], "externalTopic"))};
const std::int64_t LocalCapabilitiesDirectoryOtherTest::_LASTSEEN_MS(1000);
const std::int64_t LocalCapabilitiesDirectoryOtherTest::_EXPIRYDATE_MS(10000);
const std::string LocalCapabilitiesDirectoryOtherTest::_PUBLIC_KEY_ID("publicKeyId");
const int LocalCapabilitiesDirectoryOtherTest::_TIMEOUT(2000);

TEST_F(LocalCapabilitiesDirectoryOtherTest, removeCapabilities_invokesGcdClient)
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
                remove(Eq(_dummyParticipantIdsVector[0]), _, _, _, _))
                .Times(1)
                .InSequence(s);

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            _entry, _defaultOnSuccess, _defaultProviderRuntimeExceptionError);

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], _defaultOnSuccess,
                                        _defaultProviderRuntimeExceptionError);
}

TEST_F(LocalCapabilitiesDirectoryOtherTest, testRemoveGlobal_participantNotRegisteredNoGbids_GcdCalled)
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

TEST_F(LocalCapabilitiesDirectoryOtherTest, testRemove_localProvider_GcdNotCalled)
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

TEST_F(LocalCapabilitiesDirectoryOtherTest, testRemoveUsesSameGbidOrderAsAdd_1)
{
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[0]});
}

TEST_F(LocalCapabilitiesDirectoryOtherTest, testRemoveUsesSameGbidOrderAsAdd_2)
{
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[1]});
}

TEST_F(LocalCapabilitiesDirectoryOtherTest, testRemoveUsesSameGbidOrderAsAdd_3)
{
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[0], _KNOWN_GBIDS[1]});
}

TEST_F(LocalCapabilitiesDirectoryOtherTest, testRemoveUsesSameGbidOrderAsAdd_4)
{
    testRemoveUsesSameGbidOrderAsAdd({_KNOWN_GBIDS[1], _KNOWN_GBIDS[0]});
}

TEST_F(LocalCapabilitiesDirectoryOtherTest, reregisterGlobalCapabilities)
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

TEST_F(LocalCapabilitiesDirectoryOtherTest,
       doNotReregisterDiscoveryEntriesFromGlobalCapabilitiesDirectory)
{
    types::GlobalDiscoveryEntry entry1(_defaultProviderVersion,
                                       _DOMAIN_1_NAME,
                                       _INTERFACE_1_NAME,
                                       _dummyParticipantIdsVector[0],
                                       types::ProviderQos(),
                                       _lastSeenDateMs,
                                       _lastSeenDateMs + _defaultExpiryIntervalMs,
                                       _PUBLIC_KEY_ID,
                                       _EXTERNAL_ADDRESSES_VECTOR[0]);
    types::GlobalDiscoveryEntry expectedEntry1(entry1);

    types::GlobalDiscoveryEntry entry2(_defaultProviderVersion,
                                       _DOMAIN_2_NAME,
                                       _INTERFACE_2_NAME,
                                       _dummyParticipantIdsVector[1],
                                       types::ProviderQos(),
                                       _lastSeenDateMs,
                                       _lastSeenDateMs + _defaultExpiryIntervalMs,
                                       _PUBLIC_KEY_ID,
                                       _EXTERNAL_ADDRESSES_VECTOR[0]);

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
    _localCapabilitiesDirectory->registerReceivedCapabilities({entry2});

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

TEST_F(LocalCapabilitiesDirectoryOtherTest,
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

TEST_F(LocalCapabilitiesDirectoryOtherTest,
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

TEST_F(LocalCapabilitiesDirectoryOtherTest, clearRemovesEntries)
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
                    this, &LocalCapabilitiesDirectoryOtherTest::fakeLookupByParticipantIdWithResult));
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

TEST_F(LocalCapabilitiesDirectoryOtherTest, removeLocalCapabilityByParticipantId)
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
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryOtherTest::simulateTimeout));

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

TEST_F(LocalCapabilitiesDirectoryOtherTest, registerLocalCapability_lookupLocal)
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

    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntries));

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

TEST_F(LocalCapabilitiesDirectoryOtherTest, removeGlobalExpiredEntries_ReturnNonExpiredGlobalEntries)
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

TEST_F(LocalCapabilitiesDirectoryOtherTest, registerReceivedCapabilites_registerMqttAddress)
{
    const std::string& addressType = "mqtt";
    const std::string& topic = "mqtt_TEST_channelId";
    system::RoutingTypes::MqttAddress mqttAddress("brokerUri", topic);
    registerReceivedCapabilities(addressType, serializer::serializeToJson(mqttAddress));
}

TEST_F(LocalCapabilitiesDirectoryOtherTest, callTouchPeriodically)
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

TEST_F(LocalCapabilitiesDirectoryOtherTest, touchNotCalled_noParticipantIdsToTouch_entryHasLocalScope)
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

TEST_F(LocalCapabilitiesDirectoryOtherTest, touchCalledOnce_multipleParticipantIdsForSingleGbid)
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

TEST_F(LocalCapabilitiesDirectoryOtherTest, touchCalledOnce_singleParticipantIdForMultipleGbids)
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

TEST_F(LocalCapabilitiesDirectoryOtherTest, touchCalledTwice_twoParticipantIdsForDifferentGbids)
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

TEST_F(LocalCapabilitiesDirectoryOtherTest, touchRefreshesAllEntries_GcdTouchOnlyUsesGlobalOnes)
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

TEST_F(LocalCapabilitiesDirectoryOtherTest, testRemoveStaleProvidersOfClusterController)
{
    std::int64_t maxLastSeenMs = 100000;
    for (const auto& gbid : _KNOWN_GBIDS) {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    removeStale(Eq(_clusterControllerId), Eq(maxLastSeenMs), Eq(gbid), _, _))
                .WillOnce(Invoke(
                        this,
                        &LocalCapabilitiesDirectoryOtherTest::fakeCapabilitiesClientRemoveStaleSuccess));
    }

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();
    _localCapabilitiesDirectory->removeStaleProvidersOfClusterController(maxLastSeenMs);
}

TEST_F(LocalCapabilitiesDirectoryOtherTest, testRemoveStaleProvidersOfClusterControllerRetry)
{
    std::int64_t maxLastSeenMs = TimePoint::now().toMilliseconds() - 10000;
    for (const auto& gbid : _KNOWN_GBIDS) {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    removeStale(Eq(_clusterControllerId), Eq(maxLastSeenMs), Eq(gbid), _, _))
                .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryOtherTest::
                                               fakeCapabilitiesClientRemoveStaleWithException))
                .WillOnce(Invoke(
                        this,
                        &LocalCapabilitiesDirectoryOtherTest::fakeCapabilitiesClientRemoveStaleSuccess));
    }

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();
    _localCapabilitiesDirectory->removeStaleProvidersOfClusterController(maxLastSeenMs);
}

TEST_F(LocalCapabilitiesDirectoryOtherTest, testRemoveStaleProvidersOfClusterControllerNoRetry)
{
    std::int64_t maxLastSeenMs = TimePoint::now().toMilliseconds() - 3600100;
    for (const auto& gbid : _KNOWN_GBIDS) {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    removeStale(Eq(_clusterControllerId), Eq(maxLastSeenMs), Eq(gbid), _, _))
                .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryOtherTest::
                                               fakeCapabilitiesClientRemoveStaleWithException));
    }

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();
    _localCapabilitiesDirectory->removeStaleProvidersOfClusterController(maxLastSeenMs);
}

TEST_F(LocalCapabilitiesDirectoryOtherTest, testReAddAllGlobalDiscoveryEntriesPeriodically)
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
