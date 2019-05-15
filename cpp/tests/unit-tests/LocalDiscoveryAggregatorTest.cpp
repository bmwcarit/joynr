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

#include <gtest/gtest.h>

#include "joynr/LocalDiscoveryAggregator.h"

#include "joynr/Future.h"
#include "joynr/Semaphore.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/DiscoveryQos.h"

#include "tests/mock/MockDiscovery.h"

using namespace ::testing;
using namespace joynr;

class LocalDiscoveryAggregatorTest : public ::testing::Test
{
public:
    LocalDiscoveryAggregatorTest()
            : provisionedDiscoveryEntries(),
              localDiscoveryAggregator(provisionedDiscoveryEntries),
              discoveryMock(std::make_shared<MockDiscovery>())
    {
    }

    ~LocalDiscoveryAggregatorTest() override
    {
    }

protected:
    std::map<std::string, types::DiscoveryEntryWithMetaInfo> provisionedDiscoveryEntries;
    LocalDiscoveryAggregator localDiscoveryAggregator;
    std::shared_ptr<MockDiscovery> discoveryMock;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalDiscoveryAggregatorTest);
};

TEST_F(LocalDiscoveryAggregatorTest, addAsync_proxyNotSet_doesNotThrow)
{
    const types::DiscoveryEntryWithMetaInfo discoveryEntry;
    EXPECT_DEATH(localDiscoveryAggregator.addAsync(discoveryEntry, false, nullptr, nullptr),
                 "Assertion.*");
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncDomainInterface_proxyNotSet_doesNotThrow)
{
    const std::vector<std::string> domains;
    const std::string interfaceName;
    const types::DiscoveryQos discoveryQos;
    EXPECT_DEATH(localDiscoveryAggregator.lookupAsync(
                         domains, interfaceName, discoveryQos),
                 "Assertion.*");
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncParticipantId_proxyNotSet_doesNotThrow)
{
    const std::string participantId;
    EXPECT_DEATH(
            localDiscoveryAggregator.lookupAsync(participantId), "Assertion.*");
}

TEST_F(LocalDiscoveryAggregatorTest, removeAsync_proxyNotSet_doesNotThrow)
{
    const std::string participantId;
    EXPECT_DEATH(
            localDiscoveryAggregator.removeAsync(participantId), "Assertion.*");
}

TEST_F(LocalDiscoveryAggregatorTest, addAsync_callsProxy)
{
    localDiscoveryAggregator.setDiscoveryProxy(discoveryMock);

    types::DiscoveryEntryWithMetaInfo discoveryEntry;
    discoveryEntry.setParticipantId("testParticipantId");
    EXPECT_CALL(*discoveryMock, addAsyncMock(Eq(discoveryEntry), _, Eq(nullptr), Eq(nullptr), _));
    localDiscoveryAggregator.addAsync(discoveryEntry, false, nullptr, nullptr);
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncDomainInterface_callsProxy)
{
    localDiscoveryAggregator.setDiscoveryProxy(discoveryMock);

    const std::vector<std::string> domains{"domain1", "domain2"};
    const std::string interfaceName("testInterfaceName");
    types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeout(42421);
    EXPECT_CALL(
            *discoveryMock,
            lookupAsyncMock(
                    Eq(domains), Eq(interfaceName), Eq(discoveryQos),Matcher<std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>>(_),_,_));
    localDiscoveryAggregator.lookupAsync(domains, interfaceName, discoveryQos);
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncParticipantId_callsProxy)
{
    localDiscoveryAggregator.setDiscoveryProxy(discoveryMock);

    const std::string participantId("testParticipantId");
    EXPECT_CALL(*discoveryMock, lookupAsyncMock(Eq(participantId), Eq(nullptr), Eq(nullptr), _));
    localDiscoveryAggregator.lookupAsync(participantId);
}

TEST_F(LocalDiscoveryAggregatorTest, removeAsync_callsProxy)
{
    localDiscoveryAggregator.setDiscoveryProxy(discoveryMock);

    const std::string participantId("testParticipantId");
    EXPECT_CALL(*discoveryMock, removeAsyncMock(Eq(participantId), Eq(nullptr), Eq(nullptr), _));
    localDiscoveryAggregator.removeAsync(participantId, nullptr, nullptr);
}

TEST_F(LocalDiscoveryAggregatorTest, lookupAsyncParticipantId_provisionedEntry_doesNotCallProxy)
{
    Semaphore semaphore(0);
    const std::string participantId("testProvisionedParticipantId");

    types::DiscoveryEntryWithMetaInfo provisionedDiscoveryEntry;
    provisionedDiscoveryEntry.setParticipantId("testProvisionedParticipantId");
    provisionedDiscoveryEntries.insert(std::make_pair(participantId, provisionedDiscoveryEntry));
    LocalDiscoveryAggregator localDiscoveryAggregator(provisionedDiscoveryEntries);
    localDiscoveryAggregator.setDiscoveryProxy(discoveryMock);

    EXPECT_CALL(*discoveryMock, lookupAsyncMock(_, _, _, _)).Times(0);

    auto onSuccess = [&semaphore, &provisionedDiscoveryEntry](
            const types::DiscoveryEntryWithMetaInfo& entry) {
        EXPECT_EQ(entry, provisionedDiscoveryEntry);
        semaphore.notify();
    };

    auto future = localDiscoveryAggregator.lookupAsync(participantId, onSuccess, nullptr);

    types::DiscoveryEntryWithMetaInfo result;
    future->get(100, result);

    EXPECT_EQ(result, provisionedDiscoveryEntry);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(100)));
}
