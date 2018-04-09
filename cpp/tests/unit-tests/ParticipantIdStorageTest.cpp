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

#include <cstdio>
#include <string>
#include <tuple>

#include <gtest/gtest.h>

#include "joynr/ParticipantIdStorage.h"

using namespace joynr;

static const std::string storageFile("test-participantIdStorageTest.persist");

class ParticipantIdStorageTest : public ::testing::TestWithParam<std::tuple<std::string, std::string> > {
public:
    ParticipantIdStorageTest() {
        std::remove(storageFile.c_str());

        auto inputPair = GetParam();
        domain = std::get<0>(inputPair);
        interfaceName = std::get<1>(inputPair);
    }

    std::string domain;
    std::string interfaceName;
};

class ParticipantIdStorageAssertTest : public ParticipantIdStorageTest {};

// Test that the default participant id is used when no provider exists"
TEST_P(ParticipantIdStorageTest, defaultProviderParticipantId)
{
    ParticipantIdStorage store(storageFile);

    std::string participantId = store.getProviderParticipantId(this->domain,
                                                               this->interfaceName,
                                                               "defaultParticipantId");
    ASSERT_EQ(std::string("defaultParticipantId"), participantId);
}

// Test that a participant id is created when no provider exists and
// no default value is given
TEST_P(ParticipantIdStorageTest, newProviderParticipantId)
{
    ParticipantIdStorage store(storageFile);
    std::string participantId = store.getProviderParticipantId(this->domain,
                                                               this->interfaceName,
                                                               std::string());
    // Check that the id is long enough to be a UUID
    ASSERT_TRUE(participantId.size() > 32);

    // also check get function without default value
    participantId = store.getProviderParticipantId(this->domain,
                                                   this->interfaceName);
    // Check that the id is long enough to be a UUID
    ASSERT_TRUE(participantId.size() > 32);
}

// Test that a persisted participant id is used
TEST_P(ParticipantIdStorageTest, persistedProviderParticipantId)
{
    std::string expectedParticipantId;
    {
        ParticipantIdStorage store(storageFile);
        expectedParticipantId = store.getProviderParticipantId(this->domain,
                                                               this->interfaceName);
        store.setProviderParticipantId(this->domain, this->interfaceName, expectedParticipantId);
    }

    // create a new storage
    ParticipantIdStorage store(storageFile);

    // Check that the setting was persisted
    std::string participantId = store.getProviderParticipantId(this->domain,
                                                               this->interfaceName);

    ASSERT_EQ(expectedParticipantId, participantId);
}

TEST_P(ParticipantIdStorageTest, settingsAreNotAutomaticallySyncedToFile)
{
    const std::string participantID = "participantID-should-not-be-saved-to-file";
    {
        ParticipantIdStorage store(storageFile);
        store.getProviderParticipantId(this->domain,
                                       this->interfaceName);
    }
    {
        ParticipantIdStorage store(storageFile);
        std::string queriedParticipantID = store.getProviderParticipantId(this->domain,
                                                                          this->interfaceName);
        //participantID does not exist
        EXPECT_NE(queriedParticipantID, participantID);
    }
}

std::tuple<std::string, std::string> const stringValues[] = {
    // domain: tuple[0]
    // interfaceName: tuple[1]
    std::make_tuple( "domain", "interfaceName"),
    std::make_tuple( "dom.ain", "interfa/ceName"),
    std::make_tuple( "dom/ain", "interfa.ceName"),
    std::make_tuple( "dömain", "interßäceName"),
    std::make_tuple( "0123456789012345678912", "0123456789012345678912")
};
INSTANTIATE_TEST_CASE_P(
  checkStrings, ParticipantIdStorageTest, ::testing::ValuesIn(stringValues));

TEST_P(ParticipantIdStorageAssertTest, assertOnGetProviderParticipantId) {
    ParticipantIdStorage store(storageFile);
    EXPECT_DEATH(store.getProviderParticipantId(this->domain,
                                                this->interfaceName), "Assertion.*");
}

TEST_P(ParticipantIdStorageAssertTest, assertOnSetProviderParticipantId) {
    ParticipantIdStorage store(storageFile);
    EXPECT_DEATH(store.setProviderParticipantId(this->domain,
                                                this->interfaceName,
                                                "participantID"), "Assertion.*");
}

std::tuple<std::string, std::string> const failingStrings[] = {
    // domain: tuple[0]
    // interfaceName: tuple[1]
    std::make_tuple( "", ""),
    std::make_tuple( "", "interfaceName"),
    std::make_tuple( "domain", "")
};
INSTANTIATE_TEST_CASE_P(
  failingStrings, ParticipantIdStorageAssertTest, ::testing::ValuesIn(failingStrings));
