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

#pragma GCC diagnostic ignored "-Wunsafe-loop-optimizations"

#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <tuple>

#include "tests/utils/Gtest.h"

#include "joynr/ParticipantIdStorage.h"
#include "joynr/Util.h"

using namespace joynr;

static const std::string storageFile("test-participantIdStorageTest.persist");

class ParticipantIdStorageTest
        : public ::testing::TestWithParam<std::tuple<std::string, std::string, std::uint32_t>>
{
public:
    ParticipantIdStorageTest()
    {
        std::remove(storageFile.c_str());

        std::tie(domain, interfaceName, majorVersion) = GetParam();
    }

    std::string domain;
    std::string interfaceName;
    std::int32_t majorVersion;
};

class ParticipantIdStorageAssertTest : public ParticipantIdStorageTest
{
};

// Test that the default participant id is used when no provider exists"
TEST_P(ParticipantIdStorageTest, defaultProviderParticipantId)
{
    ParticipantIdStorage store(storageFile);

    std::string participantId = store.getProviderParticipantId(
            this->domain, this->interfaceName, this->majorVersion, "defaultParticipantId");
    ASSERT_EQ(std::string("defaultParticipantId"), participantId);
}

// Test that a participant id is created when no provider exists and
// no default value is given
TEST_P(ParticipantIdStorageTest, newProviderParticipantId)
{
    ParticipantIdStorage store(storageFile);
    std::string participantId = store.getProviderParticipantId(
            this->domain, this->interfaceName, this->majorVersion, std::string());
    // Check that the id is long enough to be a UUID
    ASSERT_TRUE(participantId.size() > 21);

    // also check get function without default value
    participantId =
            store.getProviderParticipantId(this->domain, this->interfaceName, this->majorVersion);
    // Check that the id is long enough to be a UUID
    ASSERT_TRUE(participantId.size() > 21);
}

// Test that a persisted participant id is used
TEST_P(ParticipantIdStorageTest, persistedProviderParticipantId)
{
    std::string expectedParticipantId;
    {
        ParticipantIdStorage store(storageFile);
        expectedParticipantId = store.getProviderParticipantId(
                this->domain, this->interfaceName, this->majorVersion);
        store.setProviderParticipantId(
                this->domain, this->interfaceName, this->majorVersion, expectedParticipantId);
    }

    // create a new storage
    ParticipantIdStorage store(storageFile);

    // Check that the setting was persisted
    std::string participantId =
            store.getProviderParticipantId(this->domain, this->interfaceName, this->majorVersion);

    ASSERT_EQ(expectedParticipantId, participantId);
}

TEST_P(ParticipantIdStorageTest, settingsAreNotAutomaticallySyncedToFile)
{
    const std::string participantID = "participantID-should-not-be-saved-to-file";
    {
        ParticipantIdStorage store(storageFile);
        store.getProviderParticipantId(this->domain, this->interfaceName, this->majorVersion);
    }
    {
        ParticipantIdStorage store(storageFile);
        std::string queriedParticipantID = store.getProviderParticipantId(
                this->domain, this->interfaceName, this->majorVersion);
        // participantID does not exist
        EXPECT_NE(queriedParticipantID, participantID);
    }
}

std::tuple<std::string, std::string, std::uint32_t> const stringValues[] = {
        // domain: tuple[0]
        // interfaceName: tuple[1]
        // majorVersion: tuple[2]
        std::make_tuple("domain", "interfaceName", 0),
        std::make_tuple("dom.ain", "interfa/ceName", 1),
        std::make_tuple("dom/ain", "interfa.ceName", 10),
        std::make_tuple("dömain", "interßäceName", 1890),
        std::make_tuple("0123456789012345678912", "0123456789012345678912", 1234567890)};
INSTANTIATE_TEST_SUITE_P(checkStrings, ParticipantIdStorageTest, ::testing::ValuesIn(stringValues));

