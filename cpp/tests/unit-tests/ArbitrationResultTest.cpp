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

#include "joynr/ArbitrationResult.h"

class ArbitrationResultTest: public ::testing::Test
{
public:
    ArbitrationResultTest()
        : arbitrationResult()
    {}

    joynr::ArbitrationResult arbitrationResult;
};

TEST_F(ArbitrationResultTest, testDiscoveryEntriesIsEmptySuccess)
{
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries;
    arbitrationResult = joynr::ArbitrationResult(discoveryEntries);
    bool isArbitrationResultEmpty = arbitrationResult.isEmpty();
    EXPECT_TRUE(isArbitrationResultEmpty);
}

TEST_F(ArbitrationResultTest, testDiscoveryEntriesIsEmptyFailed)
{
    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry1 = joynr::types::DiscoveryEntryWithMetaInfo();
    discoveryEntry1.setParticipantId("participantId1");
    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry2 = joynr::types::DiscoveryEntryWithMetaInfo();
    discoveryEntry2.setParticipantId("participantId2");
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> expectedDiscoveryEntries {discoveryEntry2, discoveryEntry1};

    arbitrationResult = joynr::ArbitrationResult(expectedDiscoveryEntries);

    bool isArbitrationResultEmpty = arbitrationResult.isEmpty();
    EXPECT_FALSE(isArbitrationResultEmpty);
}

TEST_F(ArbitrationResultTest, testGetDiscoveryEntries)
{
    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry1 = joynr::types::DiscoveryEntryWithMetaInfo();
    discoveryEntry1.setParticipantId("participantId1");
    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry2 = joynr::types::DiscoveryEntryWithMetaInfo();
    discoveryEntry2.setParticipantId("participantId2");
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> expectedDiscoveryEntries {discoveryEntry2, discoveryEntry1};

    arbitrationResult = joynr::ArbitrationResult(expectedDiscoveryEntries);
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> actualDiscoveryEntries = arbitrationResult.getDiscoveryEntries();

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

TEST_F(ArbitrationResultTest, testSetDiscoveryEntries)
{
    arbitrationResult = joynr::ArbitrationResult();
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> actualDiscoveryEntriesBeforeSet = arbitrationResult.getDiscoveryEntries();
    EXPECT_TRUE(actualDiscoveryEntriesBeforeSet.empty());

    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry1 = joynr::types::DiscoveryEntryWithMetaInfo();
    discoveryEntry1.setParticipantId("participantId1");
    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry2 = joynr::types::DiscoveryEntryWithMetaInfo();
    discoveryEntry2.setParticipantId("participantId2");
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> expectedDiscoveryEntriesAfterSet {discoveryEntry2, discoveryEntry1};
    arbitrationResult.setDiscoveryEntries(expectedDiscoveryEntriesAfterSet);
    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> actualDiscoveryEntriesAfterSet = arbitrationResult.getDiscoveryEntries();

    bool discoveryEntry1Found = false;
    bool discoveryEntry2Found = false;
    for (const auto& actualEntry : actualDiscoveryEntriesAfterSet) {
        if (actualEntry.getParticipantId() == discoveryEntry1.getParticipantId()) {
            discoveryEntry1Found = true;
        }
        if (actualEntry.getParticipantId() == discoveryEntry2.getParticipantId()) {
            discoveryEntry2Found = true;
        }
    }
    EXPECT_TRUE(discoveryEntry1Found && discoveryEntry2Found);
    EXPECT_EQ(expectedDiscoveryEntriesAfterSet.size(), actualDiscoveryEntriesAfterSet.size());
}

