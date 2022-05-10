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

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <boost/optional/optional_io.hpp>
#include "tests/utils/Gtest.h"

#include "joynr/LocalDiscoveryAggregator.h"

#include "joynr/Future.h"
#include "joynr/Semaphore.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/DiscoveryQos.h"

#include "tests/mock/MockDiscovery.h"

using ::testing::DoAll;

using namespace ::testing;
using namespace joynr;

class LocalDiscoveryAggregatorTest : public ::testing::Test
{
public:
    LocalDiscoveryAggregatorTest()
            : _provisionedDiscoveryEntries(),
              _localDiscoveryAggregator(_provisionedDiscoveryEntries),
              _discoveryMock(std::make_shared<MockDiscovery>()),
              _semaphore(0)
    {
        _discoveryProxyNotSetOnError = [this] (const exceptions::JoynrRuntimeException& error) {
            EXPECT_EQ("internal discoveryProxy not set", error.getMessage());
            _semaphore.notify();
        };
    }

    ~LocalDiscoveryAggregatorTest() override
    {
    }

protected:
    std::map<std::string, types::DiscoveryEntryWithMetaInfo> _provisionedDiscoveryEntries;
    LocalDiscoveryAggregator _localDiscoveryAggregator;
    std::shared_ptr<MockDiscovery> _discoveryMock;
    Semaphore _semaphore;
    std::function<void(const exceptions::JoynrRuntimeException&)> _discoveryProxyNotSetOnError;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalDiscoveryAggregatorTest);
};

TEST_F(LocalDiscoveryAggregatorTest, addAsync_proxyNotSet_reportsErrorViaCallbackAndFuture)
{
    const types::DiscoveryEntryWithMetaInfo discoveryEntry;
    auto future = _localDiscoveryAggregator.addAsync(discoveryEntry, false, nullptr, _discoveryProxyNotSetOnError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(100)));
    try {
        future->get(100);
        FAIL() << "unexpected success";
    } catch (const exceptions::JoynrRuntimeException& error) {
        EXPECT_EQ("internal discoveryProxy not set", error.getMessage());
    }
}

TEST_F(LocalDiscoveryAggregatorTest, addAsync_withGbids_proxyNotSet_reportsErrorViaCallbackAndFuture)
{
    const types::DiscoveryEntryWithMetaInfo discoveryEntry;
    const std::vector<std::string> gbids = { "joynrdefaultgbid", "othergbid" };
    auto future = _localDiscoveryAggregator.addAsync(discoveryEntry, false, gbids, nullptr, nullptr, _discoveryProxyNotSetOnError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(100)));
    try {
        future->get(100);
        FAIL() << "unexpected success";
    } catch (const exceptions::JoynrRuntimeException& error) {
        EXPECT_EQ("internal discoveryProxy not set", error.getMessage());
    }
}

