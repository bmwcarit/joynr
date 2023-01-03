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

class LocalCapabilitiesDirectoryAddMethodTest : public AbstractLocalCapabilitiesDirectoryTest
{
public:
    LocalCapabilitiesDirectoryAddMethodTest() : _settings(),
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

    ~LocalCapabilitiesDirectoryAddMethodTest() override
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

    std::function<void()> createVoidOnSuccessFunction()
    {
        return [this]() { _semaphore->notify(); };
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

    std::function<void()> createUnexpectedAddOnSuccessFunction()
    {
        return []() { FAIL(); };
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

    std::function<void(const types::DiscoveryEntryWithMetaInfo&)>
    createLookupParticipantIdSuccessFunction()
    {
        return [this](const types::DiscoveryEntryWithMetaInfo& result) {
            std::ignore = result;
            _semaphore->notify();
        };
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

    void checkAddToGcdClient(const std::vector<std::string>& expectedGbids, Sequence& s)
    {
        EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                    add(GlobalDiscoveryEntryMatcher(_expectedGlobalCapEntry), _, Eq(expectedGbids),
                        _, _, _))
                .Times(1)
                .InSequence(s)
                .WillOnce(InvokeArgument<3>());
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
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectoryAddMethodTest);
};

const std::vector<std::string> AbstractLocalCapabilitiesDirectoryTest::_KNOWN_GBIDS{
        "testGbid1", "testGbid2", "testGbid3"};
const std::string AbstractLocalCapabilitiesDirectoryTest::_INTERFACE_1_NAME("myInterfaceA");
const std::string AbstractLocalCapabilitiesDirectoryTest::_INTERFACE_2_NAME("myInterfaceB");
const std::string AbstractLocalCapabilitiesDirectoryTest::_INTERFACE_3_NAME("myInterfaceC");
const std::string AbstractLocalCapabilitiesDirectoryTest::_DOMAIN_1_NAME("domainA");
const std::string AbstractLocalCapabilitiesDirectoryTest::_DOMAIN_2_NAME("domainB");
const std::string AbstractLocalCapabilitiesDirectoryTest::_DOMAIN_3_NAME("domainB");
const std::string AbstractLocalCapabilitiesDirectoryTest::_LOCAL_ADDRESS(serializer::serializeToJson(
        system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "localTopic")));
const std::vector<std::string> AbstractLocalCapabilitiesDirectoryTest::_EXTERNAL_ADDRESSES_VECTOR{
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[0], "externalTopic")),
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[1], "externalTopic")),
        serializer::serializeToJson(
                system::RoutingTypes::MqttAddress(_KNOWN_GBIDS[2], "externalTopic"))};
