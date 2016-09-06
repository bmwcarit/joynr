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
#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include "joynr/Directory.h"
#include "joynr/TrackableObject.h"
#include "joynr/SingleThreadedIOService.h"

using namespace joynr;

class DirectoryTest : public ::testing::Test
{
    public:
    DirectoryTest()
        : directory(nullptr),
          testValue(nullptr),
          secondTestValue(nullptr),
          firstKey(""),
          secondKey(""),
          singleThreadedIOService()
    {

    }


    void SetUp(){
        directory = new Directory<std::string, std::string>("Directory", singleThreadedIOService.getIOService());
        testValue = std::make_shared<std::string>("testValue");
        secondTestValue = std::make_shared<std::string>("secondTestValue");
        firstKey = std::string("firstKey");
        secondKey = std::string("secondKey");
    }
    void TearDown(){
        delete directory;
    }

protected:
    Directory<std::string, std::string>* directory;
    std::shared_ptr<std::string> testValue;
    std::shared_ptr<std::string> secondTestValue;
    std::string firstKey;
    std::string secondKey;
    SingleThreadedIOService singleThreadedIOService;
private:
    DISALLOW_COPY_AND_ASSIGN(DirectoryTest);
};

TEST_F(DirectoryTest, addAndContains)
{
    directory->add(firstKey, testValue);
    ASSERT_TRUE(directory->contains(firstKey));
}

TEST_F(DirectoryTest, containsNot)
{
    directory->add(firstKey, testValue);
    ASSERT_TRUE(directory->contains(firstKey));
    ASSERT_FALSE(directory->contains(secondKey));
}

TEST_F(DirectoryTest, lookup)
{
    directory->add(firstKey, testValue);
    directory->add(secondKey,secondTestValue);
    std::shared_ptr<std::string> result1 = directory->lookup(firstKey);
    std::shared_ptr<std::string> result2 = directory->lookup(secondKey);
    ASSERT_EQ(result1, testValue);
    ASSERT_EQ(result2, secondTestValue);
}

TEST_F(DirectoryTest, remove)
{
    directory->add(firstKey, testValue);
    directory->add(secondKey, secondTestValue);
    ASSERT_TRUE(directory->contains(firstKey));
    ASSERT_TRUE(directory->contains(secondKey));
    directory->remove(firstKey);
    ASSERT_FALSE(directory->contains(firstKey));
    ASSERT_TRUE(directory->contains(secondKey));
}

TEST_F(DirectoryTest, scheduledRemove)
{
    directory->add(firstKey, std::make_shared<std::string>("scheduledRemove_testValue"), 100);
    ASSERT_TRUE(directory->contains(firstKey));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_FALSE(directory->contains(firstKey));
}

TEST_F(DirectoryTest, ObjectsAreDeletedByDirectoryAfterTtl)
{
    Directory<std::string, TrackableObject> *directory = new Directory<std::string, TrackableObject>("Directory",  singleThreadedIOService.getIOService());
    {
        auto tp = std::make_shared<TrackableObject>();
        ASSERT_EQ(TrackableObject::getInstances(), 1);
        directory->add("key", tp, 100);
    }
    ASSERT_EQ(TrackableObject::getInstances(), 1) << "Directory copied / deleted object";
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_EQ(TrackableObject::getInstances(), 0) << "Directory did not delete Object";
    delete directory;
}

TEST_F(DirectoryTest, ObjectsAreDeletedIfDirectoryIsDeleted)
{
    Directory<std::string, TrackableObject> *directory = new Directory<std::string, TrackableObject>("Directory",  singleThreadedIOService.getIOService());
    {
        auto tp = std::make_shared<TrackableObject>();
        ASSERT_EQ(TrackableObject::getInstances(), 1);
        directory->add("key", tp, 100);
    }
    delete directory;
    ASSERT_EQ(TrackableObject::getInstances(), 0) << "Directory did not delete Object";
}

TEST_F(DirectoryTest, useStdStringKeys)
{
    std::string key = "key";
    auto value = std::make_shared<std::string>("value");
    Directory<std::string, std::string> directory("Directory",  singleThreadedIOService.getIOService());
    ASSERT_FALSE(directory.contains(key)) << "Empty directory contains entry.";
    directory.add(key, value);
    ASSERT_TRUE(directory.contains(key));
    ASSERT_EQ(value, directory.lookup(key));
    directory.remove(key);
    ASSERT_FALSE(directory.contains(key));
}

TEST_F(DirectoryTest, lookupNonExisingKeys)
{
    ASSERT_TRUE(nullptr == directory->lookup("__THIS__KEY__DOES__NOT__EXIST__"));
}

TEST_F(DirectoryTest, useLastTTLForKey)
{
    Directory<std::string, std::string> directory("Directory",  singleThreadedIOService.getIOService());
    auto value = std::make_shared<std::string>("value");
    std::string key("key");

    directory.add(key, value, 100000); // Won't be removed after 50 ms (see sleep_for below)
    directory.add(key, value, 10);     // Will be removed after 50 ms

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    ASSERT_FALSE(directory.contains(key));
}