#ifndef NDEBUG
TEST_P(ParticipantIdStorageAssertTest, assertOnGetProviderParticipantId)
{
    ParticipantIdStorage store(storageFile);
    EXPECT_DEATH(
            store.getProviderParticipantId(this->domain, this->interfaceName, this->majorVersion),
            "Assertion.*");
}

TEST_P(ParticipantIdStorageAssertTest, assertOnSetProviderParticipantId)
{
    ParticipantIdStorage store(storageFile);
    EXPECT_DEATH(store.setProviderParticipantId(
                         this->domain, this->interfaceName, this->majorVersion, "participantID"),
                 "Assertion.*");
}

std::tuple<std::string, std::string, std::uint32_t> const failingStrings[] = {
        // domain: tuple[0]
        // interfaceName: tuple[1]
        // majorVersion: tuple[2]
        std::make_tuple("", "", 1), std::make_tuple("", "interfaceName", 1),
        std::make_tuple("domain", "", 1)};
INSTANTIATE_TEST_SUITE_P(failingStrings,
                         ParticipantIdStorageAssertTest,
                         ::testing::ValuesIn(failingStrings));
#endif

TEST(ParticipantIdStorageTest, writeIniFile)
{
    std::remove(storageFile.c_str());
    ParticipantIdStorage store(storageFile);

    const int entriesToWrite = 100;
    for (int i = 0; i < entriesToWrite; ++i) {
        store.setProviderParticipantId(joynr::util::createUuid(),
                                       joynr::util::createUuid(),
                                       1234567890,
                                       joynr::util::createUuid());
    }

    std::ifstream fileStream(storageFile.c_str());
    std::int32_t numberOfEntriesInFile = std::count(
            std::istreambuf_iterator<char>(fileStream), std::istreambuf_iterator<char>(), '\n');

    EXPECT_EQ(entriesToWrite, numberOfEntriesInFile);
}

TEST(ParticipantIdStorageTest, deleteCorruptedFile)
{

    const std::string wrongParticipantID = "WRONG_PARTICIPANT_ID";
    const std::string expectedParticipantId = "EXPECTED_PARTICIPANT_ID";

    {
        std::ofstream file(storageFile, std::ios_base::out | std::ios_base::app);
        const std::string header = "joynr.participant";
        const std::string domain = ".domain";
        const std::string interface = ".interface";
        const std::string majorVersion = ".v5";
        // add twice the same line to make the file an invalid INI file
        file << header << domain << interface << majorVersion << "=" << wrongParticipantID
             << std::endl;
        file << header << domain << interface << majorVersion << "=" << wrongParticipantID
             << std::endl;
    }

    {
        ParticipantIdStorage store(storageFile);
        store.setProviderParticipantId("domain", "interface", 5, expectedParticipantId);

        const std::string defaultValue = "NOT_EXPECTED_DEFAULT";
        const std::string result =
                store.getProviderParticipantId("domain", "interface", 5, defaultValue);
        // if the INI file would be reused, then getProviderParticipantId would return
        // NOT_EXPECTED_DEFAULT instead of EXPECTED_PARTICIPANT_ID
        EXPECT_NE(result, defaultValue);
        EXPECT_NE(result, wrongParticipantID);
        EXPECT_EQ(result, expectedParticipantId);
    }

    {
        // Check it a second time
        ParticipantIdStorage store(storageFile);
        const std::string result = store.getProviderParticipantId("domain", "interface", 5);
        EXPECT_EQ(result, expectedParticipantId);
    }
}

/*
 * Scope of the test is to cause a crash if the underlying storage
 * is accessed simultaneously for reading and writing.
 * The observed crashed happened because a write moved the memory location
 * of the underling boost::ptree and the read crashed.
 */
static const std::string storageFileParallel("ParticipantIdStorageParallelTest.persist");

class ParticipantIdStorageParallelTest : public ::testing::Test
{
public:
    ParticipantIdStorageParallelTest()
            : store(storageFileParallel),
              domain("domain"),
              interfaceName("interfaceName"),
              majorVersion(10),
              participantId("participantId"),
              canStart(false)
    {
        std::remove(storageFileParallel.c_str());
    }

