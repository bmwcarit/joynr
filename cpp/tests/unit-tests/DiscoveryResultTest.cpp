/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2019 BMW Car IT GmbH
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
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "joynr/DiscoveryQos.h"
#include "joynr/DiscoveryResult.h"

class DiscoveryResultTest: public ::testing::Test
{
public:
    DiscoveryResultTest()
        : discoveryResult()
    {}

    joynr::DiscoveryResult discoveryResult;
};


TEST_F(DiscoveryResultTest, testGetLastSeenHasNoValueWhenEntriesEmpty)
{
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);
    boost::optional<joynr::types::DiscoveryEntry> discoveryEntryLastSeen = discoveryResult.getLastSeen();
    EXPECT_FALSE(discoveryEntryLastSeen.has_value());
}

TEST_F(DiscoveryResultTest, testGetHighestPriorityHasNoValueWhenEntriesEmpty)
{
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);
    boost::optional<joynr::types::DiscoveryEntry> discoveryEntryWithHighestPriority = discoveryResult.getHighestPriority();
    EXPECT_FALSE(discoveryEntryWithHighestPriority.has_value());
}

TEST_F(DiscoveryResultTest, testGetLatestVersionHasNoValueWhenEntriesEmpty)
{
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);
    boost::optional<joynr::types::DiscoveryEntry> discoveryEntryWithLatestVersion = discoveryResult.getLatestVersion();
    EXPECT_FALSE(discoveryEntryWithLatestVersion.has_value());
}

TEST_F(DiscoveryResultTest, testGetParticipantIdHasNoValueWhenEntriesEmpty)
{
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);
    boost::optional<joynr::types::DiscoveryEntry> discoveryEntryWithParticipantId = discoveryResult.getParticipantId("participantId");
    EXPECT_FALSE(discoveryEntryWithParticipantId.has_value());
}

TEST_F(DiscoveryResultTest, testGetWithKeywordHasNoValueWhenEntriesEmpty)
{
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);
    std::vector<joynr::types::DiscoveryEntry> discoveryEntriesWithKeyword = discoveryResult.getWithKeyword("keyword");
    EXPECT_TRUE(discoveryEntriesWithKeyword.empty());
}

TEST_F(DiscoveryResultTest, testGetAllDiscoveryEntries)
{
    joynr::types::DiscoveryEntry discoveryEntry1 = joynr::types::DiscoveryEntry();
    discoveryEntry1.setParticipantId("participantId1");
    joynr::types::DiscoveryEntry discoveryEntry2 = joynr::types::DiscoveryEntry();
    discoveryEntry2.setParticipantId("participantId2");
    std::vector<joynr::types::DiscoveryEntry> expectedDiscoveryEntries {discoveryEntry1, discoveryEntry2};

    discoveryResult = joynr::DiscoveryResult(expectedDiscoveryEntries);
    std::vector<joynr::types::DiscoveryEntry> actualDiscoveryEntries = discoveryResult.getAllDiscoveryEntries();

    bool discoveryEntry1Found = false;
    bool discoveryEntry2Found = false;
    for (const auto& actualEntry : actualDiscoveryEntries) {
        if (actualEntry.getParticipantId() == discoveryEntry1.getParticipantId()) {
            discoveryEntry1Found = true;
        }
        if (actualEntry.getParticipantId() == discoveryEntry2.getParticipantId()) {
            discoveryEntry2Found = true;
        }
    }
    EXPECT_TRUE(discoveryEntry1Found && discoveryEntry2Found);
    EXPECT_EQ(expectedDiscoveryEntries.size(), actualDiscoveryEntries.size());
}

TEST_F(DiscoveryResultTest, testGetLastSeen)
{
    const std::int64_t expectedLastSeenDateMs = 1000;
    const std::int64_t anotherLastSeenDateMs = 100;

    joynr::types::DiscoveryEntry discoveryEntry1 = joynr::types::DiscoveryEntry();
    joynr::types::DiscoveryEntry discoveryEntry2 = joynr::types::DiscoveryEntry();
    discoveryEntry1.setLastSeenDateMs(expectedLastSeenDateMs);
    discoveryEntry2.setLastSeenDateMs(anotherLastSeenDateMs);
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries {discoveryEntry2, discoveryEntry1};
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);

    joynr::types::DiscoveryEntry actualLastSeenDiscoveryEntry = discoveryResult.getLastSeen().value();

    EXPECT_EQ(actualLastSeenDiscoveryEntry.getLastSeenDateMs(), expectedLastSeenDateMs);
}

TEST_F(DiscoveryResultTest, testGetHighestPriority)
{
    const std::int64_t highPriority = 1000;
    const std::int64_t lowPriority = 100;

    joynr::types::ProviderQos highPriorityProviderQos = joynr::types::ProviderQos();
    joynr::types::ProviderQos lowPriorityProviderQos = joynr::types::ProviderQos();
    highPriorityProviderQos.setPriority(highPriority);
    lowPriorityProviderQos.setPriority(lowPriority);

    joynr::types::DiscoveryEntry discoveryEntry1 = joynr::types::DiscoveryEntry();
    joynr::types::DiscoveryEntry discoveryEntry2 = joynr::types::DiscoveryEntry();
    discoveryEntry1.setQos(highPriorityProviderQos);
    discoveryEntry2.setQos(lowPriorityProviderQos);
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries {discoveryEntry2, discoveryEntry1};
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);

    joynr::types::DiscoveryEntry highPriorityDiscoveryEntry = discoveryResult.getHighestPriority().value();

    EXPECT_EQ(highPriorityDiscoveryEntry.getQos().getPriority(), highPriority);
}

