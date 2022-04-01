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

#include <atomic>

#include <gtest/gtest.h>

#include "joynr/Directory.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/SingleThreadedIOService.h"

using namespace joynr;

class TrackableObject
{
public:
    TrackableObject()
    {
        ++instances;
        JOYNR_LOG_TRACE(logger(),
                        "Creating TrackableObject at address {}. Now we have {} instances.",
                        static_cast<const void*>(this),
                        instances);
    }

    ~TrackableObject()
    {
        --instances;
        JOYNR_LOG_TRACE(logger(),
                        "Deleting TrackableObject at address {}. Now we have {} instances.",
                        static_cast<const void*>(this),
                        instances);
    }

    static int getInstances()
    {
        return instances;
    }

private:
    static std::atomic_int instances;
    ADD_LOGGER(TrackableObject)
};

std::atomic_int TrackableObject::instances;

template <typename Key, typename T>
class TestDirectory : public Directory<Key, T>
{
public:
    TestDirectory(const std::string& directoryName, boost::asio::io_service& ioService)
        : Directory<Key, T>(directoryName, ioService)
    {
    }

    ~TestDirectory() = default;

    bool containsTimer(const Key& keyId)
    {
        std::lock_guard<std::mutex> lock(Directory<Key, T>::_mutex);
        return Directory<Key, T>::_timeoutTimerMap.find(keyId) != Directory<Key, T>::_timeoutTimerMap.cend();
    }
};

class DirectoryTest : public ::testing::Test
{
public:
    DirectoryTest()
            : _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _directory("Directory", _singleThreadedIOService->getIOService()),
              _testDirectory("TestDirectory", _singleThreadedIOService->getIOService()),
              _testValue(nullptr),
              _secondTestValue(nullptr),
              _firstKey(""),
              _secondKey("")
    {
        _singleThreadedIOService->start();
    }

    ~DirectoryTest()
    {
        _singleThreadedIOService->stop();
    }

    void SetUp()
    {
        _testValue = std::make_shared<std::string>("testValue");
        _secondTestValue = std::make_shared<std::string>("secondTestValue");
        _firstKey = std::string("firstKey");
        _secondKey = std::string("secondKey");
    }

protected:
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    Directory<std::string, std::string> _directory;
    TestDirectory<std::string, std::string> _testDirectory;
    std::shared_ptr<std::string> _testValue;
    std::shared_ptr<std::string> _secondTestValue;
    std::string _firstKey;
    std::string _secondKey;

private:
    DISALLOW_COPY_AND_ASSIGN(DirectoryTest);
};

TEST_F(DirectoryTest, addAndContains)
{
    _directory.add(_firstKey, _testValue);
    ASSERT_TRUE(_directory.contains(_firstKey));
}

TEST_F(DirectoryTest, containsNot)
{
    _directory.add(_firstKey, _testValue);
    ASSERT_TRUE(_directory.contains(_firstKey));
    ASSERT_FALSE(_directory.contains(_secondKey));
}

TEST_F(DirectoryTest, lookup)
{
    _directory.add(_firstKey, _testValue);
    _directory.add(_secondKey, _secondTestValue);
    std::shared_ptr<std::string> result1 = _directory.lookup(_firstKey);
    std::shared_ptr<std::string> result2 = _directory.lookup(_secondKey);
    ASSERT_EQ(result1, _testValue);
    ASSERT_EQ(result2, _secondTestValue);
}

TEST_F(DirectoryTest, remove)
{
    _directory.add(_firstKey, _testValue);
    _directory.add(_secondKey, _secondTestValue);
    ASSERT_TRUE(_directory.contains(_firstKey));
    ASSERT_TRUE(_directory.contains(_secondKey));
    _directory.remove(_firstKey);
    ASSERT_FALSE(_directory.contains(_firstKey));
    ASSERT_TRUE(_directory.contains(_secondKey));
}

TEST_F(DirectoryTest, scheduledRemove)
{
    _directory.add(_firstKey, std::make_shared<std::string>("scheduledRemove_testValue"), 100);
    ASSERT_TRUE(_directory.contains(_firstKey));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_FALSE(_directory.contains(_firstKey));
}

TEST_F(DirectoryTest, ObjectsAreDeletedByDirectoryAfterTtl)
{
    Directory<std::string, TrackableObject> directory(
            "Directory", _singleThreadedIOService->getIOService());
    {
        auto tp = std::make_shared<TrackableObject>();
        ASSERT_EQ(TrackableObject::getInstances(), 1);
        directory.add("key", tp, 100);
    }
    ASSERT_EQ(TrackableObject::getInstances(), 1) << "Directory copied / deleted object";
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_EQ(TrackableObject::getInstances(), 0) << "Directory did not delete Object";
}

