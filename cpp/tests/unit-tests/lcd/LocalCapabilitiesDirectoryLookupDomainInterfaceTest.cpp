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

class LocalCapabilitiesDirectoryLookupDomainInterfaceTest : public AbstractLocalCapabilitiesDirectoryTest
{
public:
    LocalCapabilitiesDirectoryLookupDomainInterfaceTest()
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

    ~LocalCapabilitiesDirectoryLookupDomainInterfaceTest() override
    {
        _singleThreadedIOService->stop();
        test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectory);
        test::util::resetAndWaitUntilDestroyed(_localCapabilitiesDirectoryWithMockCapStorage);

        test::util::removeFileInCurrentDirectory(".*\\.settings");
        test::util::removeFileInCurrentDirectory(".*\\.persist");
    }
    
    std::function<void(const types::DiscoveryError::Enum&)> createExpectedDiscoveryErrorFunction(
            const types::DiscoveryError::Enum& expectedError)
    {
        return [this, expectedError](const types::DiscoveryError::Enum& error) {
            EXPECT_EQ(expectedError, error);
            _semaphore->notify();
        };
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

    std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>
    createUnexpectedLookupSuccessFunction()
    {
        return [](const std::vector<types::DiscoveryEntryWithMetaInfo>& result) {
            FAIL() << "Got result: " + (result.empty() ? "EMPTY" : result.at(0).toString());
        };
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

private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectoryLookupDomainInterfaceTest);
};

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

    const std::uint8_t expectedReturnedGlobalEntries = 2;
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        _discoveryQos,
                                        multipleGbids,
                                        createLookupSuccessFunction(expectedReturnedGlobalEntries),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
       lookupByDomainInterfaceWithGbidsIsProperlyRejected_invalidGbid)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::Enum::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
       lookupByDomainInterfaceWithGbidsIsProperlyRejected_unknownGbid)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::Enum::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
       lookupByDomainInterfaceWithGbidsIsProperlyRejected_internalError)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
       lookupByDomainInterfaceWithGbidsIsProperlyRejected_noEntryForSelectedBackend)
{
    lookupByDomainInterfaceWithGbidsIsProperlyRejected(
            types::DiscoveryError::NO_ENTRY_FOR_SELECTED_BACKENDS);
}

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, lookupByDomainInterfaceWithGbids_unknownGbids)
{
    const std::vector<std::string>& unknownGbids{"unknownGbid", _KNOWN_GBIDS[0]};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(
            unknownGbids, types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, lookupByDomainInterfaceWithGbids_invalidGbid_emptyGbid)
{
    const std::vector<std::string>& emptyGbid{""};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(
            emptyGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, lookupByDomainInterfaceWithGbids_invalidGbid_duplicateGbid)
{
    const std::vector<std::string>& duplicateGbid{
            _KNOWN_GBIDS[1], _KNOWN_GBIDS[0], _KNOWN_GBIDS[1]};
    testLookupByDomainInterfaceWithGbids_gbidValidationFails(
            duplicateGbid, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, lookupByDomainInterface_globalOnly_emptyCache_invokesGcd)
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, lookupForInterfaceAddressReturnsCachedValues)
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, lookupForInterfaceAddressDelegatesToCapabilitiesClient)
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, registerLocalCapability_lookupLocalAndGlobal)
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

    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntries));
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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
                    Invoke(this, &LocalCapabilitiesDirectoryLookupDomainInterfaceTest::
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
       lookupLocalAndGlobal_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(
                    Invoke(this, &LocalCapabilitiesDirectoryLookupDomainInterfaceTest::
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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
                    Invoke(this, &LocalCapabilitiesDirectoryLookupDomainInterfaceTest::
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, lookupGlobalOnly_GlobalFailsNoLocalEntries_ReturnsNoEntries)
{
    types::DiscoveryQos localDiscoveryQos;
    localDiscoveryQos.setCacheMaxAge(5000);
    localDiscoveryQos.setDiscoveryTimeout(5000);
    localDiscoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillRepeatedly(
                    Invoke(this, &LocalCapabilitiesDirectoryLookupDomainInterfaceTest::
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, lookupGlobalOnly_GlobalFailsLocalEntries_ReturnsNoEntries)
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
                    Invoke(this, &LocalCapabilitiesDirectoryLookupDomainInterfaceTest::
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, lookupMultipeDomainsReturnsResultForMultipleDomains)
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, registerLocalCapability_lookupGlobalOnly)
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
                    InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryLookupDomainInterfaceTest::simulateTimeout));

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
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntries));
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, registerGlobalCapability_lookupLocal)
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
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntries));

    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(1),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);
}

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, registerGlobalCapability_lookupLocalThenGlobal)
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
            .WillOnce(InvokeArgument<2>(std::vector<std::string>{}));
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryLookupDomainInterfaceTest::simulateTimeout));

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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, registerGlobalCapability_lookupGlobalOnly)
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
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient, lookup(_, _, _, _, _, _, _))
            .Times(1)
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
    _localCapabilitiesDirectory->registerReceivedCapabilities(std::move(_globalCapEntries));
    _localCapabilitiesDirectory->lookup({_DOMAIN_1_NAME},
                                        _INTERFACE_1_NAME,
                                        localDiscoveryQos,
                                        _KNOWN_GBIDS,
                                        createLookupSuccessFunction(2),
                                        _unexpectedOnDiscoveryErrorFunction);
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));

    _localCapabilitiesDirectory->remove(_dummyParticipantIdsVector[0], nullptr, nullptr);
}

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, registerLocalCapability_lookupLocalThenGlobal)
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
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryLookupDomainInterfaceTest::simulateTimeout));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, localAndGlobalDoesNotReturnDuplicateEntriesCacheEnabled)
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest, localAndGlobalDoesNotReturnDuplicateEntriesCacheDisabled)
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

TEST_F(LocalCapabilitiesDirectoryLookupDomainInterfaceTest,
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
