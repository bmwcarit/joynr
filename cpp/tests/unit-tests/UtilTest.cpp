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

#include "muesli/detail/IncrementalTypeList.h"
#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/Util.h"
#include <boost/filesystem.hpp>
#include <boost/mpl/find.hpp>
#include <fstream>

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

TEST(UtilTest, createValidUuid)
{
    std::string uuid1 = util::createUuid();
    std::string uuid2 = util::createUuid();
    EXPECT_NE(uuid1, uuid2);
}

TEST(UtilTest, vectorContainsContainsValue)
{
    const std::vector<std::string> stringValues{"s1", "s2", "s3", "s4"};
    std::string stringValue = "s1";
    EXPECT_TRUE(util::vectorContains(stringValues, stringValue));

    const std::vector<int> intValues{1, 2, 3, 4, 5};
    int intValue = 3;
    EXPECT_TRUE(util::vectorContains(intValues, intValue));

    const std::vector<double> doubleValues{1.2, 2.3, 3.4, 4.5, 5.6, 6.7};
    double doubleValue = 1.2;
    EXPECT_TRUE(util::vectorContains(doubleValues, doubleValue));
}

TEST(UtilTest, vectorDoesNotContainValue)
{
    const std::vector<std::string> stringValues{"s1", "s2", "s3", "s4y"};
    std::string stringValue = "s5";
    EXPECT_FALSE(util::vectorContains(stringValues, stringValue));

    const std::vector<int> intValues{1, 2, 3, 4, 5};
    int intValue = 7;
    EXPECT_FALSE(util::vectorContains(intValues, intValue));

    const std::vector<double> doubleValues{1.2, 2.3, 3.4, 4.5, 5.6, 6.7};
    double doubleValue = 0.5;
    EXPECT_FALSE(util::vectorContains(doubleValues, doubleValue));
}

TEST(UtilTest, removeAllTest)
{
    std::vector<int> intValues{1, 2, 3, 4, 5};
    EXPECT_EQ(intValues.size(), 5);
    util::removeAll(intValues, 3);
    EXPECT_EQ(std::find(intValues.begin(), intValues.end(), 3), intValues.end());
    EXPECT_EQ(intValues.size(), 4);

    std::vector<double> doubleValues{1.1, 1.1, 1.1, 1.1};
    EXPECT_EQ(doubleValues.size(), 4);
    util::removeAll(doubleValues, 1.1);
    EXPECT_EQ(std::find(doubleValues.begin(), doubleValues.end(), 1.1), doubleValues.end());
    EXPECT_EQ(doubleValues.size(), 0);

    std::vector<std::string> stringValues{"s1", "s2", "s3", "s4", "s4"};
    EXPECT_EQ(stringValues.size(), 5);
    std::string valueToBeremoved = "s4";
    util::removeAll(stringValues, valueToBeremoved);
    EXPECT_EQ(std::find(stringValues.begin(), stringValues.end(), valueToBeremoved),
              stringValues.end());

    EXPECT_EQ(stringValues.size(), 3);
}

TEST(UtilTest, asWeakPtrTest)
{
    std::shared_ptr<int> intPtr = std::make_shared<int>(10);
    EXPECT_EQ(intPtr.use_count(), 1);
    std::shared_ptr<int> secondPtr = intPtr;
    EXPECT_EQ(intPtr.use_count(), 2);

    auto weakPtr = util::as_weak_ptr(intPtr);
    EXPECT_EQ(intPtr.use_count(), 2);
}

TEST(UtilTest, compareValuesTest)
{
    int intValue1 = 10;
    int intValue2 = 10;
    int intValue3 = 20;

    EXPECT_TRUE(util::compareValues(intValue1, intValue2));
    EXPECT_TRUE(util::compareValues(intValue1, intValue1));
    EXPECT_FALSE(util::compareValues(intValue1, intValue3));

    double doubleValue1 = 5.5;
    double doubleValue2 = 5.5;
    double doubleValue3 = 5.9;

    EXPECT_TRUE(util::compareValues(doubleValue1, doubleValue2));
    EXPECT_TRUE(util::compareValues(doubleValue1, doubleValue1));
    EXPECT_FALSE(util::compareValues(doubleValue1, doubleValue3));
}

