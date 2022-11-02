/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include <climits>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/CapabilitiesStorage.h"
#include "joynr/InterfaceAddress.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/LocalCapabilitiesDirectoryStore.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryScope.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/Version.h"

using namespace ::testing;
using namespace joynr;

class LocalCapabilitiesDirectoryStoreTest : public ::testing::Test
{
public:
    LocalCapabilitiesDirectoryStoreTest()
            : _defaultProviderVersion(30, 06),
              _domainName("testdomain"),
              _domainNameGlobal("testdomain2"),
              _interfaceName("testinterface"),
              _interfaceNameGlobal("testinterface2"),
              _participantId("testparticipantid"),
              _participantIdGlobal("testparticipantid2"),
              _publicKeyId("testkey"),
              _qos(),
              _localEntry(_defaultProviderVersion,
                          _domainName,
                          _interfaceName,
                          _participantId,
                          _qos,
                          10000,
                          10000,
                          _publicKeyId),
              _globalEntry(_defaultProviderVersion,
                           _domainNameGlobal,
                           _interfaceNameGlobal,
                           _participantIdGlobal,
                           _qos,
                           10000,
                           10000,
                           _publicKeyId),
              _semaphore(std::make_shared<Semaphore>(0))
    {
        _qos.setScope(types::ProviderScope::LOCAL);
        _localEntry.setQos(_qos);
        _qos.setScope(types::ProviderScope::GLOBAL);
        _globalEntry.setQos(_qos);
    }

protected:
    LocalCapabilitiesDirectoryStore _localCapabilitiesDirectoryStore;
    types::Version _defaultProviderVersion;
    std::string _domainName;
    std::string _domainNameGlobal;
    std::string _interfaceName;
    std::string _interfaceNameGlobal;
    std::string _participantId;
    std::string _participantIdGlobal;
    std::string _publicKeyId;
    types::ProviderQos _qos;
    types::DiscoveryEntry _localEntry;
    types::DiscoveryEntry _globalEntry;
    std::shared_ptr<Semaphore> _semaphore;

    void test_getLocalAndCachedCapabilities_interfaceAddresses(
            types::DiscoveryQos discoveryQos,
            std::shared_ptr<ILocalCapabilitiesCallback> callback)
    {
        InterfaceAddress interfaceAddress(_localEntry.getDomain(), _localEntry.getInterfaceName());
        InterfaceAddress interfaceAddressGlobal(
                _globalEntry.getDomain(), _globalEntry.getInterfaceName());
        std::vector<InterfaceAddress> interfaceAddresses;
        interfaceAddresses.push_back(interfaceAddress);
        interfaceAddresses.push_back(interfaceAddressGlobal);

        std::vector<std::string> gbids = {"gbid1", "gbid2"};

        _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(_localEntry);
        _localCapabilitiesDirectoryStore.insertInGlobalLookupCache(_globalEntry, gbids);

        ASSERT_TRUE(_localCapabilitiesDirectoryStore.getLocalAndCachedCapabilities(
                interfaceAddresses, discoveryQos, gbids, callback));
    }
};

TEST_F(LocalCapabilitiesDirectoryStoreTest, getGlobalLookupCache)
{
    std::unique_lock<std::recursive_mutex> cacheLock(
            _localCapabilitiesDirectoryStore.getCacheLock());
    ASSERT_NE(nullptr, _localCapabilitiesDirectoryStore.getGlobalLookupCache(cacheLock));
}

TEST_F(LocalCapabilitiesDirectoryStoreTest, getLocallyRegisteredCapabilities)
{
    std::unique_lock<std::recursive_mutex> cacheLock(
            _localCapabilitiesDirectoryStore.getCacheLock());
    ASSERT_NE(
            nullptr, _localCapabilitiesDirectoryStore.getLocallyRegisteredCapabilities(cacheLock));
}

TEST_F(LocalCapabilitiesDirectoryStoreTest, insertInLocalCapabilitiesStorage)
{
    std::unique_lock<std::recursive_mutex> cacheLock(
            _localCapabilitiesDirectoryStore.getCacheLock());
    ASSERT_EQ(0,
              _localCapabilitiesDirectoryStore.getLocallyRegisteredCapabilities(cacheLock)->size());
    ASSERT_EQ(0, _localCapabilitiesDirectoryStore.getGlobalLookupCache(cacheLock)->size());

    _qos.setScope(types::ProviderScope::GLOBAL);
    _localEntry.setQos(_qos);
    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(_localEntry);

    ASSERT_EQ(0, _localCapabilitiesDirectoryStore.getGlobalLookupCache(cacheLock)->size());
    ASSERT_EQ(1, _localCapabilitiesDirectoryStore.getLocalCapabilities(_participantId).size());
    ASSERT_EQ(1,
              _localCapabilitiesDirectoryStore.getLocallyRegisteredCapabilities(cacheLock)->size());
}