TEST_F(DirectoryTest, ObjectsAreDeletedIfDirectoryIsDeleted)
{
    {
        Directory<std::string, TrackableObject> directory(
                "Directory", _singleThreadedIOService->getIOService());
        auto tp = std::make_shared<TrackableObject>();
        ASSERT_EQ(TrackableObject::getInstances(), 1);
        directory.add("key", tp, 100);
    }
    ASSERT_EQ(TrackableObject::getInstances(), 0) << "Directory did not delete Object";
}

TEST_F(DirectoryTest, useStdStringKeys)
{
    std::string key = "key";
    auto value = std::make_shared<std::string>("value");
    Directory<std::string, std::string> directory(
            "Directory", _singleThreadedIOService->getIOService());
    ASSERT_FALSE(directory.contains(key)) << "Empty _directory contains entry.";
    directory.add(key, value);
    ASSERT_TRUE(directory.contains(key));
    ASSERT_EQ(value, directory.lookup(key));
    directory.remove(key);
    ASSERT_FALSE(directory.contains(key));
}

TEST_F(DirectoryTest, lookupNonExistingKeys)
{
    ASSERT_TRUE(nullptr == _directory.lookup("__THIS__KEY__DOES__NOT__EXIST__"));
}

TEST_F(DirectoryTest, useLastTTLForKey)
{
    auto value = std::make_shared<std::string>("value");
    std::string key("key");

    _directory.add(key, value, 100000); // Won't be removed after 50 ms (see sleep_for below)
    _directory.add(key, value, 10);     // Will be removed after 50 ms

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    ASSERT_FALSE(_directory.contains(key));
}

TEST_F(DirectoryTest, take)
{
    _directory.add(_firstKey, _testValue);
    _directory.add(_secondKey, _secondTestValue);

    // both keys exist
    ASSERT_EQ(_directory.lookup(_firstKey), _testValue);
    ASSERT_EQ(_directory.lookup(_secondKey), _secondTestValue);

    // remove key from the directory with take()
    ASSERT_EQ(_directory.take(_secondKey), _secondTestValue);

    // only one key is left in the directory
    ASSERT_EQ(_directory.lookup(_firstKey), _testValue);
    ASSERT_FALSE(_directory.lookup(_secondKey));
}

TEST_F(DirectoryTest, takeDeletesTimer)
{
    _testDirectory.add(_firstKey, _testValue, 100000);
    _testDirectory.add(_secondKey, _secondTestValue, 100000);

    // both keys exist
    ASSERT_TRUE(_testDirectory.contains(_firstKey));
    ASSERT_TRUE(_testDirectory.contains(_secondKey));

    // both timers exist
    ASSERT_TRUE(_testDirectory.containsTimer(_firstKey));
    ASSERT_TRUE(_testDirectory.containsTimer(_secondKey));

    // remove key from the directory with take()
    ASSERT_EQ(_testDirectory.take(_secondKey), _secondTestValue);

    // only one key is left in the directory
    ASSERT_EQ(_testDirectory.lookup(_firstKey), _testValue);
    ASSERT_FALSE(_testDirectory.lookup(_secondKey));

    // only one timer is left in the directory
    ASSERT_TRUE(_testDirectory.containsTimer(_firstKey));
    ASSERT_FALSE(_testDirectory.containsTimer(_secondKey));
}

TEST_F(DirectoryTest, removeDeletesTimer)
{
    _testDirectory.add(_firstKey, _testValue, 100000);
    _testDirectory.add(_secondKey, _secondTestValue, 100000);

    // both keys exist
    ASSERT_TRUE(_testDirectory.contains(_firstKey));
    ASSERT_TRUE(_testDirectory.contains(_secondKey));

    // both timers exist
    ASSERT_TRUE(_testDirectory.containsTimer(_firstKey));
    ASSERT_TRUE(_testDirectory.containsTimer(_secondKey));

    // remove key from the directory with remove()
    _testDirectory.remove(_firstKey);

    // only one key is left in the directory
    ASSERT_FALSE(_testDirectory.contains(_firstKey));
    ASSERT_TRUE(_testDirectory.contains(_secondKey));

    // only one timer is left in the directory
    ASSERT_FALSE(_testDirectory.containsTimer(_firstKey));
    ASSERT_TRUE(_testDirectory.containsTimer(_secondKey));
}
