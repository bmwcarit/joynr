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

#include <condition_variable>
#include <cstdio>
#include <string>
#include <thread>
#include <tuple>

#include <gtest/gtest.h>

#include "joynr/ParticipantIdStorage.h"
#include "joynr/Util.h"

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


/*
 * Scope of the test is to cause a crash if the underlying storage
 * is accessed simultaneously for reading and writing.
 * The observed crashed happened because a write moved the memory location
 * of the underling boost::ptree and the read crashed.
 */
static const std::string storageFileParallel("ParticipantIdStorageParallelTest.persist");

class ParticipantIdStorageParallelTest : public ::testing::Test {
public:
    ParticipantIdStorageParallelTest():
        store(storageFileParallel),
        domain("domain"),
        interfaceName("interfaceName"),
        participantId("participantId"),
        canStart(false)
    {
        std::remove(storageFileParallel.c_str());
    }

    ~ParticipantIdStorageParallelTest() override {
        std::remove(storageFileParallel.c_str());
    }

    void writeOneEntryAndNotify() {
        store.setProviderParticipantId(joynr::util::createUuid(),
                                       joynr::util::createUuid(),
                                       joynr::util::createUuid());

        // notify threads
        std::cout << "Done writing first entry." << std::endl;
        {
            std::unique_lock<std::mutex> lock(mutex);
            canStart = true;
        }

        cv.notify_all();
    }

    void writeWithNotify(int numberOfWrites) {
        writeOneEntryAndNotify();
        std::cout << "Start writing..." << std::endl;
        writeToStorage(numberOfWrites);
    }

    void readAfterNotify(int numberOfReads) {
        std::unique_lock<std::mutex> lock(mutex);
        while(!canStart) {
            cv.wait_for(lock, std::chrono::milliseconds(100));
        }
        readFromStorage(numberOfReads);
    }

    void writeAfterNotify(int numberOfWrites) {
        std::unique_lock<std::mutex> lock(mutex);
        while(!canStart) {
            cv.wait_for(lock, std::chrono::milliseconds(100));
        }
        writeToStorage(numberOfWrites);
    }

protected:
    void readFromStorage(int numberOfReads) {
        assert(canStart);
        for (int i = 0; i < numberOfReads; ++i) {
            store.getProviderParticipantId(domain, interfaceName);
        }
    }

    void writeToStorage(int numberOfWrites) {
        assert(canStart);
        for (int i = 0; i < numberOfWrites; ++i) {
            store.setProviderParticipantId(joynr::util::createUuid(),
                                           joynr::util::createUuid(),
                                           joynr::util::createUuid());
        }
    }

    ParticipantIdStorage store;
    const std::string domain;
    const std::string interfaceName;
    const std::string participantId;

    std::condition_variable cv;
    std::mutex mutex;
    bool canStart;
};

// This test simulates 2 applications using libJoynr, one doing 10000 lookups and the
// other registering 1000 providers
TEST_F(ParticipantIdStorageParallelTest, checkParallel_RW_AccessOfStorage) {
    std::thread read (&ParticipantIdStorageParallelTest::readAfterNotify, this, 10000);
    std::thread write (&ParticipantIdStorageParallelTest::writeWithNotify, this, 1000);

    read.join();
    write.join();
}

// This test simulates 2 applications using libJoynr, each doing 10000 lookups
TEST_F(ParticipantIdStorageParallelTest, checkParallel_R_AccessOfStorage) {
    std::thread read_1 (&ParticipantIdStorageParallelTest::readAfterNotify, this, 10000);
    std::thread read_2 (&ParticipantIdStorageParallelTest::readAfterNotify, this, 10000);

    writeOneEntryAndNotify();

    read_1.join();
    read_2.join();
}

// This test simulates 2 applications using libJoynr, each registering 1000 providers
TEST_F(ParticipantIdStorageParallelTest, checkParallel_W_AccessOfStorage) {
    std::thread write_1 (&ParticipantIdStorageParallelTest::writeAfterNotify, this, 1000);
    std::thread write_2 (&ParticipantIdStorageParallelTest::writeAfterNotify, this, 1000);

    writeOneEntryAndNotify();

    write_1.join();
    write_2.join();
}

// This test simulates 10 applications using libJoynr, each registering 100 providers and doing
// 10 lookups in the storage.
TEST_F(ParticipantIdStorageParallelTest, checkParallel_RW_AccessOfStorage_multipleThreads) {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
      threads.push_back(std::thread(&ParticipantIdStorageParallelTest::readAfterNotify, this, 10));
      threads.push_back(std::thread(&ParticipantIdStorageParallelTest::writeAfterNotify, this, 100));
    }

    writeOneEntryAndNotify();

    for (auto& t : threads) {
      t.join();
    }
}
