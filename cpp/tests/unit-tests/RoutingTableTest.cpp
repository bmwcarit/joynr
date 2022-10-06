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
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "tests/utils/Gtest.h"
#include <boost/optional/optional_io.hpp>

#include "joynr/RoutingTable.h"

#include "joynr/InProcessMessagingAddress.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Util.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/system/RoutingTypes/WebSocketProtocol.h"

#include "tests/mock/MockDispatcher.h"
#include "tests/mock/MockInProcessMessagingSkeleton.h"

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
              mqttTestValue(
                      std::make_shared<joynr::system::RoutingTypes::MqttAddress>(knownGbids[0],
                                                                                 "mqttTestValue")),
              firstKey("firstKey"),
              secondKey("secondKey"),
              thirdKey("thirdKey")
    {
    }

protected:
    void testLookupByParticipantIdAndGbid(
            const std::string& participantId,
            const std::string& gbid,
            RoutingTable& subject,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> expectedAddress)
    {
        subject.add(participantId, isGloballyVisibleTrue, address, expiryDateMaxMs, isStickyFalse);

        boost::optional<routingtable::RoutingEntry> result =
                subject.lookupRoutingEntryByParticipantIdAndGbid(participantId, gbid);
        if (expectedAddress == nullptr) {
            EXPECT_EQ(boost::none, result);
        } else {
            auto foundAddress = result->address;
            EXPECT_EQ(*expectedAddress, *foundAddress);
            if (participantId == gcdParticipantId) {
                if (auto mqttAddress =
                            dynamic_cast<const joynr::system::RoutingTypes::MqttAddress*>(
                                    foundAddress.get())) {
                    EXPECT_EQ(gbid, mqttAddress->getBrokerUri());
                    EXPECT_NE(*expectedAddress, *address);
                } else {
                    EXPECT_EQ(*expectedAddress, *address);
                }
            } else {
                EXPECT_EQ(*expectedAddress, *address);
            }
        }
    }

    void testLookupByParticipantIdAndGbid_noGbidReplacement(
            const std::string& participantId,
            RoutingTable& subject,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> expectedAddress)
    {
        // auto originalAddress = std::make_shared<const
        // joynr::system::RoutingTypes::Address>(*address);

        testLookupByParticipantIdAndGbid(participantId, "", subject, address, expectedAddress);

        testLookupByParticipantIdAndGbid(
                participantId, "unknownGbid", subject, address, expectedAddress);

        testLookupByParticipantIdAndGbid(participantId, "", subject, address, expectedAddress);

        testLookupByParticipantIdAndGbid(participantId, "", subject, address, expectedAddress);

        // calling the old get API should return the unmodified address
        boost::optional<routingtable::RoutingEntry> result =
                subject.lookupRoutingEntryByParticipantId(participantId);
        EXPECT_TRUE(result);
        EXPECT_EQ(*expectedAddress, *(result->address));
        EXPECT_EQ(*expectedAddress, *address);

        // cleanup
        subject.remove(participantId);
    }

    void testLookupByParticipantIdAndGbid_nonMqttAddress_noGbidReplacement(
            const std::string& participantId,
            RoutingTable& subject)
    {
        auto webSocketAddress =
                std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>(
                        system::RoutingTypes::WebSocketProtocol::WS, "host", 42, "path");
        auto expectedWebSocketAddress =
                std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>(
                        *webSocketAddress);
        testLookupByParticipantIdAndGbid_noGbidReplacement(
                participantId, subject, webSocketAddress, expectedWebSocketAddress);

        auto webSocketClientAddress =
                std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                        "testId");
        auto expectedWebSocketClientAddress =
                std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                        *webSocketClientAddress);
        testLookupByParticipantIdAndGbid_noGbidReplacement(
                participantId, subject, webSocketClientAddress, expectedWebSocketClientAddress);

        auto dispatcher = std::make_shared<MockDispatcher>();
        auto skeleton =
                std::make_shared<MockInProcessMessagingSkeleton>(util::as_weak_ptr(dispatcher));
        auto inProcessAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
        auto expectedInProcessAddress =
                std::make_shared<const joynr::InProcessMessagingAddress>(*inProcessAddress);
        testLookupByParticipantIdAndGbid_noGbidReplacement(
                participantId, subject, inProcessAddress, expectedInProcessAddress);
    }

    const std::string gcdParticipantId;
    const std::vector<std::string> knownGbids;
    RoutingTable routingTable;
    const std::shared_ptr<Address> testValue;
    const std::shared_ptr<Address> secondTestValue;
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

TEST_F(RoutingTableTest,
       lookupRoutingEntryByParticipantIdAndGbid_emptyGbidNotKnown_gcdParticipantId_mqttAddress)
{
    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
            "testGbid", "testTopic");
    auto originalAddress =
            std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(*address);

    testLookupByParticipantIdAndGbid(gcdParticipantId, "", routingTable, address, nullptr);

    testLookupByParticipantIdAndGbid(
            gcdParticipantId, "unknownGbid", routingTable, address, nullptr);

    auto expectedAddress0 = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
            knownGbids[0], originalAddress->getTopic());
    ;
    testLookupByParticipantIdAndGbid(
            gcdParticipantId, knownGbids[0], routingTable, address, expectedAddress0);

    auto expectedAddress1 = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
            knownGbids[1], originalAddress->getTopic());
    ;
    testLookupByParticipantIdAndGbid(
            gcdParticipantId, knownGbids[1], routingTable, address, expectedAddress1);

    // calling the old get API should return the unmodified address
    boost::optional<routingtable::RoutingEntry> result =
            routingTable.lookupRoutingEntryByParticipantId(gcdParticipantId);
    ASSERT_TRUE(result);
    EXPECT_EQ(*originalAddress, *(result->address));
    EXPECT_NE(*expectedAddress0, *(result->address));

    // cleanup
    routingTable.remove(gcdParticipantId);
}

