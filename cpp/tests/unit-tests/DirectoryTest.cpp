/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

using namespace joynr;

class DirectoryTest : public ::testing::Test
{
    public:
    DirectoryTest()
        : directory(nullptr),
          testValue(nullptr),
          secondTestValue(nullptr),
          firstKey(""),
          secondKey("")
    {

    }


    void SetUp(){
        directory = new Directory<std::string, std::string>("Directory");
        testValue = std::shared_ptr<std::string>(new std::string("testValue"));
        secondTestValue = std::shared_ptr<std::string>(new std::string("secondTestValue"));
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
    directory->add(firstKey, std::shared_ptr<std::string>(new std::string("scheduledRemove_testValue")),100);
    ASSERT_TRUE(directory->contains(firstKey));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_FALSE(directory->contains(firstKey));
}

TEST(UnfixturedDirectoryTest, QSPObjectsAreDeletedByDirectoryAfterTtl)
{
    Directory<std::string, TrackableObject> *directory = new Directory<std::string, TrackableObject>("Directory");
    {
        std::shared_ptr<TrackableObject> tp = std::shared_ptr<TrackableObject>(new TrackableObject());
        ASSERT_EQ(TrackableObject::getInstances(), 1);
        directory->add("key", tp, 100);
    }
    ASSERT_EQ(TrackableObject::getInstances(), 1) << "Directory copied / deleted object";
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_EQ(TrackableObject::getInstances(), 0) << "Directory did not delete Object";
    delete directory;
}

TEST(UnfixturedDirectoryTest, ObjectsAreDeletedIfDirectoryIsDeleted)
{
    Directory<std::string, TrackableObject> *directory = new Directory<std::string, TrackableObject>("Directory");
    {
        std::shared_ptr<TrackableObject> tp = std::shared_ptr<TrackableObject>(new TrackableObject());
        ASSERT_EQ(TrackableObject::getInstances(), 1);
        directory->add("key", tp, 100);
    }
    delete directory;
    ASSERT_EQ(TrackableObject::getInstances(), 0) << "Directory did not delete Object";
}

TEST(UnfixturedDirectoryTest, useStdStringKeys)
{
    std::string key = "key";
    std::shared_ptr<std::string> value(new std::string("value"));
    Directory<std::string, std::string> directory("Directory");
    ASSERT_FALSE(directory.contains(key)) << "Empty directory contains entry.";
    directory.add(key, value);
    ASSERT_TRUE(directory.contains(key));
    ASSERT_EQ(value, directory.lookup(key));
    directory.remove(key);
    ASSERT_FALSE(directory.contains(key));
}