TEST_F(LocalDiscoveryAggregatorTest, addToAllAsync_proxyNotSet_reportsErrorViaCallbackAndFuture)
{
    const types::DiscoveryEntryWithMetaInfo discoveryEntry;
    auto future = _localDiscoveryAggregator.addToAllAsync(discoveryEntry, false, nullptr, nullptr, _discoveryProxyNotSetOnError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(100)));
    try {
        future->get(100);
        FAIL() << "unexpected success";
    } catch (const exceptions::JoynrRuntimeException& error) {
        EXPECT_EQ("internal discoveryProxy not set", error.getMessage());
    }
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncDomainInterface_proxyNotSet_reportsErrorViaCallbackAndFuture)
{
    const std::vector<std::string> domains;
    const std::string interfaceName;
    const types::DiscoveryQos discoveryQos;
    auto future = _localDiscoveryAggregator.lookupAsync(
                         domains, interfaceName, discoveryQos, nullptr, _discoveryProxyNotSetOnError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(100)));
    try {
        std::vector<types::DiscoveryEntryWithMetaInfo> result;
        future->get(100, result);
        FAIL() << "unexpected success";
    } catch (const exceptions::JoynrRuntimeException& error) {
        EXPECT_EQ("internal discoveryProxy not set", error.getMessage());
    }
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncDomainInterface_withGbids_proxyNotSet_reportsErrorViaCallbackAndFuture)
{
    const std::vector<std::string> domains;
    const std::string interfaceName;
    const types::DiscoveryQos discoveryQos;
    const std::vector<std::string> gbids = { "joynrdefaultgbid", "othergbid" };
    auto future = _localDiscoveryAggregator.lookupAsync(
                         domains, interfaceName, discoveryQos, gbids, nullptr, nullptr, _discoveryProxyNotSetOnError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(100)));
    try {
        std::vector<types::DiscoveryEntryWithMetaInfo> result;
        future->get(100, result);
        FAIL() << "unexpected success";
    } catch (const exceptions::JoynrRuntimeException& error) {
        EXPECT_EQ("internal discoveryProxy not set", error.getMessage());
    }
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncParticipantId_proxyNotSet_reportsErrorViaCallbackAndFuture)
{
    const std::string participantId;
    auto future = _localDiscoveryAggregator.lookupAsync(participantId, nullptr, _discoveryProxyNotSetOnError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(100)));
    try {
        types::DiscoveryEntryWithMetaInfo result;
        future->get(100, result);
        FAIL() << "unexpected success";
    } catch (const exceptions::JoynrRuntimeException& error) {
        EXPECT_EQ("internal discoveryProxy not set", error.getMessage());
    }
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncParticipantId_withGbids_proxyNotSet_reportsErrorViaCallbackAndFuture)
{
    const std::string participantId;
    const types::DiscoveryQos discoveryQos;
    const std::vector<std::string> gbids = { "joynrdefaultgbid", "othergbid" };
    auto future = _localDiscoveryAggregator.lookupAsync(
                participantId, discoveryQos, gbids, nullptr, nullptr, _discoveryProxyNotSetOnError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(100)));
    try {
        types::DiscoveryEntryWithMetaInfo result;
        future->get(100, result);
        FAIL() << "unexpected success";
    } catch (const exceptions::JoynrRuntimeException& error) {
        EXPECT_EQ("internal discoveryProxy not set", error.getMessage());
    }
}

TEST_F(LocalDiscoveryAggregatorTest, removeAsync_proxyNotSet_reportsErrorViaCallbackAndFuture)
{
    const std::string participantId;
    auto future = _localDiscoveryAggregator.removeAsync(participantId, nullptr, _discoveryProxyNotSetOnError);
    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(100)));
    try {
        future->get(100);
        FAIL() << "unexpected success";
    } catch (const exceptions::JoynrRuntimeException& error) {
        EXPECT_EQ("internal discoveryProxy not set", error.getMessage());
    }
}

TEST_F(LocalDiscoveryAggregatorTest, addAsync_withoutAwaitGlobalRegistration_callsProxy)
{
    _localDiscoveryAggregator.setDiscoveryProxy(_discoveryMock);

    types::DiscoveryEntryWithMetaInfo discoveryEntry;
    discoveryEntry.setParticipantId("testParticipantId");
    const joynr::MessagingQos messagingQos(2016);
    EXPECT_CALL(*_discoveryMock, addAsyncMock(Eq(discoveryEntry), Eq(nullptr), Eq(nullptr), Eq(messagingQos)));

    _localDiscoveryAggregator.addAsync(discoveryEntry, nullptr, nullptr, messagingQos);
}

TEST_F(LocalDiscoveryAggregatorTest, addAsync_withAwaitGlobalRegistration_callsProxy)
{
    _localDiscoveryAggregator.setDiscoveryProxy(_discoveryMock);

    types::DiscoveryEntryWithMetaInfo discoveryEntry;
    discoveryEntry.setParticipantId("testParticipantId");
    const bool awaitGlobalRegistration = true;
    const joynr::MessagingQos messagingQos(2205);
    EXPECT_CALL(*_discoveryMock, addAsyncMock(Eq(discoveryEntry), Eq(awaitGlobalRegistration), Eq(nullptr), Eq(nullptr), Eq(messagingQos)));

    _localDiscoveryAggregator.addAsync(discoveryEntry, awaitGlobalRegistration, nullptr, nullptr, messagingQos);
}

