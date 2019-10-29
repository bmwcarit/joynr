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

#include <atomic>

#include <gtest/gtest.h>

#include "joynr/RoutingTable.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

using namespace joynr;
using Address = joynr::system::RoutingTypes::Address;
using WebSocketClientAddress = joynr::system::RoutingTypes::WebSocketClientAddress;

class RoutingTableTest : public ::testing::Test
{
public:
    RoutingTableTest()
            : gcdParticipantId("testGcdParticipantId"),
              knownGbids(std::vector<std::string>{"gbid1", "gbid2"}),
              routingTable(gcdParticipantId, knownGbids),
              testValue(std::make_shared<WebSocketClientAddress>("testValue")),
              secondTestValue(std::make_shared<WebSocketClientAddress>("secondTestValue")),
              testGbid("testGbid"),
              otherGbid("otherGbid"),
              mqttTestValue(std::make_shared<joynr::system::RoutingTypes::MqttAddress>(testGbid, "mqttTestValue")),
              firstKey("firstKey"),
              secondKey("secondKey"),
              thirdKey("thirdKey")
    {
    }

protected:
    const std::string gcdParticipantId;
    const std::vector<std::string> knownGbids;
    RoutingTable routingTable;
    const std::shared_ptr<Address> testValue;
    const std::shared_ptr<Address> secondTestValue;
    const std::string testGbid;
    const std::string otherGbid;
    const std::shared_ptr<Address> mqttTestValue;
    const std::string firstKey;
    const std::string secondKey;
    const std::string thirdKey;
    static constexpr std::int64_t expiryDateMaxMs = std::numeric_limits<std::int64_t>::max();
    static const bool isStickyFalse = false;
    static const bool isStickyTrue = true;
    static const bool isGloballyVisibleTrue = true;

private:
    DISALLOW_COPY_AND_ASSIGN(RoutingTableTest);
};

TEST_F(RoutingTableTest, addAndContains)
{
    routingTable.add(firstKey, isGloballyVisibleTrue, testValue, expiryDateMaxMs, isStickyFalse);
    ASSERT_TRUE(routingTable.containsParticipantId(firstKey));
}

TEST_F(RoutingTableTest, containsNot)
{
    routingTable.add(firstKey, isGloballyVisibleTrue, testValue, expiryDateMaxMs, isStickyFalse);
    ASSERT_TRUE(routingTable.containsParticipantId(firstKey));
    ASSERT_FALSE(routingTable.containsParticipantId(secondKey));
}

TEST_F(RoutingTableTest, lookupRoutingEntryByParticipantId)
{
    const bool firstIsGloballyVisible = true;
    const bool secondIsGloballyVisible = false;
    const bool expectedIsSticky1 = isStickyFalse;
    const std::int64_t expectedExpiryDateMs1 = expiryDateMaxMs;
    const bool expectedIsSticky2 = !isStickyFalse;
    const std::int64_t expectedExpiryDateMs2 = expiryDateMaxMs - 1;
    routingTable.add(
            firstKey, firstIsGloballyVisible, testValue, expectedExpiryDateMs1, expectedIsSticky1);
    routingTable.add(secondKey,
                     secondIsGloballyVisible,
                     secondTestValue,
                     expectedExpiryDateMs2,
                     expectedIsSticky2);
    boost::optional<routingtable::RoutingEntry> result1 =
            routingTable.lookupRoutingEntryByParticipantId(firstKey);
    boost::optional<routingtable::RoutingEntry> result2 =
            routingTable.lookupRoutingEntryByParticipantId(secondKey);
    ASSERT_EQ(*(result1->address), *testValue);
    ASSERT_EQ(result1->isGloballyVisible, firstIsGloballyVisible);
    ASSERT_EQ(result1->_expiryDateMs, expectedExpiryDateMs1);
    ASSERT_EQ(result1->_isSticky, expectedIsSticky1);
    ASSERT_EQ(*(result2->address), *secondTestValue);
    ASSERT_EQ(result2->isGloballyVisible, secondIsGloballyVisible);
    ASSERT_EQ(result2->_expiryDateMs, expectedExpiryDateMs2);
    ASSERT_EQ(result2->_isSticky, expectedIsSticky2);
}

TEST_F(RoutingTableTest, lookupRoutingEntryByParticipantIdAndGbid_gcdParticipantId_mqttAddress)
{
    const std::string expectedGbid = otherGbid;
    //Positive test case: gbid of the existing entry gets replaced with the one from the custom header
    routingTable.add(gcdParticipantId,
                     isGloballyVisibleTrue,
                     mqttTestValue,
                     expiryDateMaxMs,
                     isStickyFalse);
    boost::optional<routingtable::RoutingEntry> result1 =
            routingTable.lookupRoutingEntryByParticipantIdAndGbid(gcdParticipantId, otherGbid);
    ASSERT_EQ(expectedGbid,
              (dynamic_cast<const joynr::system::RoutingTypes::MqttAddress*> ((result1->address).get()))->getBrokerUri());
    routingTable.remove(gcdParticipantId);
}

TEST_F(RoutingTableTest, lookupRoutingEntryByParticipantIdAndGbid_nonGcdParticipantId_mqttAddress)
{
    const std::string expectedGbid = testGbid;
    //Negative test case: requested participantId does not match gcdParticipantId -> no replacement happens
    routingTable.add(
            secondKey, isGloballyVisibleTrue, mqttTestValue, expiryDateMaxMs, isStickyFalse);
    boost::optional<routingtable::RoutingEntry> result3 =
            routingTable.lookupRoutingEntryByParticipantIdAndGbid(secondKey, otherGbid);
    ASSERT_EQ(expectedGbid,
              (dynamic_cast<const joynr::system::RoutingTypes::MqttAddress*> ((result3->address).get()))->getBrokerUri());
    routingTable.remove(secondKey);
}