TEST_F(DiscoveryResultTest, testGetLastestVersionByMajor)
{
    const joynr::types::Version latestVersion = joynr::types::Version(2, 0);
    const joynr::types::Version oldestVersion = joynr::types::Version(1, 0);

    joynr::types::DiscoveryEntry discoveryEntry1 = joynr::types::DiscoveryEntry();
    joynr::types::DiscoveryEntry discoveryEntry2 = joynr::types::DiscoveryEntry();
    discoveryEntry1.setProviderVersion(latestVersion);
    discoveryEntry2.setProviderVersion(oldestVersion);
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries {discoveryEntry2, discoveryEntry1};
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);

    joynr::types::DiscoveryEntry latestMajorVersionDiscoveryEntry = discoveryResult.getLatestVersion().value();

    EXPECT_EQ(latestMajorVersionDiscoveryEntry.getProviderVersion(), latestVersion);
}

TEST_F(DiscoveryResultTest, testGetLastestVersionByMinor)
{
    const joynr::types::Version latestVersion = joynr::types::Version(1, 2);
    const joynr::types::Version oldestVersion = joynr::types::Version(1, 1);

    joynr::types::DiscoveryEntry discoveryEntry1 = joynr::types::DiscoveryEntry();
    joynr::types::DiscoveryEntry discoveryEntry2 = joynr::types::DiscoveryEntry();
    discoveryEntry1.setProviderVersion(latestVersion);
    discoveryEntry2.setProviderVersion(oldestVersion);
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries {discoveryEntry2, discoveryEntry1};
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);

    joynr::types::DiscoveryEntry latestMinorVersionDiscoveryEntry = discoveryResult.getLatestVersion().value();

    EXPECT_EQ(latestMinorVersionDiscoveryEntry.getProviderVersion(), latestVersion);
}

TEST_F(DiscoveryResultTest, testGetParticipantIdSuccess)
{
    const std::string participantId1 = "participantId1";
    const std::string participantId2 = "participantId2";

    joynr::types::DiscoveryEntry discoveryEntry1 = joynr::types::DiscoveryEntry();
    joynr::types::DiscoveryEntry discoveryEntry2 = joynr::types::DiscoveryEntry();
    discoveryEntry1.setParticipantId(participantId1);
    discoveryEntry2.setParticipantId(participantId2);
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries {discoveryEntry2, discoveryEntry1};
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);

    joynr::types::DiscoveryEntry foundDiscoveryEntry = discoveryResult.getParticipantId(participantId1).value();

    EXPECT_EQ(foundDiscoveryEntry.getParticipantId(), participantId1);
}

TEST_F(DiscoveryResultTest, testGetParticipantIdHasNoValue)
{
    const std::string participantId1 = "participantId1";
    const std::string participantId2 = "participantId2";
    const std::string participantId3 = "participantId3";

    joynr::types::DiscoveryEntry discoveryEntry1 = joynr::types::DiscoveryEntry();
    joynr::types::DiscoveryEntry discoveryEntry2 = joynr::types::DiscoveryEntry();
    discoveryEntry1.setParticipantId(participantId1);
    discoveryEntry2.setParticipantId(participantId2);
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries {discoveryEntry2, discoveryEntry1};
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);

    boost::optional<joynr::types::DiscoveryEntry> discoveryEntryWithNoValue = discoveryResult.getParticipantId(participantId3);

    EXPECT_FALSE(discoveryEntryWithNoValue.has_value());
}

TEST_F(DiscoveryResultTest, testGetWithKeyword)
{
    const std::string keywordValue = "keywordValue";
    const std::string participantId1 = "participantId1";
    const std::string participantId2 = "participantId2";
    const std::string participantId3 = "participantId3";

    joynr::types::CustomParameter keywordParameter = joynr::types::CustomParameter(joynr::DiscoveryQos::KEYWORD_PARAMETER(), keywordValue);

    joynr::types::ProviderQos providerQosWithKeyword = joynr::types::ProviderQos();
    std::vector<joynr::types::CustomParameter> keywordParameters {keywordParameter};
    providerQosWithKeyword.setCustomParameters(keywordParameters);
    joynr::types::ProviderQos providerQosWithoutKeyword = joynr::types::ProviderQos();

    joynr::types::DiscoveryEntry discoveryEntry1 = joynr::types::DiscoveryEntry();
    joynr::types::DiscoveryEntry discoveryEntry2 = joynr::types::DiscoveryEntry();
    joynr::types::DiscoveryEntry discoveryEntry3 = joynr::types::DiscoveryEntry();
    discoveryEntry1.setParticipantId(participantId1);
    discoveryEntry2.setParticipantId(participantId2);
    discoveryEntry3.setParticipantId(participantId3);
    discoveryEntry1.setQos(providerQosWithKeyword);
    discoveryEntry2.setQos(providerQosWithoutKeyword);
    discoveryEntry3.setQos(providerQosWithKeyword);

    std::vector<joynr::types::DiscoveryEntry> discoveryEntries {discoveryEntry3, discoveryEntry2, discoveryEntry1};
    discoveryResult = joynr::DiscoveryResult(discoveryEntries);

    bool discoveryEntry1Found = false;
    bool discoveryEntry3Found = false;

    std::vector<joynr::types::DiscoveryEntry> discoveryEntriesWithKeyword = discoveryResult.getWithKeyword(keywordValue);
    for (const auto& discoveryEntry : discoveryEntriesWithKeyword) {
        if (discoveryEntry.getParticipantId() == discoveryEntry1.getParticipantId()) {
            discoveryEntry1Found = true;
        }
        if (discoveryEntry.getParticipantId() == discoveryEntry3.getParticipantId()) {
            discoveryEntry3Found = true;
        }
    }

    EXPECT_TRUE(discoveryEntry1Found && discoveryEntry3Found);
}
