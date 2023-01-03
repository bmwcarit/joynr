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

class LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest : public AbstractLocalCapabilitiesDirectoryTest
{
public:
    LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest()
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

    ~LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest() override
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

    std::function<void(const types::DiscoveryEntryWithMetaInfo&)>
    createUnexpectedLookupParticipantIdSuccessFunction()
    {
        return [](const types::DiscoveryEntryWithMetaInfo& result) {
            FAIL() << "Got result: " + result.toString();
        };
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
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest);
};

const std::vector<std::string> LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_KNOWN_GBIDS{
        "testGbid1", "testGbid2", "testGbid3"};
const std::string LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_INTERFACE_1_NAME("myInterfaceA");
const std::string LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_INTERFACE_2_NAME("myInterfaceB");
const std::string LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_INTERFACE_3_NAME("myInterfaceC");
const std::string LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_DOMAIN_1_NAME("domainA");
const std::string LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_DOMAIN_2_NAME("domainB");
const std::string LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_DOMAIN_3_NAME("domainB");
const std::string LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_LOCAL_ADDRESS(serializer::serializeToJson(
        system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "localTopic")));
const std::vector<std::string> LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_EXTERNAL_ADDRESSES_VECTOR{
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "externalTopic")),
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[1], "externalTopic")),
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[2], "externalTopic"))};
const std::int64_t LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_LASTSEEN_MS(1000);
const std::int64_t LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_EXPIRYDATE_MS(10000);
const std::string LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_PUBLIC_KEY_ID("publicKeyId");
const int LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::_TIMEOUT(2000);

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest, lookupLocalEntryByParticipantId_callsMockStorage)
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

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest,
       lookupByParticipantIdWithGbids_globalOnly_multipleGbids_allCached_1)
{
    std::vector<std::string> gbids{_KNOWN_GBIDS[0], _KNOWN_GBIDS[2]};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbids);
}

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest,
       lookupByParticipantIdWithGbids_globalOnly_multipleGbids_allCached_2)
{
    std::vector<std::string> gbids{_KNOWN_GBIDS[1], _KNOWN_GBIDS[2]};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbids);
}

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest,
       lookupByParticipantIdWithGbids_globalOnly_multipleGbids_noneCached_1)
{
    std::vector<std::string> gbidsForLookup{_KNOWN_GBIDS[0], _KNOWN_GBIDS[2]};
    std::vector<std::string> expectedGbids{gbidsForLookup};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(
            gbidsForLookup, expectedGbids);
}

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest,
       lookupByParticipantIdWithGbids_globalOnly_multipleGbids_noneCached_2)
{
    std::vector<std::string> gbidsForLookup{_KNOWN_GBIDS[1], _KNOWN_GBIDS[2]};
    std::vector<std::string> expectedGbids{gbidsForLookup};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(
            gbidsForLookup, expectedGbids);
}

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest,
       lookupByParticipantIdWithGbids_globalOnly_emptyGbidsVector_allCached)
{
    std::vector<std::string> gbidsForLookup{};
    testLookupByParticipantIdWithGbids_globalOnly_cached_doesNotInvokeGcdClient(gbidsForLookup);
}

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest,
       lookupByParticipantIdWithGbids_globalOnly_emptyGbidsVector_noneCached)
{
    std::vector<std::string> gbidsForLookup{};
    testLookupByParticipantIdWithGbids_globalOnly_notCached_invokesGCDClient(
            gbidsForLookup, _KNOWN_GBIDS);
}

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest, lookupByParticipantIdWithGbids_unknownGbids)
{
    const std::vector<std::string>& unknownGbids{"unknownGbid", _KNOWN_GBIDS[0]};
    testLookupByParticipantIdWithGbids_gbidValidationFails(
            unknownGbids, types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest, lookupByParticipantIdWithGbids_invalidGbid_emptyGbid)
{
    const std::vector<std::string>& emptyGbid{""};
    testLookupByParticipantIdWithGbids_gbidValidationFails(
            emptyGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest, lookupByParticipantIdWithGbids_invalidGbid_duplicateGbid)
{
    const std::vector<std::string>& duplicateGbid{
            _KNOWN_GBIDS[1], _KNOWN_GBIDS[0], _KNOWN_GBIDS[1]};

    initializeMockLocalCapabilitiesDirectoryStore();
    testLookupByParticipantIdWithGbids_gbidValidationFails(
            duplicateGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest, lookupForParticipantIdReturnsCachedValues)
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
                    this, &LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::fakeLookupByParticipantIdWithResult))
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

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest, lookupForParticipantIdReturnsNoCapability)
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
                    Invoke(this, &LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::fakeLookupNoEntryForParticipant));

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

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest, lookupForParticipantIdDelegatesToCapabilitiesClient)
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
                    this, &LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest::fakeLookupByParticipantIdWithResult));

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

TEST_F(LocalCapabilitiesDirectoryLookupParticipantIdMethodsTest,
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