TEST_F(RoutingTableTest, lookupRoutingEntryByParticipantIdAndGbid_gcdParticipantId_nonMqttAddress)
{
    //Negative test case: address Type of the gcdParticipant is not MqttAddress
    routingTable.add(gcdParticipantId,
                     isGloballyVisibleTrue,
                     secondTestValue,
                     expiryDateMaxMs,
                     isStickyFalse);
    boost::optional<routingtable::RoutingEntry> result2 =
            routingTable.lookupRoutingEntryByParticipantIdAndGbid(gcdParticipantId, otherGbid);
    ASSERT_FALSE(dynamic_cast<const joynr::system::RoutingTypes::MqttAddress*> ((result2->address).get()));
    ASSERT_EQ(secondTestValue, result2->address);
    routingTable.remove(gcdParticipantId);
}

TEST_F(RoutingTableTest, lookupParticipantIdsByAddress)
{
    routingTable.add(firstKey, isGloballyVisibleTrue, testValue, expiryDateMaxMs, isStickyFalse);
    routingTable.add(secondKey, isGloballyVisibleTrue, secondTestValue, expiryDateMaxMs, isStickyFalse);
    routingTable.add(thirdKey, isGloballyVisibleTrue, testValue, expiryDateMaxMs, isStickyFalse);
    std::unordered_set<std::string> result1 = routingTable.lookupParticipantIdsByAddress(testValue);
    bool found1 = (result1.find(firstKey) != result1.end());
    bool found2 = (result1.find(secondKey) != result1.end());
    bool found3 = (result1.find(thirdKey) != result1.end());
    ASSERT_EQ(found1, true);
    ASSERT_EQ(found2, false);
    ASSERT_EQ(found3, true);
    ASSERT_EQ(result1.size(), 2);

    std::unordered_set<std::string> result2 =
            routingTable.lookupParticipantIdsByAddress(secondTestValue);
    bool found4 = (result2.find(secondKey) != result2.end());
    ASSERT_EQ(found4, true);
    ASSERT_EQ(result2.size(), 1);
}

TEST_F(RoutingTableTest, remove)
{
    ASSERT_FALSE(routingTable.containsParticipantId(firstKey));
    ASSERT_FALSE(routingTable.containsParticipantId(secondKey));

    routingTable.add(firstKey, isGloballyVisibleTrue, testValue, expiryDateMaxMs, isStickyFalse);
    routingTable.add(secondKey, isGloballyVisibleTrue, secondTestValue, expiryDateMaxMs, isStickyFalse);
    ASSERT_TRUE(routingTable.containsParticipantId(firstKey));
    ASSERT_TRUE(routingTable.containsParticipantId(secondKey));

    routingTable.remove(firstKey);
    ASSERT_FALSE(routingTable.containsParticipantId(firstKey));
    ASSERT_TRUE(routingTable.containsParticipantId(secondKey));
}

TEST_F(RoutingTableTest, removeDoesNotRemoveStickyRoutingEntry)
{
    ASSERT_FALSE(routingTable.containsParticipantId(firstKey));
    ASSERT_FALSE(routingTable.containsParticipantId(secondKey));

    routingTable.add(firstKey, isGloballyVisibleTrue, testValue, expiryDateMaxMs, isStickyTrue);
    routingTable.add(secondKey, isGloballyVisibleTrue, secondTestValue, expiryDateMaxMs, isStickyFalse);
    ASSERT_TRUE(routingTable.containsParticipantId(firstKey));
    ASSERT_TRUE(routingTable.containsParticipantId(secondKey));

    routingTable.remove(firstKey);
    routingTable.remove(secondKey);
    ASSERT_TRUE(routingTable.containsParticipantId(firstKey));
    ASSERT_FALSE(routingTable.containsParticipantId(secondKey));
}

TEST_F(RoutingTableTest, lookupNonExistingKeys)
{
    ASSERT_FALSE(routingTable.lookupRoutingEntryByParticipantId("__THIS__KEY__DOES__NOT__EXIST__"));
}

TEST_F(RoutingTableTest, purge)
{
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch()).count();
    const std::int64_t offsetMs = 100;
    const auto expiryDateMs = now + offsetMs;
    routingTable.add(firstKey, isGloballyVisibleTrue, testValue, expiryDateMs, isStickyFalse);
    routingTable.add(secondKey, isGloballyVisibleTrue, secondTestValue, expiryDateMs, isStickyTrue);
    routingTable.add(thirdKey, isGloballyVisibleTrue, secondTestValue, expiryDateMaxMs, isStickyFalse);
    ASSERT_TRUE(routingTable.containsParticipantId(firstKey));
    ASSERT_TRUE(routingTable.containsParticipantId(secondKey));
    ASSERT_TRUE(routingTable.containsParticipantId(thirdKey));

    std::this_thread::sleep_for(std::chrono::milliseconds(offsetMs + 1));

    routingTable.purge();

    ASSERT_FALSE(routingTable.containsParticipantId(firstKey));
    ASSERT_TRUE(routingTable.containsParticipantId(secondKey));
    ASSERT_TRUE(routingTable.containsParticipantId(thirdKey));
}
