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

#include "libjoynrclustercontroller/messaging/MessagingPropertiesPersistence.h"
#include "tests/utils/Gtest.h"

#include <cstdio>

using namespace joynr;

static const std::string persistenceFilename("test-messagingPropertiesPersistenceTest.persist");

class MessagingPropertiesPersistenceTest : public ::testing::Test
{
public:
    MessagingPropertiesPersistenceTest()
    {
    }

    void SetUp()
    {
        std::remove(persistenceFilename.c_str());
    }
};

// Test that a channelId is created if none exists
TEST_F(MessagingPropertiesPersistenceTest, createChannelId)
{
    MessagingPropertiesPersistence persist(persistenceFilename);
    std::string channelId = persist.getChannelId();

    // Check that the channel Id looks like a UUID
    ASSERT_TRUE(channelId.size() > 21);
}

// Test that the channelId is persisted
TEST_F(MessagingPropertiesPersistenceTest, persistChannelId)
{
    MessagingPropertiesPersistence* persist =
            new MessagingPropertiesPersistence(persistenceFilename);
    std::string firstChannelId = persist->getChannelId();

    // Remove the persistence object and then create a new one
    delete persist;
    persist = new MessagingPropertiesPersistence(persistenceFilename);

    // Check that the channel id was persisted
    std::string secondChannelId = persist->getChannelId();
    ASSERT_EQ(firstChannelId, secondChannelId);
    delete persist;
}