    ~ParticipantIdStorageParallelTest() override
    {
        std::remove(storageFileParallel.c_str());
    }

    void writeOneEntryAndNotify()
    {
        store.setProviderParticipantId(joynr::util::createUuid(),
                                       joynr::util::createUuid(),
                                       1234567890,
                                       joynr::util::createUuid());

        // notify threads
        std::cout << "Done writing first entry." << std::endl;
        {
            std::unique_lock<std::mutex> lock(mutex);
            canStart = true;
        }

        cv.notify_all();
    }

    void writeWithNotify(int numberOfWrites)
    {
        writeOneEntryAndNotify();
        std::cout << "Start writing..." << std::endl;
        writeToStorage(numberOfWrites);
    }

    void readAfterNotify(int numberOfReads)
    {
        std::unique_lock<std::mutex> lock(mutex);
        while (!canStart) {
            cv.wait_for(lock, std::chrono::milliseconds(100));
        }
        readFromStorage(numberOfReads);
    }

    void writeAfterNotify(int numberOfWrites)
    {
        std::unique_lock<std::mutex> lock(mutex);
        while (!canStart) {
            cv.wait_for(lock, std::chrono::milliseconds(100));
        }
        writeToStorage(numberOfWrites);
    }

protected:
    void readFromStorage(int numberOfReads)
    {
        assert(canStart);
        for (int i = 0; i < numberOfReads; ++i) {
            store.getProviderParticipantId(domain, interfaceName, majorVersion);
        }
    }

    void writeToStorage(int numberOfWrites)
    {
        assert(canStart);
        for (int i = 0; i < numberOfWrites; ++i) {
            store.setProviderParticipantId(joynr::util::createUuid(),
                                           joynr::util::createUuid(),
                                           1234567890,
                                           joynr::util::createUuid());
        }
    }

    ParticipantIdStorage store;
    const std::string domain;
    const std::string interfaceName;
    const std::int32_t majorVersion;
    const std::string participantId;

    std::condition_variable cv;
    std::mutex mutex;
    bool canStart;
};

// This test simulates 2 applications using libJoynr, one doing 10000 lookups and the
// other registering 1000 providers
TEST_F(ParticipantIdStorageParallelTest, checkParallel_RW_AccessOfStorage)
{
    std::thread read(&ParticipantIdStorageParallelTest::readAfterNotify, this, 10000);
    std::thread write(&ParticipantIdStorageParallelTest::writeWithNotify, this, 1000);

    read.join();
    write.join();
}

// This test simulates 2 applications using libJoynr, each doing 10000 lookups
TEST_F(ParticipantIdStorageParallelTest, checkParallel_R_AccessOfStorage)
{
    std::thread read_1(&ParticipantIdStorageParallelTest::readAfterNotify, this, 10000);
    std::thread read_2(&ParticipantIdStorageParallelTest::readAfterNotify, this, 10000);

    writeOneEntryAndNotify();

    read_1.join();
    read_2.join();
}

// This test simulates 2 applications using libJoynr, each registering 1000 providers
TEST_F(ParticipantIdStorageParallelTest, checkParallel_W_AccessOfStorage)
{
    std::thread write_1(&ParticipantIdStorageParallelTest::writeAfterNotify, this, 1000);
    std::thread write_2(&ParticipantIdStorageParallelTest::writeAfterNotify, this, 1000);

    writeOneEntryAndNotify();

    write_1.join();
    write_2.join();
}

// This test simulates 10 applications using libJoynr, each registering 100 providers and doing
// 10 lookups in the storage.
TEST_F(ParticipantIdStorageParallelTest, checkParallel_RW_AccessOfStorage_multipleThreads)
{
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.push_back(
                std::thread(&ParticipantIdStorageParallelTest::readAfterNotify, this, 10));
        threads.push_back(
                std::thread(&ParticipantIdStorageParallelTest::writeAfterNotify, this, 100));
    }

    writeOneEntryAndNotify();

    for (auto& t : threads) {
        t.join();
    }
}
