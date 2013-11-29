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
#include "gmock/gmock.h"
#include "joynr/ThreadSafeMap.h"
#include <QSharedPointer>

using ::testing::Property;
using ::testing::Eq;
using ::testing::ByRef;
using ::testing::NotNull;
using ::testing::_;
using namespace ::testing;
using namespace joynr;

class ThreadsafeMapTest : public ::testing::Test
{
    public:
    ThreadsafeMapTest()
        : map(NULL),
          testValue(NULL),
          secondTestValue(NULL),
          firstKey(""),
          secondKey(""){
    }

    void SetUp(){
        map = new ThreadSafeMap<QString, QSharedPointer<QString> >();
        testValue = QSharedPointer<QString>(new QString("testValue"));
        secondTestValue = QSharedPointer<QString>(new QString("secondTestValue"));
        firstKey = QString("firstKey");
        secondKey = QString("secondKey");
    }
    void TearDown(){
        delete map;
    }

protected:
    ThreadSafeMap<QString, QSharedPointer<QString> >* map;
    QSharedPointer<QString> testValue;
    QSharedPointer<QString> secondTestValue;
    QString firstKey;
    QString secondKey;
private:
    DISALLOW_COPY_AND_ASSIGN(ThreadsafeMapTest);
};


TEST_F(ThreadsafeMapTest, insertAndContains)
{
    map->insert(firstKey, testValue);
    ASSERT_TRUE(map->contains(firstKey));
    map->insert(secondKey, testValue);
    ASSERT_TRUE(map->contains(secondKey));
}

TEST_F(ThreadsafeMapTest, containsNot)
{
    map->insert(firstKey, testValue);
    ASSERT_TRUE(map->contains(firstKey));
    ASSERT_FALSE(map->contains(secondKey));
}

TEST_F(ThreadsafeMapTest, value)
{
    map->insert(firstKey, testValue);
    map->insert(secondKey,secondTestValue);
    QSharedPointer<QString> result1 = map->value(firstKey);
    QSharedPointer<QString> result2 = map->value(secondKey);
    ASSERT_EQ(result1, testValue);
    ASSERT_EQ(result2, secondTestValue);
}

TEST_F(ThreadsafeMapTest, remove)
{
    map->insert(firstKey, testValue);
    map->insert(secondKey, secondTestValue);
    ASSERT_TRUE(map->contains(firstKey));
    ASSERT_TRUE(map->contains(secondKey));
    map->remove(firstKey);
    ASSERT_FALSE(map->contains(firstKey));
    ASSERT_TRUE(map->contains(secondKey));
}

TEST_F(ThreadsafeMapTest, take)
{
    map->insert(firstKey, testValue);
    map->insert(secondKey, secondTestValue);
    ASSERT_TRUE(map->contains(firstKey));
    ASSERT_TRUE(map->contains(secondKey));
    map->take(firstKey);
    ASSERT_FALSE(map->contains(firstKey));
    ASSERT_TRUE(map->contains(secondKey));
}