TEST_F(RoutingTableTest,
       lookupRoutingEntryByParticipantIdAndGbid_emptyGbidKnown_gcdParticipantId_mqttAddress)
{
    const std::vector<std::string> gbidsArray{""};
    RoutingTable subject(gcdParticipantId, gbidsArray);

    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
            "testGbid", "testTopic");
    auto originalAddress =
            std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(*address);

    testLookupByParticipantIdAndGbid(gcdParticipantId, "unknownGbid", subject, address, nullptr);

    auto expectedAddress0 = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
            gbidsArray[0], originalAddress->getTopic());
    ;
    testLookupByParticipantIdAndGbid(gcdParticipantId, "", subject, address, expectedAddress0);

    // calling the old get API should return the unmodified address
    boost::optional<routingtable::RoutingEntry> result =
            subject.lookupRoutingEntryByParticipantId(gcdParticipantId);
    ASSERT_TRUE(result);
    EXPECT_EQ(*originalAddress, *(result->address));
    EXPECT_NE(*expectedAddress0, *(result->address));

    // cleanup
    subject.remove(gcdParticipantId);
}

TEST_F(RoutingTableTest,
       lookupRoutingEntryByParticipantIdAndGbid_emptyGbidNotKnown_otherParticipantId_mqttAddress)
{
    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
            "testGbid", "testTopic");
    auto expectedAddress =
            std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(*address);

    testLookupByParticipantIdAndGbid_noGbidReplacement(
            secondKey, routingTable, address, expectedAddress);
}

TEST_F(RoutingTableTest,
       lookupRoutingEntryByParticipantIdAndGbid_emptyGbidKnown_otherParticipantId_mqttAddress)
{
    const std::vector<std::string> gbidsArray{""};
    RoutingTable subject(gcdParticipantId, gbidsArray);

    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
            "testGbid", "testTopic");
    auto expectedAddress =
            std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(*address);

    testLookupByParticipantIdAndGbid_noGbidReplacement(
            secondKey, subject, address, expectedAddress);
}

TEST_F(RoutingTableTest,
       lookupRoutingEntryByParticipantIdAndGbid_emptyGbidNotKnown_gcdParticipantId_nonMqttAddress)
{
    testLookupByParticipantIdAndGbid_nonMqttAddress_noGbidReplacement(
            gcdParticipantId, routingTable);
}

TEST_F(RoutingTableTest,
       lookupRoutingEntryByParticipantIdAndGbid_emptyGbidKnown_gcdParticipantId_nonMqttAddress)
{
    const std::vector<std::string> gbidsArray{""};
    RoutingTable subject(gcdParticipantId, gbidsArray);

    testLookupByParticipantIdAndGbid_nonMqttAddress_noGbidReplacement(gcdParticipantId, subject);
}

TEST_F(RoutingTableTest,
       lookupRoutingEntryByParticipantIdAndGbid_emptyGbidNotKnown_otherParticipantId_nonMqttAddress)
{
    testLookupByParticipantIdAndGbid_nonMqttAddress_noGbidReplacement(secondKey, routingTable);
}

TEST_F(RoutingTableTest,
       lookupRoutingEntryByParticipantIdAndGbid_emptyGbidKnown_otherParticipantId_nonMqttAddress)
{
    const std::vector<std::string> gbidsArray{""};
    RoutingTable subject(gcdParticipantId, gbidsArray);

    testLookupByParticipantIdAndGbid_nonMqttAddress_noGbidReplacement(secondKey, subject);
}

TEST_F(RoutingTableTest, lookupParticipantIdsByAddress)
{
    routingTable.add(firstKey, isGloballyVisibleTrue, testValue, expiryDateMaxMs, isStickyFalse);
    routingTable.add(
            secondKey, isGloballyVisibleTrue, secondTestValue, expiryDateMaxMs, isStickyFalse);
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
    routingTable.add(
            secondKey, isGloballyVisibleTrue, secondTestValue, expiryDateMaxMs, isStickyFalse);
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
    routingTable.add(
            secondKey, isGloballyVisibleTrue, secondTestValue, expiryDateMaxMs, isStickyFalse);
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
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
    const std::int64_t offsetMs = 100;
    const auto expiryDateMs = now + offsetMs;
    routingTable.add(firstKey, isGloballyVisibleTrue, testValue, expiryDateMs, isStickyFalse);
    routingTable.add(secondKey, isGloballyVisibleTrue, secondTestValue, expiryDateMs, isStickyTrue);
    routingTable.add(
            thirdKey, isGloballyVisibleTrue, secondTestValue, expiryDateMaxMs, isStickyFalse);
    ASSERT_TRUE(routingTable.containsParticipantId(firstKey));
    ASSERT_TRUE(routingTable.containsParticipantId(secondKey));
    ASSERT_TRUE(routingTable.containsParticipantId(thirdKey));

    std::this_thread::sleep_for(std::chrono::milliseconds(offsetMs + 1));

    routingTable.purge();

    ASSERT_FALSE(routingTable.containsParticipantId(firstKey));
    ASSERT_TRUE(routingTable.containsParticipantId(secondKey));
    ASSERT_TRUE(routingTable.containsParticipantId(thirdKey));
}