TEST_F(LocalDiscoveryAggregatorTest, addAsync_withGbids_callsProxy)
{
    _localDiscoveryAggregator.setDiscoveryProxy(_discoveryMock);

    types::DiscoveryEntryWithMetaInfo discoveryEntry;
    discoveryEntry.setParticipantId("testParticipantId");
    const bool awaitGlobalRegistration = true;
    const std::vector<std::string> gbids = { "joynrdefaultgbid", "othergbid" };
    std::vector<std::string> capturedGbids;
    const joynr::MessagingQos messagingQos(2070);

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onRuntimeError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };
    auto onApplicationError = [&future](const joynr::types::DiscoveryError::Enum& errorEnum) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(joynr::types::DiscoveryError::getLiteral(errorEnum)));
    };
    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();

    EXPECT_CALL(*_discoveryMock, addAsyncMock(Eq(discoveryEntry),
                                             Eq(awaitGlobalRegistration),
                                             _, // gbids
                                             _, // onSuccess
                                             _, // onApplicationError
                                             _, // onRuntimeError
                                             Eq(messagingQos)))
            .WillOnce(DoAll(::testing::SaveArg<2>(&capturedGbids), InvokeArgument<3>(), Return(mockFuture)));
    _localDiscoveryAggregator.addAsync(discoveryEntry,
                                      awaitGlobalRegistration,
                                      gbids,
                                      onSuccess,
                                      onApplicationError,
                                      onRuntimeError,
                                      messagingQos);
    future.get();
    EXPECT_EQ(gbids, capturedGbids);
}

TEST_F(LocalDiscoveryAggregatorTest, addToAllAsync_callsProxy)
{
    _localDiscoveryAggregator.setDiscoveryProxy(_discoveryMock);

    types::DiscoveryEntryWithMetaInfo discoveryEntry;
    discoveryEntry.setParticipantId("testParticipantId");
    const bool awaitGlobalRegistration = true;
    const joynr::MessagingQos messagingQos(1516);

    EXPECT_CALL(*_discoveryMock, addToAllAsyncMock(Eq(discoveryEntry),
                                                  Eq(awaitGlobalRegistration),
                                                  _, // onSuccess
                                                  _, // onApplicationError
                                                  _, // onRuntimeError
                                                  Eq(messagingQos)));
    _localDiscoveryAggregator.addToAllAsync(discoveryEntry, awaitGlobalRegistration, nullptr, nullptr, nullptr, messagingQos);
}