TEST_F(LocalCapabilitiesDirectoryStoreTest, insertInGlobalLookupCache)
{
    std::vector<std::string> gbids = {"gbid1", "gbid2"};
    std::unique_lock<std::recursive_mutex> cacheLock(
            _localCapabilitiesDirectoryStore.getCacheLock());
    ASSERT_EQ(0, _localCapabilitiesDirectoryStore.getGlobalLookupCache(cacheLock)->size());
    _localCapabilitiesDirectoryStore.insertInGlobalLookupCache(_localEntry, gbids);
    ASSERT_EQ(1, _localCapabilitiesDirectoryStore.getGlobalLookupCache(cacheLock)->size());
    ASSERT_EQ(1, _localCapabilitiesDirectoryStore.getCachedGlobalDiscoveryEntries().size());
    ASSERT_EQ(gbids,
              _localCapabilitiesDirectoryStore.getGbidsForParticipantId(_participantId, cacheLock));
}

TEST_F(LocalCapabilitiesDirectoryStoreTest, updateEntryInGlobalLookupCacheIfItExists)
{
    const std::vector<std::string> gbids1 = {"gbid1", "gbid2"};
    const std::vector<std::string> expectedGbids1 = {gbids1};
    const std::vector<types::DiscoveryEntry> expectedDiscoveryEntries1 = {_localEntry};
    std::unique_lock<std::recursive_mutex> cacheLock(
            _localCapabilitiesDirectoryStore.getCacheLock());
    ASSERT_EQ(0, _localCapabilitiesDirectoryStore.getGlobalLookupCache(cacheLock)->size());

    _localCapabilitiesDirectoryStore.insertInGlobalLookupCache(_localEntry, gbids1);

    ASSERT_EQ(1, _localCapabilitiesDirectoryStore.getGlobalLookupCache(cacheLock)->size());
    ASSERT_EQ(expectedGbids1,
              _localCapabilitiesDirectoryStore.getGbidsForParticipantId(_participantId, cacheLock));
    cacheLock.unlock();
    ASSERT_EQ(expectedDiscoveryEntries1,
              _localCapabilitiesDirectoryStore.getCachedGlobalDiscoveryEntries());

    // change the entry and re-add it again. It expected to be updated
    _localEntry.setDomain("anotherDomain");

    const std::vector<std::string> gbids2 = {"gbid2", "gbid3"};
    const std::vector<std::string> expectedGbids2 = {"gbid2", "gbid3", "gbid1"};
    const std::vector<types::DiscoveryEntry> expectedDiscoveryEntries2 = {_localEntry};

    _localCapabilitiesDirectoryStore.insertInGlobalLookupCache(_localEntry, gbids2);

    cacheLock.lock();
    ASSERT_EQ(1, _localCapabilitiesDirectoryStore.getGlobalLookupCache(cacheLock)->size());
    ASSERT_EQ(1, _localCapabilitiesDirectoryStore.getCachedGlobalDiscoveryEntries().size());
    ASSERT_EQ(expectedGbids2,
              _localCapabilitiesDirectoryStore.getGbidsForParticipantId(_participantId, cacheLock));
    cacheLock.unlock();

    // check that the entry is updated
    ASSERT_EQ(expectedDiscoveryEntries2,
              _localCapabilitiesDirectoryStore.getCachedGlobalDiscoveryEntries());
    ASSERT_EQ(expectedDiscoveryEntries2[0].getDomain(),
              _localCapabilitiesDirectoryStore.getCachedGlobalDiscoveryEntries()[0].getDomain());
}

