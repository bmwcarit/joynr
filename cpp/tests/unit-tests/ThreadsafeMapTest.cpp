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
#include "joynr/PrivateCopyAssign.h"
#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"
#include "joynr/ThreadSafeMap.h"

#include <string>
#include <memory>

using ::testing::Property;
using ::testing::Eq;
using ::testing::ByRef;
using ::testing::_;
using namespace ::testing;
using namespace joynr;

class ThreadsafeMapTest : public ::testing::Test
{
public:
    ThreadsafeMapTest()
            : map(), testValue(nullptr), secondTestValue(nullptr), firstKey(""), secondKey("")
    {
    }

    void SetUp()
    {
        testValue = std::make_shared<std::string>("testValue");
        secondTestValue = std::make_shared<std::string>("secondTestValue");
        firstKey = std::string("firstKey");
        secondKey = std::string("secondKey");
    }

protected:
    ThreadSafeMap<std::string, std::shared_ptr<std::string>> map;
    std::shared_ptr<std::string> testValue;
    std::shared_ptr<std::string> secondTestValue;
    std::string firstKey;
    std::string secondKey;

private:
    DISALLOW_COPY_AND_ASSIGN(ThreadsafeMapTest);
};

TEST_F(ThreadsafeMapTest, insertAndContains)
{
    map.insert(firstKey, testValue);
    ASSERT_TRUE(map.contains(firstKey));
    map.insert(secondKey, testValue);
    ASSERT_TRUE(map.contains(secondKey));
}

TEST_F(ThreadsafeMapTest, containsNot)
{
    map.insert(firstKey, testValue);
    ASSERT_TRUE(map.contains(firstKey));
    ASSERT_FALSE(map.contains(secondKey));
}

TEST_F(ThreadsafeMapTest, value)
{
    map.insert(firstKey, testValue);
    map.insert(secondKey, secondTestValue);
    std::shared_ptr<std::string> result1 = map.value(firstKey);
    std::shared_ptr<std::string> result2 = map.value(secondKey);
    ASSERT_EQ(result1, testValue);
    ASSERT_EQ(result2, secondTestValue);
}

TEST_F(ThreadsafeMapTest, remove)
{
    map.insert(firstKey, testValue);
    map.insert(secondKey, secondTestValue);
    ASSERT_TRUE(map.contains(firstKey));
    ASSERT_TRUE(map.contains(secondKey));
    map.remove(firstKey);
    ASSERT_FALSE(map.contains(firstKey));
    ASSERT_TRUE(map.contains(secondKey));
}

TEST_F(ThreadsafeMapTest, take)
{
    map.insert(firstKey, testValue);
    map.insert(secondKey, secondTestValue);
    ASSERT_TRUE(map.contains(firstKey));
    ASSERT_TRUE(map.contains(secondKey));
    map.take(firstKey);
    ASSERT_FALSE(map.contains(firstKey));
    ASSERT_TRUE(map.contains(secondKey));
}
