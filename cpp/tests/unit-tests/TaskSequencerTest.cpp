/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/TaskSequencer.h"

using namespace ::testing;
using namespace joynr;

class TaskSequencerTest : public ::testing::Test
{
public:
    TaskSequencerTest() : _creationCounter{0}, _executionCounter{0}
    {
    }

protected:
    class TestFuture : public Future<std::uint64_t>
    {
    public:
        std::shared_ptr<std::uint64_t> _memorySharedByPromise;
    };

    using TestTaskSequencer = TaskSequencer<std::uint64_t>;

    struct TaskState
    {
        std::uint64_t creationNo;
        std::uint64_t executionNo;

        int operator==(const TaskState& other) const
        {
            return (creationNo == other.creationNo) && (executionNo == other.executionNo);
        }
    };

    TestTaskSequencer::Task createTestTask(bool fulfillPromise = true)
    {
        auto creationId = _creationCounter;
        _creationCounter++;
        return std::bind(&TaskSequencerTest::createTestFuture, this, creationId, fulfillPromise);
    }

    std::shared_ptr<TestFuture> createTestFuture(const std::uint64_t& creationId,
                                                 bool fulfillPromise = true)
    {
        auto newFuture = std::make_shared<TestFuture>();
        if (fulfillPromise) {
            newFuture->onSuccess(creationId);
        }
        std::unique_lock<std::mutex> lock(_sequenceMutex);
        _sequence.push_back(TaskState{creationId, _executionCounter});
        _executionCounter++;
        _sequenceChanged.notify_all();
        return newFuture;
    }

    std::vector<TaskState> waitForTestFutureCreation(
            const std::size_t& expectedNumber,
            const std::chrono::seconds& waitDuration = std::chrono::seconds{10})
    {
        std::unique_lock<std::mutex> lock(_sequenceMutex);
        if (_sequence.size() > expectedNumber) {
            return _sequence;
        }
        _sequenceChanged.wait_for(lock, waitDuration, [this, expectedNumber] {
            return expectedNumber < _sequence.size();
        });
        return _sequence;
    }

    std::uint64_t _creationCounter;
    std::uint64_t _executionCounter;

    std::mutex _sequenceMutex;
    std::condition_variable _sequenceChanged;
    std::vector<TaskState> _sequence;
};

TEST_F(TaskSequencerTest, CtorDtor)
{
    std::unique_ptr<TestTaskSequencer> instance(new TestTaskSequencer());
    std::this_thread::yield(); // Wait for TestTaskSequencer orchestration thread
    auto callDtor = std::async([&] { instance.reset(); });
    ASSERT_EQ(callDtor.wait_for(std::chrono::seconds(10)), std::future_status::ready)
            << "DTOR hangs forever";
}

TEST_F(TaskSequencerTest, addSequence)
{
    TestTaskSequencer test;
    static constexpr std::uint64_t numberOfTasks = 1000;
    std::vector<TaskState> expectedSequence;
    for (std::uint64_t i = 0; i < numberOfTasks; i++) {
        test.add(createTestTask());
        expectedSequence.push_back({i, i});
    }
    const auto sequence = waitForTestFutureCreation(expectedSequence.size());
    EXPECT_THAT(sequence, ::testing::ContainerEq(expectedSequence));
}

TEST_F(TaskSequencerTest, addConcurrency)
{
    TestTaskSequencer test;
    static constexpr std::uint64_t numberOfTasks = 1000;
    std::vector<TestTaskSequencer::Task> tasks;
    std::set<std::uint64_t> expectedExecutions;
    for (std::uint64_t i = 0; i < numberOfTasks; i++) {
        tasks.push_back(createTestTask());
        expectedExecutions.insert(i);
    }
    std::vector<std::future<void>> addAsync;
    for (auto task : tasks) {
        addAsync.push_back(std::async([&test, task]() mutable { test.add(std::move(task)); }));
    }
    addAsync.clear();
    const auto sequence = waitForTestFutureCreation(expectedExecutions.size());
    std::set<std::uint64_t> executions;
    for (auto taskState : sequence) {
        executions.insert(taskState.executionNo);
    }
    EXPECT_THAT(executions, ::testing::ContainerEq(expectedExecutions));
}

TEST_F(TaskSequencerTest, cancelBeforeDtor)
{
    std::unique_ptr<TestTaskSequencer> instance(new TestTaskSequencer());
    std::this_thread::yield(); // Wait for TestTaskSequencer orchestration thread
    instance->cancel();
    auto callDtor = std::async([&] { instance.reset(); });
    ASSERT_EQ(callDtor.wait_for(std::chrono::seconds(10)), std::future_status::ready)
            << "DTOR hangs forever after cancel";
}

TEST_F(TaskSequencerTest, cancelOngoingTask)
{
    TestTaskSequencer test;
    test.add(createTestTask());
    test.add(createTestTask(false));
    test.add(createTestTask());
    auto sequence = waitForTestFutureCreation(2);
    EXPECT_EQ(2, sequence.size());
    test.cancel();
    sequence = waitForTestFutureCreation(2);
    EXPECT_EQ(2, sequence.size());
}

TEST_F(TaskSequencerTest, cancelReleasesTaskMemory)
{
    TestTaskSequencer test;
    auto sharedPromise = std::make_shared<std::uint64_t>(42);
    std::weak_ptr<std::uint64_t> refPromise(sharedPromise);
    test.add(createTestTask(false)); // Block execution, so next task remains in task queue
    test.add([this, sharedPromise]() { return createTestFuture(*sharedPromise); });
    sharedPromise.reset();
    std::this_thread::yield();
    EXPECT_FALSE(refPromise.expired()) << "Test setup does not capture memory in task queue.";
    test.cancel();
    EXPECT_TRUE(refPromise.expired());
}

TEST_F(TaskSequencerTest, cancelReleasesFutureMemory)
{
    TestTaskSequencer test;
    auto sharedPromise = std::make_shared<std::uint64_t>(42);
    std::weak_ptr<std::uint64_t> refPromise(sharedPromise);
    test.add([this, sharedPromise]() {
        auto newFuture = std::make_shared<TestFuture>();
        newFuture->_memorySharedByPromise = sharedPromise;
        return newFuture;
    });
    sharedPromise.reset();
    std::this_thread::yield();
    EXPECT_FALSE(refPromise.expired()) << "Test setup does not capture memory in future";
    test.cancel();
    EXPECT_TRUE(refPromise.expired());
}

TEST_F(TaskSequencerTest, runRobustness)
{
    TestTaskSequencer test;
    test.add([]() { return std::shared_ptr<TestFuture>(); });
    test.add([]() -> std::shared_ptr<TestFuture> { throw 42; });
    test.add(nullptr);
    test.add(createTestTask());
    auto sequence = waitForTestFutureCreation(1);
    EXPECT_EQ(1, sequence.size());
}