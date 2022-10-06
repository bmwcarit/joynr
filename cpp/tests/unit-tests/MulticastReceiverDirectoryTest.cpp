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

#include <string>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/MulticastReceiverDirectory.h"

using ::testing::Contains;

class MulticastReceiverDirectoryTest : public testing::Test
{
public:
    MulticastReceiverDirectoryTest()
            : multicastReceiverDirectory(),
              multicastId("testMulticastId"),
              receiverId("testReceiverId")
    {
    }

protected:
    joynr::MulticastReceiverDirectory multicastReceiverDirectory;
    std::string multicastId;
    std::string receiverId;
};

TEST_F(MulticastReceiverDirectoryTest, emptyDirectoryDoesNotContainEntry)
{
    EXPECT_FALSE(multicastReceiverDirectory.contains(multicastId));
    EXPECT_FALSE(multicastReceiverDirectory.contains(multicastId, receiverId));
}

TEST_F(MulticastReceiverDirectoryTest, directoryContainsEntryAfterRegister)
{
    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId);
    EXPECT_TRUE(multicastReceiverDirectory.contains(multicastId));
    EXPECT_TRUE(multicastReceiverDirectory.contains(multicastId, receiverId));
}

TEST_F(MulticastReceiverDirectoryTest, registeringSameEntryTwiceDoesNotThrow)
{
    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId);
    EXPECT_NO_THROW(multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId));
    EXPECT_TRUE(multicastReceiverDirectory.contains(multicastId));
    EXPECT_TRUE(multicastReceiverDirectory.contains(multicastId, receiverId));
}

TEST_F(MulticastReceiverDirectoryTest, directoryDoesNotContainEntryAfterUnregister)
{
    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId);

    multicastReceiverDirectory.unregisterMulticastReceiver(multicastId, receiverId);
    EXPECT_FALSE(multicastReceiverDirectory.contains(multicastId));
    EXPECT_FALSE(multicastReceiverDirectory.contains(multicastId, receiverId));
}

TEST_F(MulticastReceiverDirectoryTest, unregisterReturnsCorrectResult)
{
    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId);
    EXPECT_TRUE(multicastReceiverDirectory.unregisterMulticastReceiver(multicastId, receiverId));
    EXPECT_FALSE(multicastReceiverDirectory.unregisterMulticastReceiver(multicastId, receiverId));
}

TEST_F(MulticastReceiverDirectoryTest, unregisterSameEntryTwiceDoesNotThrow)
{
    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId);

    multicastReceiverDirectory.unregisterMulticastReceiver(multicastId, receiverId);
    EXPECT_NO_THROW(
            multicastReceiverDirectory.unregisterMulticastReceiver(multicastId, receiverId));
    EXPECT_FALSE(multicastReceiverDirectory.contains(multicastId));
    EXPECT_FALSE(multicastReceiverDirectory.contains(multicastId, receiverId));
}

TEST_F(MulticastReceiverDirectoryTest, registerMultipleReceiverIdsForSingleMulticastId)
{
    std::string receiverId2 = "testReceiverId_TWO";
    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId);

    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId2);
    EXPECT_TRUE(multicastReceiverDirectory.contains(multicastId));
    EXPECT_TRUE(multicastReceiverDirectory.contains(multicastId, receiverId));
    EXPECT_TRUE(multicastReceiverDirectory.contains(multicastId, receiverId2));
}

TEST_F(MulticastReceiverDirectoryTest, unregisterLastReceiverIdForSingleMulticastIdRemovesMulticast)
{
    std::string receiverId2 = "testReceiverId_TWO";
    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId);
    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId2);

    multicastReceiverDirectory.unregisterMulticastReceiver(multicastId, receiverId);
    multicastReceiverDirectory.unregisterMulticastReceiver(multicastId, receiverId2);
    EXPECT_FALSE(multicastReceiverDirectory.contains(multicastId));
    EXPECT_FALSE(multicastReceiverDirectory.contains(multicastId, receiverId));
    EXPECT_FALSE(multicastReceiverDirectory.contains(multicastId, receiverId2));
}

TEST_F(MulticastReceiverDirectoryTest, getReceiversReturnsEmptySetForNonExistingMulticats)
{
    EXPECT_TRUE(multicastReceiverDirectory.getReceivers(multicastId).empty());
}

TEST_F(MulticastReceiverDirectoryTest, getReceivers)
{
    std::string receiverId2 = "testReceiverId_TWO";
    std::unordered_set<std::string> expectedReceivers = {receiverId, receiverId2};
    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId);
    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId2);

    std::unordered_set<std::string> receivers =
            multicastReceiverDirectory.getReceivers(multicastId);
    EXPECT_EQ(expectedReceivers, receivers);
}