const std::int64_t AbstractLocalCapabilitiesDirectoryTest::_LASTSEEN_MS(1000);
const std::int64_t AbstractLocalCapabilitiesDirectoryTest::_EXPIRYDATE_MS(10000);
const std::string AbstractLocalCapabilitiesDirectoryTest::_PUBLIC_KEY_ID("publicKeyId");
const int AbstractLocalCapabilitiesDirectoryTest::_TIMEOUT(2000);

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, add_global_invokesGcd)
{
    Sequence s;

    checkAddToGcdClient(_KNOWN_GBIDS, s);
    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addCapabilityWithSingleNonDefaultGbid)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addUpdatesLastSeenDate)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addWithGbids_global_multipleGbids_invokesGcd)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addWithGbids_global_emptyGbidVector_addsToKnownBackends)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addToAll_global_invokesGcd)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, add_local_doesNotInvokeGcd)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addGlobalEntry_callsMockStorage)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);
    _entry.setQos(providerQos);
    const std::vector<std::string>& expectedGbids = _KNOWN_GBIDS;
    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(
            *_mockLocalCapabilitiesDirectoryStore,
            insertInLocalCapabilitiesStorage(DiscoveryEntryMatcher(_entry), false, expectedGbids))
            .Times(1);
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore, insertInGlobalLookupCache(_, _)).Times(0);
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addLocalEntry_callsMockStorage)
{
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    _entry.setQos(providerQos);
    initializeMockLocalCapabilitiesDirectoryStore();
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore,
                insertInLocalCapabilitiesStorage(DiscoveryEntryMatcher(_entry), false, _KNOWN_GBIDS))
            .Times(1);
    EXPECT_CALL(*_mockLocalCapabilitiesDirectoryStore, insertInGlobalLookupCache(_, _)).Times(0);
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectoryWithMockCapStorage->add(
            _entry, createVoidOnSuccessFunction(), _unexpectedProviderRuntimeExceptionFunction);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addGlobalCapSucceeds_NextAddShallAddGlobalAgain)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddIsProperlyRejected_invalidGbid)
{
    testAddIsProperlyRejected(types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddIsProperlyRejected_unknownGbid)
{
    testAddIsProperlyRejected(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddIsProperlyRejected_internalError)
{
    testAddIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddWithGbids_unknownGbid)
{
    const std::vector<std::string>& gbids{_KNOWN_GBIDS[0], "unknownGbid"};
    testGbidValidationOnAdd(gbids, types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddWithGbids_invalidGbid_emptyGbid)
{
    const std::vector<std::string>& gbids{""};
    testGbidValidationOnAdd(gbids, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddWithGbids_invalidGbid_duplicateGbid)
{
    const std::vector<std::string>& gbids{_KNOWN_GBIDS[1], _KNOWN_GBIDS[1]};
    testGbidValidationOnAdd(gbids, types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addSameGbidTwiceInARow)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addDifferentGbidsAfterEachOther)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddKnownLocalEntryDoesNothing)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddToAll)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddToAllLocal)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddToAllIsProperlyRejected_internalError)
{
    testAddToAllIsProperlyRejected(types::DiscoveryError::INTERNAL_ERROR);
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddToAllIsProperlyRejected_invalidGbid)
{
    testAddToAllIsProperlyRejected(types::DiscoveryError::INVALID_GBID);
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddToAllIsProperlyRejected_unknownGbid)
{
    testAddToAllIsProperlyRejected(types::DiscoveryError::UNKNOWN_GBID);
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, testAddToAllIsProperlyRejected_exception)
{
    const bool awaitGlobalRegistration = true;

    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(Matcher<const types::GlobalDiscoveryEntry&>(DiscoveryEntryMatcher(_entry)), _,
                    Eq(_KNOWN_GBIDS), _, _, _))
            .Times(1)
            .WillOnce(Invoke(
                    this, &LocalCapabilitiesDirectoryAddMethodTest::fakeCapabilitiesClientAddWithException));

    initializeMockLocalCapabilitiesDirectoryStore();
    finalizeTestSetupAfterMockExpectationsAreDone();

    _localCapabilitiesDirectory->addToAll(
            _entry,
            awaitGlobalRegistration,
            createUnexpectedAddOnSuccessFunction(),
            createExpectedDiscoveryErrorFunction(types::DiscoveryError::INTERNAL_ERROR));

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(_TIMEOUT)));
}

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addAddsToCache)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addLocallyDoesNotCallCapabilitiesClient)
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

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, addMultipleTimesSameProviderAwaitForGlobal)
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
                    .WillOnce(DoAll(Invoke(this, &LocalCapabilitiesDirectoryAddMethodTest::
                                                         fakeCapabilitiesClientAddWithException),
                                    ReleaseSemaphore(gcdSemaphore))); // invoke onError
    EXPECT_CALL(*_globalCapabilitiesDirectoryClient,
                add(An<const types::GlobalDiscoveryEntry&>(), _, _, _, _, _))
            .Times(1)
            .After(exp1)
            .WillOnce(DoAll(
                    Invoke(this, &LocalCapabilitiesDirectoryAddMethodTest::fakeCapabilitiesClientAddSuccess),
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

class LocalCapabilitiesDirectoryWithProviderScope
        : public LocalCapabilitiesDirectoryAddMethodTest,
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
                        this, &LocalCapabilitiesDirectoryAddMethodTest::fakeCapabilitiesClientAddSuccess));
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

INSTANTIATE_TEST_SUITE_P(changeProviderScope,
                         LocalCapabilitiesDirectoryWithProviderScope,
                         ::testing::Values(types::ProviderScope::LOCAL,
                                           types::ProviderScope::GLOBAL));

TEST_F(LocalCapabilitiesDirectoryAddMethodTest, registerMultipleGlobalCapabilitiesCheckIfTheyAreMerged)
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
