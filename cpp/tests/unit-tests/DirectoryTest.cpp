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
#include "gtest/gtest.h"
#include "joynr/Directory.h"
#include "joynr/TrackableObject.h"
#include "joynr/ThreadUtil.h"

using namespace joynr;

class DirectoryTest : public ::testing::Test
{
    public:
    DirectoryTest()
        : directory(NULL),
          testValue(NULL),
          secondTestValue(NULL),
          firstKey(""),
          secondKey("")
    {

    }


    void SetUp(){
        directory = new Directory<QString, QString>("Directory");
        testValue = std::shared_ptr<QString>(new QString("testValue"));
        secondTestValue = std::shared_ptr<QString>(new QString("secondTestValue"));
        firstKey = QString("firstKey");
        secondKey = QString("secondKey");
    }
    void TearDown(){
        delete directory;
    }

protected:
    Directory<QString, QString>* directory;
    std::shared_ptr<QString> testValue;
    std::shared_ptr<QString> secondTestValue;
    QString firstKey;
    QString secondKey;
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
    std::shared_ptr<QString> result1 = directory->lookup(firstKey);
    std::shared_ptr<QString> result2 = directory->lookup(secondKey);
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
    directory->add(firstKey, std::shared_ptr<QString>(new QString("scheduledRemove_testValue")),100);
    ASSERT_TRUE(directory->contains(firstKey));
    ThreadUtil::sleepForMillis(200);
    ASSERT_FALSE(directory->contains(firstKey));
}


TEST(UnfixturedDirectoryTest, ObjectsAreDeletedByDirectoryAfterTtl)
{
    Directory<QString, TrackableObject> *directory = new Directory<QString, TrackableObject>("Directory");
    TrackableObject *t1 = new TrackableObject();
    ASSERT_EQ(TrackableObject::getInstances(), 1);
    directory->add("key", t1, 100);
    ASSERT_EQ(TrackableObject::getInstances(), 1) << "Directory copied / deleted object";
    ThreadUtil::sleepForMillis(200);
    ASSERT_EQ(TrackableObject::getInstances(), 0) << "Directory did not delete Object";
    delete directory;
}

TEST(UnfixturedDirectoryTest, ObjectsAreDeletedWhenDirectoryIsDeleted)
{
    Directory<QString, TrackableObject> *directory = new Directory<QString, TrackableObject>("Directory");
    TrackableObject *t1 = new TrackableObject();
    ASSERT_EQ(TrackableObject::getInstances(), 1);
    directory->add("key", t1, 100);
    ASSERT_EQ(TrackableObject::getInstances(), 1) << "Directory copied / deleted object";
    delete directory;
    ASSERT_EQ(TrackableObject::getInstances(), 0) << "Directory did not delete Object when it was deleted";
}

TEST(UnfixturedDirectoryTest, QSPObjectsAreDeletedByDirectoryAfterTtl)
{
    Directory<QString, TrackableObject> *directory = new Directory<QString, TrackableObject>("Directory");
    {
        std::shared_ptr<TrackableObject> tp = std::shared_ptr<TrackableObject>(new TrackableObject());
        ASSERT_EQ(TrackableObject::getInstances(), 1);
        directory->add("key", tp, 100);
    }
    ASSERT_EQ(TrackableObject::getInstances(), 1) << "Directory copied / deleted object";
    ThreadUtil::sleepForMillis(200);
    ASSERT_EQ(TrackableObject::getInstances(), 0) << "Directory did not delete Object";
    delete directory;
}

TEST(UnfixturedDirectoryTest, QSPObjectsAreDeletedIfDirectoryIsDeleted)
{
    Directory<QString, TrackableObject> *directory = new Directory<QString, TrackableObject>("Directory");
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
    std::shared_ptr<QString> value(new QString("value"));
    Directory<std::string, QString> directory("Directory");
    ASSERT_FALSE(directory.contains(key)) << "Empty directory contains entry.";
    directory.add(key, value);
    ASSERT_TRUE(directory.contains(key));
    ASSERT_EQ(value, directory.lookup(key));
    directory.remove(key);
    ASSERT_FALSE(directory.contains(key));
}