TEST_F(LocalDiscoveryAggregatorTest, removeAsync_callsProxy)
{
    _localDiscoveryAggregator.setDiscoveryProxy(_discoveryMock);

    const std::string participantId("testParticipantId");
    const joynr::MessagingQos messagingQos(1800);
    EXPECT_CALL(*_discoveryMock, removeAsyncMock(Eq(participantId), Eq(nullptr), Eq(nullptr), Eq(messagingQos)));

    _localDiscoveryAggregator.removeAsync(participantId, nullptr, nullptr, messagingQos);
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncDomainInterface_callsProxy)
{
    _localDiscoveryAggregator.setDiscoveryProxy(_discoveryMock);

    const std::vector<std::string> domains{"domain1", "domain2"};
    const std::string interfaceName("testInterfaceName");
    const types::DiscoveryQos discoveryQos(21, 22, types::DiscoveryScope::LOCAL_AND_GLOBAL, true);
    const joynr::MessagingQos messagingQos(1404);
    EXPECT_CALL(
            *_discoveryMock,
            lookupAsyncMock(
                    Eq(domains),
                    Eq(interfaceName),
                    Eq(discoveryQos),
                    Matcher<std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>>(_),
                    _,
                    Eq(messagingQos)));
    _localDiscoveryAggregator.lookupAsync(domains, interfaceName, discoveryQos, nullptr, nullptr, messagingQos);
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncDomainInterface_withGbids_callsProxy)
{
    _localDiscoveryAggregator.setDiscoveryProxy(_discoveryMock);

    const std::vector<std::string> domains{"domain1", "domain2"};
    const std::string interfaceName("testInterfaceName");
    const types::DiscoveryQos discoveryQos(21, 22, types::DiscoveryScope::LOCAL_AND_GLOBAL, true);
    const std::vector<std::string> gbids = { "testGbid1", "testGbid2", "testGbid3" };
    const joynr::MessagingQos messagingQos(1701);
    EXPECT_CALL(
            *_discoveryMock,
            lookupAsyncMock(
                    Eq(domains),
                    Eq(interfaceName),
                    Eq(discoveryQos),
                    Eq(gbids),
                    Eq(nullptr),
                    Eq(nullptr),
                    Eq(nullptr),
                    Eq(messagingQos)));
    _localDiscoveryAggregator.lookupAsync(domains, interfaceName, discoveryQos, gbids, nullptr, nullptr, nullptr, messagingQos);
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncParticipantId_callsProxy)
{
    _localDiscoveryAggregator.setDiscoveryProxy(_discoveryMock);

    const std::string participantId("testParticipantId");
    const joynr::MessagingQos messagingQos(1503);
    EXPECT_CALL(*_discoveryMock, lookupAsyncMock(Eq(participantId), Eq(nullptr), Eq(nullptr), Eq(messagingQos)));

    _localDiscoveryAggregator.lookupAsync(participantId, nullptr, nullptr, messagingQos);
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncParticipantId_withGbids_callsProxy)
{
    _localDiscoveryAggregator.setDiscoveryProxy(_discoveryMock);

    const std::string participantId("testParticipantId");
    const types::DiscoveryQos discoveryQos(21, 22, types::DiscoveryScope::LOCAL_AND_GLOBAL, true);
    const std::vector<std::string> gbids = { "testGbid1", "testGbid2", "testGbid3" };
    const joynr::MessagingQos messagingQos(1602);
    EXPECT_CALL(*_discoveryMock, lookupAsyncMock(Eq(participantId), Eq(discoveryQos), Eq(gbids), Eq(nullptr), Eq(nullptr), Eq(nullptr), Eq(messagingQos)));

    _localDiscoveryAggregator.lookupAsync(participantId, discoveryQos, gbids, nullptr, nullptr, nullptr, messagingQos);
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncParticipantId_provisionedEntry_doesNotCallProxy)
{
    const std::string participantId("testProvisionedParticipantId");

    types::DiscoveryEntryWithMetaInfo provisionedDiscoveryEntry;
    provisionedDiscoveryEntry.setParticipantId("testProvisionedParticipantId");
    const types::DiscoveryEntryWithMetaInfo expectedDiscoveryEntry(provisionedDiscoveryEntry);
    _provisionedDiscoveryEntries.insert(std::make_pair(participantId, provisionedDiscoveryEntry));
    LocalDiscoveryAggregator testLocalDiscoveryAggregator(_provisionedDiscoveryEntries);
    testLocalDiscoveryAggregator.setDiscoveryProxy(_discoveryMock);

    EXPECT_CALL(*_discoveryMock, lookupAsyncMock(_, _, _, _)).Times(0);
    EXPECT_CALL(*_discoveryMock, lookupAsyncMock(_, _, _, _, _, _, _)).Times(0);

    auto onSuccess = [this, &expectedDiscoveryEntry](
            const types::DiscoveryEntryWithMetaInfo& entry) {
        EXPECT_EQ(expectedDiscoveryEntry, entry);
        _semaphore.notify();
    };

    auto future = testLocalDiscoveryAggregator.lookupAsync(participantId, onSuccess, nullptr);

    types::DiscoveryEntryWithMetaInfo result;
    future->get(100, result);

    EXPECT_EQ(expectedDiscoveryEntry, result);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(100)));
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncParticipantId_withGbids_provisionedEntry_doesNotCallProxy)
{
    const std::string participantId("testProvisionedParticipantId");

    types::DiscoveryEntryWithMetaInfo provisionedDiscoveryEntry;
    provisionedDiscoveryEntry.setParticipantId("testProvisionedParticipantId");
    const types::DiscoveryEntryWithMetaInfo expectedDiscoveryEntry(provisionedDiscoveryEntry);
    _provisionedDiscoveryEntries.insert(std::make_pair(participantId, provisionedDiscoveryEntry));
    LocalDiscoveryAggregator testLocalDiscoveryAggregator(_provisionedDiscoveryEntries);
    testLocalDiscoveryAggregator.setDiscoveryProxy(_discoveryMock);

    EXPECT_CALL(*_discoveryMock, lookupAsyncMock(_, _, _, _)).Times(0);
    EXPECT_CALL(*_discoveryMock, lookupAsyncMock(_, _, _, _, _, _, _)).Times(0);

    auto onSuccess = [this, &expectedDiscoveryEntry](
            const types::DiscoveryEntryWithMetaInfo& entry) {
        EXPECT_EQ(expectedDiscoveryEntry, entry);
        _semaphore.notify();
    };

    const types::DiscoveryQos discoveryQos(21, 22, types::DiscoveryScope::LOCAL_AND_GLOBAL, true);
    const std::vector<std::string> gbids = { "testGbid1", "testGbid2", "testGbid3" };
    auto future = testLocalDiscoveryAggregator.lookupAsync(participantId, discoveryQos, gbids, onSuccess, nullptr, nullptr);

    types::DiscoveryEntryWithMetaInfo result;
    future->get(100, result);

    EXPECT_EQ(expectedDiscoveryEntry, result);

    EXPECT_TRUE(_semaphore.waitFor(std::chrono::milliseconds(100)));
}
