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

using namespace joynr;
using Address = joynr::system::RoutingTypes::Address;
using WebSocketClientAddress = joynr::system::RoutingTypes::WebSocketClientAddress;

class RoutingTableTest : public ::testing::Test
{
public:
    RoutingTableTest()
        : routingTable(),
          testValue(nullptr),
          secondTestValue(nullptr),
          firstKey(""),
          secondKey(""),
          thirdKey("")
    {
    }

    void SetUp(){
        testValue = std::make_shared<WebSocketClientAddress>("testValue");
        secondTestValue = std::make_shared<WebSocketClientAddress>("secondTestValue");
        firstKey = std::string("firstKey");
        secondKey = std::string("secondKey");
        thirdKey = std::string("thirdKey");
    }

protected:
    RoutingTable routingTable;
    std::shared_ptr<Address> testValue;
    std::shared_ptr<Address> secondTestValue;
    std::string firstKey;
    std::string secondKey;
    std::string thirdKey;
private:
    DISALLOW_COPY_AND_ASSIGN(RoutingTableTest);
};

TEST_F(RoutingTableTest, addAndContains)
{
    const bool isGloballyVisible = true;
    routingTable.add(firstKey, isGloballyVisible, testValue);
    ASSERT_TRUE(routingTable.containsParticipantId(firstKey));
}

TEST_F(RoutingTableTest, containsNot)
{
    const bool isGloballyVisible = true;
    routingTable.add(firstKey, isGloballyVisible, testValue);
    ASSERT_TRUE(routingTable.containsParticipantId(firstKey));
    ASSERT_FALSE(routingTable.containsParticipantId(secondKey));
}

TEST_F(RoutingTableTest, lookupRoutingEntryByParticipantId)
{
    const bool firstIsGloballyVisible = true;
    const bool secondIsGloballyVisible = false;
    routingTable.add(firstKey, firstIsGloballyVisible, testValue);
    routingTable.add(secondKey,secondIsGloballyVisible, secondTestValue);
    boost::optional<routingtable::RoutingEntry> result1 = routingTable.lookupRoutingEntryByParticipantId(firstKey);
    boost::optional<routingtable::RoutingEntry> result2 = routingTable.lookupRoutingEntryByParticipantId(secondKey);
    ASSERT_EQ(*(result1->address), *testValue);
    ASSERT_EQ(result1->isGloballyVisible, firstIsGloballyVisible);
    ASSERT_EQ(*(result2->address), *secondTestValue);
    ASSERT_EQ(result2->isGloballyVisible, secondIsGloballyVisible);
}

TEST_F(RoutingTableTest, lookupParticipantIdsByAddress)
{
    const bool isGloballyVisible = true;
    routingTable.add(firstKey, isGloballyVisible, testValue);
    routingTable.add(secondKey,isGloballyVisible, secondTestValue);
    routingTable.add(thirdKey, isGloballyVisible, testValue);
    std::unordered_set<std::string> result1 = routingTable.lookupParticipantIdsByAddress(testValue);
    bool found1 = (result1.find(firstKey) != result1.end());
    bool found2 = (result1.find(secondKey) != result1.end());
    bool found3 = (result1.find(thirdKey) != result1.end());
    ASSERT_EQ(found1, true);
    ASSERT_EQ(found2, false);
    ASSERT_EQ(found3, true);
    ASSERT_EQ(result1.size(), 2);

    std::unordered_set<std::string> result2 = routingTable.lookupParticipantIdsByAddress(secondTestValue);
    bool found4 = (result2.find(secondKey) != result2.end());
    ASSERT_EQ(found4, true);
    ASSERT_EQ(result2.size(), 1);
}

TEST_F(RoutingTableTest, remove)
{
    const bool isGloballyVisible = true;
    routingTable.add(firstKey, isGloballyVisible, testValue);
    routingTable.add(secondKey, isGloballyVisible, secondTestValue);
    ASSERT_TRUE(routingTable.containsParticipantId(firstKey));
    ASSERT_TRUE(routingTable.containsParticipantId(secondKey));
    routingTable.remove(firstKey);
    ASSERT_FALSE(routingTable.containsParticipantId(firstKey));
    ASSERT_TRUE(routingTable.containsParticipantId(secondKey));
}

TEST_F(RoutingTableTest, lookupNonExistingKeys)
{
    ASSERT_FALSE(routingTable.lookupRoutingEntryByParticipantId("__THIS__KEY__DOES__NOT__EXIST__"));
}
