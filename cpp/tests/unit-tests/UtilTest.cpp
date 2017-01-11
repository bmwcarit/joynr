/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include <vector>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/Util.h"

using namespace joynr;

namespace {

void validatePartitionsAlwaysThrow(bool allowWildCard) {
    EXPECT_THROW(util::validatePartitions({""},                 allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({" "},                allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"_"},                allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"not_valid"},        allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"ä"},                allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "_ ./$"},     allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "_ ./$"},     allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"+a", "bc"},         allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "*123"},      allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"*", "bc"},          allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"*", "*"},           allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"*", "+"},           allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"*", "+"},           allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "*", "+"},    allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "*", "123"},  allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "*", ""},     allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"a+b", "123"},       allowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"a*b", "123"},       allowWildCard), std::invalid_argument);
}

} // anonymous namespace

TEST(UtilTest, splitIntoJsonObjects)
{
    std::string inputStream;
    std::vector<std::string> result;

    inputStream = R"( not a valid Json )";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(0, result.size());

    inputStream = R"({"id":34})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(1, result.size());
    EXPECT_EQ(result.at(0), R"({"id":34})");

    inputStream = R"({"message":{one:two}}{"id":35})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"message":{one:two}})");
    EXPECT_EQ(result.at(1), R"({"id":35})");

    //payload may not contain { or } outside a string.
    inputStream = R"({"id":3{4})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(0, result.size());

    //  { within a string should be ok
    inputStream = R"({"messa{ge":{one:two}}{"id":35})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"messa{ge":{one:two}})");

    //  } within a string should be ok
    inputStream = R"({"messa}ge":{one:two}}{"id":35})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"messa}ge":{one:two}})");

    //  }{ within a string should be ok
    inputStream = R"({"messa}{ge":{one:two}}{"id":35})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"messa}{ge":{one:two}})");

    //  {} within a string should be ok
    inputStream = R"({"messa{}ge":{one:two}}{"id":35})";
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"messa{}ge":{one:two}})");

    //string may contain \"
    inputStream = R"({"mes\"sa{ge":{one:two}}{"id":35})";
    //inputStream:{"mes\"sa{ge":{one:two}}{"id":35}
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"mes\"sa{ge":{one:two}})");


    inputStream = R"({"mes\\"sa{ge":{one:two}}{"id":35})";
    // inputStream: {"mes\\"sa{ge":{one:two}}{"id":35}
    // / does not escape within JSON String, so the string should not be ended after mes\"
    result = util::splitIntoJsonObjects(inputStream);
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(result.at(0), R"({"mes\\"sa{ge":{one:two}})");
}

TEST(UtilTest, createMulticastIdWithPartitions)
{
    EXPECT_EQ("providerParticipantId/multicastName/partition0/partition1",
              util::createMulticastId("providerParticipantId", "multicastName", { "partition0", "partition1"}));
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
            { "valid", "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" },
            allowWildCard
    ));
    EXPECT_NO_THROW(util::validatePartitions({},                    allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"*"},                 allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"+"},                 allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"abc", "*"},          allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"abc", "+", "123"},   allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"abc", "+", "*"},     allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"+", "+", "+"},       allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"+", "+", "123"},     allowWildCard));
    EXPECT_NO_THROW(util::validatePartitions({"+", "123", "*"},     allowWildCard));

    allowWildCard = false;
    EXPECT_NO_THROW(util::validatePartitions(
            { "valid", "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" },
            allowWildCard
    ));
}

TEST(UtilTest, validateValidPartitionsWithWildCardsThrows)
{
    bool doNotAllowWildCard = false;
    EXPECT_THROW(util::validatePartitions({"*"},                 doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"+"},                 doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "*"},          doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "+", "123"},   doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"abc", "+", "*"},     doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"+", "+", "+"},       doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"+", "+", "123"},     doNotAllowWildCard), std::invalid_argument);
    EXPECT_THROW(util::validatePartitions({"+", "123", "*"},     doNotAllowWildCard), std::invalid_argument);

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
    EXPECT_EQ("participantId", util::extractParticipantIdFromMulticastId("participantId/multicastname/partition1/partition2"));
    EXPECT_EQ("participantId", util::extractParticipantIdFromMulticastId("participantId/"));
    EXPECT_THROW(util::extractParticipantIdFromMulticastId("participantId"), std::invalid_argument);
}