TEST_F(MulticastReceiverDirectoryTest, directoryCorrectlyHandlesPartitionsExtendingOtherPartition)
{
    std::string multicastId1 = "providerId/partition1/partition2";
    std::string multicastId2 = multicastId1 + "/partition3";
    std::string receiverId2 = "testReceiverId_TWO";
    multicastReceiverDirectory.registerMulticastReceiver(multicastId1, receiverId);
    multicastReceiverDirectory.registerMulticastReceiver(multicastId2, receiverId2);

    std::unordered_set<std::string> expectedReceivers1 = {receiverId};
    std::unordered_set<std::string> receivers1 =
            multicastReceiverDirectory.getReceivers(multicastId1);
    EXPECT_EQ(expectedReceivers1, receivers1);

    std::unordered_set<std::string> expectedReceivers2 = {receiverId2};
    std::unordered_set<std::string> receivers2 =
            multicastReceiverDirectory.getReceivers(multicastId2);
    EXPECT_EQ(expectedReceivers2, receivers2);
}

TEST_F(MulticastReceiverDirectoryTest, getReceiversWithWildCards)
{
    std::vector<std::string> listOfSubscribersId = {"a", "b", "c", "d", "e", "f", "g", "h"};

    multicastReceiverDirectory.registerMulticastReceiver("provider/brod/+", listOfSubscribersId[0]);
    multicastReceiverDirectory.registerMulticastReceiver("provider/brod/a", listOfSubscribersId[1]);
    multicastReceiverDirectory.registerMulticastReceiver(
            "provider/brod/a/+", listOfSubscribersId[2]);
    multicastReceiverDirectory.registerMulticastReceiver(
            "provider/brod/a/+/b", listOfSubscribersId[3]);
    multicastReceiverDirectory.registerMulticastReceiver(
            "provider/brod/a/*", listOfSubscribersId[4]);
    multicastReceiverDirectory.registerMulticastReceiver("provider/brod/*", listOfSubscribersId[5]);
    multicastReceiverDirectory.registerMulticastReceiver(
            "provider/brod/+/+/a", listOfSubscribersId[6]);
    multicastReceiverDirectory.registerMulticastReceiver(
            "provider/brod/+/+/*", listOfSubscribersId[7]);

    std::string subscribeToMulticastID = "provider/brod";
    std::unordered_set<std::string> receivers =
            multicastReceiverDirectory.getReceivers(subscribeToMulticastID);
    std::unordered_set<std::string> expectedReceivers = {listOfSubscribersId[5]};
    EXPECT_EQ(expectedReceivers, receivers);

    subscribeToMulticastID = "provider/brod/a";
    receivers = multicastReceiverDirectory.getReceivers(subscribeToMulticastID);
    expectedReceivers = {listOfSubscribersId[0], listOfSubscribersId[1], listOfSubscribersId[4],
                         listOfSubscribersId[5]};
    EXPECT_EQ(expectedReceivers, receivers);

    subscribeToMulticastID = "provider/brod/a/z";
    receivers = multicastReceiverDirectory.getReceivers(subscribeToMulticastID);
    expectedReceivers = {listOfSubscribersId[2], listOfSubscribersId[4], listOfSubscribersId[5],
                         listOfSubscribersId[7]};
    EXPECT_EQ(expectedReceivers, receivers);

    subscribeToMulticastID = "provider/brod/a/b/a";
    receivers = multicastReceiverDirectory.getReceivers(subscribeToMulticastID);
    expectedReceivers = {listOfSubscribersId[4], listOfSubscribersId[5], listOfSubscribersId[6],
                         listOfSubscribersId[7]};
    EXPECT_EQ(expectedReceivers, receivers);

    subscribeToMulticastID = "provider/brod/a/z/b";
    receivers = multicastReceiverDirectory.getReceivers(subscribeToMulticastID);
    expectedReceivers = {listOfSubscribersId[3], listOfSubscribersId[4], listOfSubscribersId[5],
                         listOfSubscribersId[7]};
    EXPECT_EQ(expectedReceivers, receivers);

    subscribeToMulticastID = "provider/brod/z";
    receivers = multicastReceiverDirectory.getReceivers(subscribeToMulticastID);
    expectedReceivers = {listOfSubscribersId[0], listOfSubscribersId[5]};
    EXPECT_EQ(expectedReceivers, receivers);

    subscribeToMulticastID = "provider/anotherBrod";
    receivers = multicastReceiverDirectory.getReceivers(subscribeToMulticastID);
    expectedReceivers = {};
    EXPECT_EQ(expectedReceivers, receivers);
}

TEST_F(MulticastReceiverDirectoryTest, getMulticastIds)
{
    const std::string multicastId2("part1/name1/a/b/c");
    const std::string receiverId2("testReceiverId2");

    EXPECT_TRUE(multicastReceiverDirectory.getMulticastIds().empty());

    multicastReceiverDirectory.registerMulticastReceiver(multicastId, receiverId);
    multicastReceiverDirectory.registerMulticastReceiver(multicastId2, receiverId2);

    std::vector<std::string> multicastIds = multicastReceiverDirectory.getMulticastIds();

    EXPECT_THAT(multicastIds, Contains(multicastId));
    EXPECT_THAT(multicastIds, Contains(multicastId2));
}