TEST(UtilTest, invokeOnTest)
{
    typedef boost::mpl::vector<int, long, float, unsigned> vectorTypes;
    std::vector<std::string> typeIDs;
    typeIDs.push_back(typeid(int).name());
    typeIDs.push_back(typeid(long).name());
    typeIDs.push_back(typeid(float).name());
    typeIDs.push_back(typeid(unsigned).name());

    unsigned int counter = 0;

    auto fun = [&counter, &typeIDs](auto&& holder) {
        using iterType = std::decay_t<decltype(holder)>;
        EXPECT_TRUE((typeIDs[counter].compare(typeid(iterType).name()) == 0));
        counter++;
        return true;
    };

    util::invokeOn<vectorTypes>(fun);
}

TEST(UtilTest, fileExistsTest)
{
    std::string filename = "test.out";
    std::ofstream g(filename);
    g.close();
    EXPECT_TRUE(util::fileExists(filename));
    std::remove(filename.c_str());
    EXPECT_FALSE(util::fileExists(filename));
}

TEST(UtilTest, writeToFileTestSyncFalse)
{
    std::string filename = "test.out";
    std::string stringToWrite = "this is first line\nthis is secondLine\r";
    util::writeToFile(filename, stringToWrite, std::ios::out, false);

    std::stringstream buffer;
    std::ifstream f(filename, std::ios_base::binary);
    buffer << f.rdbuf();
    f.close();
    EXPECT_EQ(stringToWrite, buffer.str());
    std::remove(filename.c_str());
}

TEST(UtilTest, writeToFileTestSyncTrue)
{
    std::string filename = "test.out";
    std::string stringToWrite = "this is first line\nthis is secondLine\r";
    util::writeToFile(filename, stringToWrite, std::ios::out, true);

    std::stringstream buffer;
    std::ifstream f(filename, std::ios_base::binary);
    buffer << f.rdbuf();
    f.close();
    EXPECT_EQ(stringToWrite, buffer.str());
    std::remove(filename.c_str());
}

TEST(UtilTest, writeToFileException)
{
    std::string folderName = "test";
    boost::filesystem::create_directory(folderName);

    EXPECT_THROW(
            util::writeToFile(folderName, "testString", std::ios::out, false), std::runtime_error);
    std::remove(folderName.c_str());
}
TEST(UtilTest, loadStringFromFileTest)
{
    std::string filename = "test.out";
    std::string stringToWrite = "this is first line\nthis is secondLine";
    std::ofstream g(filename);
    g << stringToWrite;
    g.close();

    std::string readString = util::loadStringFromFile(filename);
    EXPECT_EQ(stringToWrite, readString);

    g.open(filename, std::ios_base::app);
    g << stringToWrite;
    g.close();

    stringToWrite.append(stringToWrite);

    readString = util::loadStringFromFile(filename);
    EXPECT_EQ(stringToWrite, readString);

    std::remove(filename.c_str());
}

TEST(UtilTest, loadStringFromFileSizeExceeded)
{
    std::string filename = "sparseFile";
    std::ofstream g(filename, std::ios::binary | std::ios::out);
    std::uint64_t size = 2 * 1024 * 1024 * 1024UL + 10;
    g.seekp((std::streamoff)size);
    g.write("", 1);
    g.close();

    EXPECT_THROW(util::loadStringFromFile(filename), std::runtime_error);
    std::remove(filename.c_str());
}

TEST(UtilTest, loadStringFromFileNoFileTest)
{
    std::string filename = "test.out";
    std::string stringToWrite = "this is first line\nthis is secondLine\n";

    EXPECT_THROW(util::loadStringFromFile(filename), std::runtime_error);
}