TEST_F(LocalCapabilitiesDirectoryStoreTest, getAllGlobalCapabilities)
{
    const std::string participantId1 = "participantId1";
    const std::string participantId2 = "participantId2";
    types::DiscoveryEntry globalDiscoveryEntry1 = _globalEntry;

    globalDiscoveryEntry1.setParticipantId(participantId1);

    types::DiscoveryEntry globalDiscoveryEntry2 = globalDiscoveryEntry1;
    globalDiscoveryEntry2.setParticipantId(participantId2);

    std::vector<std::string> gbids{"gbid1", "gbid2"};
    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(_localEntry);
    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(globalDiscoveryEntry1, gbids);
    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(globalDiscoveryEntry2, gbids);

    std::vector<types::DiscoveryEntry> globalDiscoveryEntries =
            _localCapabilitiesDirectoryStore.getAllGlobalCapabilities();

    ASSERT_EQ(2, globalDiscoveryEntries.size());

    bool foundEntry1;
    bool foundEntry2;
    for (types::DiscoveryEntry entry : globalDiscoveryEntries) {
        if (entry.getParticipantId() == participantId1) {
            foundEntry1 = true;
        }
        if (entry.getParticipantId() == participantId2) {
            foundEntry2 = true;
        }
    }
    EXPECT_TRUE(foundEntry1);
    EXPECT_TRUE(foundEntry2);
}

TEST_F(LocalCapabilitiesDirectoryStoreTest, clear)
{
    std::unique_lock<std::recursive_mutex> cacheLock(
            _localCapabilitiesDirectoryStore.getCacheLock());
    ASSERT_EQ(0,
              _localCapabilitiesDirectoryStore.getLocallyRegisteredCapabilities(cacheLock)->size());
    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(_localEntry);
    ASSERT_EQ(1,
              _localCapabilitiesDirectoryStore.getLocallyRegisteredCapabilities(cacheLock)->size());
    _localCapabilitiesDirectoryStore.clear();
    ASSERT_EQ(0,
              _localCapabilitiesDirectoryStore.getLocallyRegisteredCapabilities(cacheLock)->size());
}

TEST_F(LocalCapabilitiesDirectoryStoreTest, countGlobalCapabilities)
{
    std::vector<std::string> gbids{"gbid1", "gbid2"};
    ASSERT_EQ(0, _localCapabilitiesDirectoryStore.countGlobalCapabilities());
    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(_localEntry);
    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(_globalEntry, gbids);
    ASSERT_EQ(1, _localCapabilitiesDirectoryStore.countGlobalCapabilities());
}

// It's basically the same method....
TEST_F(LocalCapabilitiesDirectoryStoreTest, searchLocalAndGetLocalCapabilities)
{
    InterfaceAddress interfaceAddress(_localEntry.getDomain(), _localEntry.getInterfaceName());
    std::vector<InterfaceAddress> interfaceAddresses;
    interfaceAddresses.push_back(interfaceAddress);

    ASSERT_EQ(0, _localCapabilitiesDirectoryStore.searchLocal(interfaceAddresses).size());
    ASSERT_EQ(0, _localCapabilitiesDirectoryStore.getLocalCapabilities(interfaceAddresses).size());

    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(_localEntry);

    ASSERT_EQ(1, _localCapabilitiesDirectoryStore.searchLocal(interfaceAddresses).size());
    ASSERT_EQ(1, _localCapabilitiesDirectoryStore.getLocalCapabilities(interfaceAddresses).size());
}

TEST_F(LocalCapabilitiesDirectoryStoreTest, handlingOfGbidMappings)
{
    std::vector<std::string> gbids = {"gbid1", "gbid2"};

    std::unique_lock<std::recursive_mutex> cacheLock(
            _localCapabilitiesDirectoryStore.getCacheLock());
    ASSERT_EQ(0,
              _localCapabilitiesDirectoryStore
                      .getGbidsForParticipantId(_participantIdGlobal, cacheLock)
                      .size());
    cacheLock.unlock();

    _localCapabilitiesDirectoryStore.insertInGlobalLookupCache(_globalEntry, gbids);

    cacheLock.lock();
    ASSERT_EQ(gbids,
              _localCapabilitiesDirectoryStore.getGbidsForParticipantId(
                      _participantIdGlobal, cacheLock));

    _localCapabilitiesDirectoryStore.eraseParticipantIdToGbidMapping(
            _participantIdGlobal, cacheLock);

    ASSERT_EQ(0,
              _localCapabilitiesDirectoryStore
                      .getGbidsForParticipantId(_participantIdGlobal, cacheLock)
                      .size());
}

