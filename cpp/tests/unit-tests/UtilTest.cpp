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
#include <limits>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/Util.h"

using namespace joynr;

namespace
{

void validatePartitionsAlwaysThrow(bool allowWildCard)
{
    EXPECT_THROW(util::validatePartitions({""}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({" "}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"_"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"not_valid"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"ä"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "_ ./$"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "_ ./$"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"+a", "bc"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "*123"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"*", "bc"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"*", "*"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"*", "+"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"*", "+"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "*", "+"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(
            util::validatePartitions({"abc", "*", "123"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "*", ""}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"a+b", "123"}, allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"a*b", "123"}, allowWildCard), std::invalid_argument);
}

} // anonymous namespace

TEST(UtilTest, createMulticastIdWithPartitions)
{
    EXPECT_EQ("providerParticipantId/multicastName/partition0/partition1",
              util::createMulticastId(
                      "providerParticipantId", "multicastName", {"partition0", "partition1"}));
}

TEST(UtilTest, createMulticastIdWithoutPartitions)
{
    std::vector<std::string> partitions;
    EXPECT_EQ("providerParticipantId/multicastName",
              util::createMulticastId("providerParticipantId", "multicastName", partitions));
}

TEST(UtilTest, validateValidPartitionsWithWildCardsDoesNotThrow)
{
    bool allowWildCard = true;

    EXPECT_NO_THROW(util::validatePartitions(
            {"valid", "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"},
            allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({}, allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"*"}, allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"+"}, allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"abc", "*"}, allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"abc", "+", "123"}, allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"abc", "+", "*"}, allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"+", "+", "+"}, allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"+", "+", "123"}, allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"+", "123", "*"}, allowWildCard));

    allowWildCard = false;
    EXPECT_NO_THROW(util::validatePartitions(
            {"valid", "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"},
            allowWildCard));
}

TEST(UtilTest, validateValidPartitionsWithWildCardsThrows)
{
    bool doNotAllowWildCard = false;
    EXPECT_THROW(util::validatePartitions({"*"}, doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"+"}, doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "*"}, doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "+", "123"}, doNotAllowWildCard),
                 std::invalid_argument);
    EXPECT_THROW(
            util::validatePartitions({"abc", "+", "*"}, doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(
            util::validatePartitions({"+", "+", "+"}, doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(
            util::validatePartitions({"+", "+", "123"}, doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(
            util::validatePartitions({"+", "123", "*"}, doNotAllowWildCard), std::invalid_argument);

    validatePartitionsAlwaysThrow(doNotAllowWildCard);
    validatePartitionsAlwaysThrow(!doNotAllowWildCard);
}

TEST(UtilTest, checkFileExists)
{
    EXPECT_TRUE(util::fileExists("."));
    EXPECT_TRUE(util::fileExists(".."));
    EXPECT_FALSE(util::fileExists(util::createUuid() + "_NOT_EXISTING_FILE"));

    const std::string fileToTest = "TEST_ThisFileIsCreatedAndDeletedAtRuntime.check";
    EXPECT_FALSE(util::fileExists(fileToTest));
    util::saveStringToFile(fileToTest, " ");
    EXPECT_TRUE(util::fileExists(fileToTest));
    std::remove(fileToTest.c_str());
    EXPECT_FALSE(util::fileExists(fileToTest));
}

TEST(UtilTest, extractParticipantIdFromMulticastId)
{
    EXPECT_EQ("participantId",
              util::extractParticipantIdFromMulticastId(
                      "participantId/multicastname/partition1/partition2"));
    EXPECT_EQ("participantId", util::extractParticipantIdFromMulticastId("participantId/"));
    EXPECT_THROW(util::extractParticipantIdFromMulticastId("participantId"), std::invalid_argument);
}

TEST(UtilTest, isAdditionOnPointerSafe)
{
    constexpr std::uintptr_t address = std::numeric_limits<std::uintptr_t>::max() - 1;

    // no overflow
    int payloadLength = 0x1;
    EXPECT_FALSE(util::isAdditionOnPointerCausesOverflow(address, payloadLength));

    // overflow
    payloadLength = 0x2;
    EXPECT_TRUE(util::isAdditionOnPointerCausesOverflow(address, payloadLength));
}

TEST(UtilTest, setContainsSet)
{
    const std::set<std::string> haystack{"s1", "s2", "s3", "s4"};

    std::set<std::string> needles{"s1", "s2", "s3", "s4"};
    EXPECT_TRUE(util::setContainsSet(haystack, needles));

    needles = {};
    EXPECT_TRUE(util::setContainsSet(haystack, needles));

    needles = {"s2"};
    EXPECT_TRUE(util::setContainsSet(haystack, needles));

    needles = {"s1", "s2"};
    EXPECT_TRUE(util::setContainsSet(haystack, needles));

    needles = {"s1", "s4"};
    EXPECT_TRUE(util::setContainsSet(haystack, needles));
}

TEST(UtilTest, setDoesNotContainSet)
{
    const std::set<std::string> haystack{"s1", "s2", "s3"};

    std::set<std::string> needles{"s1", "s3", "s4"};
    EXPECT_FALSE(util::setContainsSet(haystack, needles));

    needles = {"s4"};
    EXPECT_FALSE(util::setContainsSet(haystack, needles));

    needles = {"s1", "s2", "s3", "s4"};
    EXPECT_FALSE(util::setContainsSet(haystack, needles));

    needles = {"s0", "s1", "s3"};
    EXPECT_FALSE(util::setContainsSet(haystack, needles));
}

TEST(UtilTest, getErrorStringDeliversCorrectString)
{
    int i;

    // check whether MT-safe util::getErrorString() returns
    // same error strings as standard MT-unsafe strerror()
    for (i = 0; i < 255; i++) {
        char* str = strerror(i);
        if (str != NULL) {
            std::string s1(str);
            std::string s2(util::getErrorString(i));
            EXPECT_EQ(s1, s2);
        }
    }
}