TEST_F(LocalCapabilitiesDirectoryStoreTest,
       getLocalAndCachedCapabilities_interfaceAddresses_local_only)
{
    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)> onSuccess =
            [this](const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result) {
                ASSERT_EQ(1, result.size());
                ASSERT_EQ(_localEntry.getParticipantId(), result.at(0).getParticipantId());
                _semaphore->notify();
            };
    std::function<void(const types::DiscoveryError::Enum&)> onError =
            [](const types::DiscoveryError::Enum& errorEnum) {
                FAIL() << "Unexpected onError call: " +
                                  types::DiscoveryError::getLiteral(errorEnum);
            };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(LONG_MAX);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);

    test_getLocalAndCachedCapabilities_interfaceAddresses(discoveryQos, localCapabilitiesCallback);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryStoreTest,
       getLocalAndCachedCapabilities_interfaceAddresses_local_and_global)
{
    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)> onSuccess =
            [this](const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result) {
                ASSERT_EQ(2, result.size());
                bool firstNotified = false;
                bool secondNotified = false;
                for (auto entry : result) {
                    if (_localEntry.getParticipantId() == entry.getParticipantId() &&
                        !firstNotified) {
                        _semaphore->notify();
                        firstNotified = true;
                    }
                    if (_globalEntry.getParticipantId() == entry.getParticipantId() &&
                        !secondNotified) {
                        _semaphore->notify();
                        secondNotified = true;
                    }
                }
            };
    std::function<void(const types::DiscoveryError::Enum&)> onError =
            [](const types::DiscoveryError::Enum& errorEnum) {
                FAIL() << "Unexpected onError call: " +
                                  types::DiscoveryError::getLiteral(errorEnum);
            };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(LONG_MAX);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    test_getLocalAndCachedCapabilities_interfaceAddresses(discoveryQos, localCapabilitiesCallback);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(1000)));
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryStoreTest,
       getLocalAndCachedCapabilities_interfaceAddresses_local_then_global)
{
    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)> onSuccess =
            [this](const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result) {
                ASSERT_EQ(1, result.size());
                ASSERT_EQ(_localEntry.getParticipantId(), result.at(0).getParticipantId());
                _semaphore->notify();
            };
    std::function<void(const types::DiscoveryError::Enum&)> onError =
            [](const types::DiscoveryError::Enum& errorEnum) {
                FAIL() << "Unexpected onError call: " +
                                  types::DiscoveryError::getLiteral(errorEnum);
            };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(LONG_MAX);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    test_getLocalAndCachedCapabilities_interfaceAddresses(discoveryQos, localCapabilitiesCallback);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryStoreTest,
       getLocalAndCachedCapabilities_interfaceAddresses_global_only)
{
    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)> onSuccess =
            [this](const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result) {
                ASSERT_EQ(1, result.size());
                ASSERT_EQ(_globalEntry.getParticipantId(), result.at(0).getParticipantId());
                _semaphore->notify();
            };
    std::function<void(const types::DiscoveryError::Enum&)> onError =
            [](const types::DiscoveryError::Enum& errorEnum) {
                FAIL() << "Unexpected onError call: " +
                                  types::DiscoveryError::getLiteral(errorEnum);
            };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(LONG_MAX);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    test_getLocalAndCachedCapabilities_interfaceAddresses(discoveryQos, localCapabilitiesCallback);

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryStoreTest, getLocalAndCachedCapabilities_participantId_local_only)
{
    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)> onSuccess =
            [this](const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result) {
                ASSERT_EQ(1, result.size());
                ASSERT_EQ(_localEntry.getParticipantId(), result.at(0).getParticipantId());
                _semaphore->notify();
            };
    std::function<void(const types::DiscoveryError::Enum&)> onError =
            [](const types::DiscoveryError::Enum& errorEnum) {
                FAIL() << "Unexpected onError call: " +
                                  types::DiscoveryError::getLiteral(errorEnum);
            };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(LONG_MAX);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);

    std::vector<std::string> gbids = {"gbid1", "gbid2"};

    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(_localEntry);
    _localCapabilitiesDirectoryStore.insertInGlobalLookupCache(_globalEntry, gbids);

    ASSERT_TRUE(_localCapabilitiesDirectoryStore.getLocalAndCachedCapabilities(
            _localEntry.getParticipantId(), discoveryQos, gbids, localCapabilitiesCallback));

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryStoreTest,
       getLocalAndCachedCapabilities_participantId_local_and_global)
{
    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)> onSuccess =
            [](const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&) {
                FAIL() << "Unexpected onSuccess call" ;
            };
    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)>
            onSuccessGlobal =
                    [this](const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result) {
                        ASSERT_EQ(1, result.size());
                        ASSERT_EQ(_globalEntry.getParticipantId(), result.at(0).getParticipantId());
                        _semaphore->notify();
                    };
    std::function<void(const types::DiscoveryError::Enum&)> onError1 =
            [](const types::DiscoveryError::Enum& errorEnum) {
                FAIL() << "Unexpected onError call: " +
                                  types::DiscoveryError::getLiteral(errorEnum);
            };

    std::function<void(const types::DiscoveryError::Enum&)> onError2 =
            [](const types::DiscoveryError::Enum& errorEnum) {
                FAIL() << "Unexpected onError call: " +
                                  types::DiscoveryError::getLiteral(errorEnum);
            };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError1));
    auto globalCapabilitiesCallback = std::make_shared<LocalCapabilitiesCallback>(
            std::move(onSuccessGlobal), std::move(onError2));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(LONG_MAX);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_AND_GLOBAL);

    std::vector<std::string> gbids = {"gbid1", "gbid2"};

    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(_localEntry);
    _localCapabilitiesDirectoryStore.insertInGlobalLookupCache(_globalEntry, gbids);

    ASSERT_FALSE(_localCapabilitiesDirectoryStore.getLocalAndCachedCapabilities(
            _localEntry.getParticipantId(), discoveryQos, gbids, localCapabilitiesCallback));
    ASSERT_TRUE(_localCapabilitiesDirectoryStore.getLocalAndCachedCapabilities(
            _globalEntry.getParticipantId(), discoveryQos, gbids, globalCapabilitiesCallback));

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryStoreTest,
       getLocalAndCachedCapabilities_participantId_local_then_global)
{
    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)> onSuccess =
            [this](const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result) {
                ASSERT_EQ(1, result.size());
                ASSERT_EQ(_localEntry.getParticipantId(), result.at(0).getParticipantId());
                _semaphore->notify();
            };
    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)>
            onSuccessGlobal =
                    [this](const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result) {
                        ASSERT_EQ(1, result.size());
                        ASSERT_EQ(_globalEntry.getParticipantId(), result.at(0).getParticipantId());
                        _semaphore->notify();
                    };
    std::function<void(const types::DiscoveryError::Enum&)> onError =
            [](const types::DiscoveryError::Enum& errorEnum) {
                FAIL() << "Unexpected onError call: " +
                                  types::DiscoveryError::getLiteral(errorEnum);
            };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));
    auto globalCapabilitiesCallback = std::make_shared<LocalCapabilitiesCallback>(
            std::move(onSuccessGlobal), std::move(onError));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(LONG_MAX);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    std::vector<std::string> gbids = {"gbid1", "gbid2"};

    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(_localEntry);
    _localCapabilitiesDirectoryStore.insertInGlobalLookupCache(_globalEntry, gbids);

    ASSERT_TRUE(_localCapabilitiesDirectoryStore.getLocalAndCachedCapabilities(
            _localEntry.getParticipantId(), discoveryQos, gbids, localCapabilitiesCallback));
    ASSERT_TRUE(_localCapabilitiesDirectoryStore.getLocalAndCachedCapabilities(
            _globalEntry.getParticipantId(), discoveryQos, gbids, globalCapabilitiesCallback));

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(1000)));
    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(LocalCapabilitiesDirectoryStoreTest, getLocalAndCachedCapabilities_participantId_global_only)
{
    std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>&)> onSuccess =
            [this](const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result) {
                ASSERT_EQ(1, result.size());
                ASSERT_EQ(_globalEntry.getParticipantId(), result.at(0).getParticipantId());
                _semaphore->notify();
            };
    std::function<void(const types::DiscoveryError::Enum&)> onError =
            [](const types::DiscoveryError::Enum& errorEnum) {
                FAIL() << "Unexpected onError call: " +
                                  types::DiscoveryError::getLiteral(errorEnum);
            };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));

    types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(LONG_MAX);
    discoveryQos.setDiscoveryScope(types::DiscoveryScope::GLOBAL_ONLY);

    std::vector<std::string> gbids = {"gbid1", "gbid2"};

    _localCapabilitiesDirectoryStore.insertInLocalCapabilitiesStorage(_localEntry);
    _localCapabilitiesDirectoryStore.insertInGlobalLookupCache(_globalEntry, gbids);

    ASSERT_TRUE(_localCapabilitiesDirectoryStore.getLocalAndCachedCapabilities(
            _globalEntry.getParticipantId(), discoveryQos, gbids, localCapabilitiesCallback));
    ASSERT_FALSE(_localCapabilitiesDirectoryStore.getLocalAndCachedCapabilities(
            _localEntry.getParticipantId(), discoveryQos, gbids, localCapabilitiesCallback));

    EXPECT_TRUE(_semaphore->waitFor(std::chrono::milliseconds(1000)));
}
