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

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/TaskSequencer.h"
#include "joynr/TimePoint.h"

using namespace ::testing;
using namespace joynr;

class TaskSequencerTest : public ::testing::Test
{
public:
    TaskSequencerTest() : _creationCounter{0}, _executionCounter{0}, _actualTimeoutDateMs(), _fulfillDelayedFuture(false)
    {
    }

protected:
    void SetUp() override
    {
        std::unique_lock<std::mutex> lock(_actualTimeoutDateMsMutex);
        _actualTimeoutDateMs.clear();
        _fulfillDelayedFuture = false;
    }

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

    TestTaskSequencer::TaskWithExpiryDate createTestTaskWithExpiryDate(const TimePoint& expiryDate, bool fulfillPromise = true,
                                                                       bool fulfillDelayed = false) {
        TestTaskSequencer::TaskWithExpiryDate timeoutTask;
        timeoutTask._task = createTestTask(fulfillPromise, fulfillDelayed);
        timeoutTask._expiryDate = expiryDate;
        timeoutTask._timeout = [&](){
            std::unique_lock<std::mutex> lock(_actualTimeoutDateMsMutex);
            _actualTimeoutDateMs.push_back(TimePoint::now().toMilliseconds());
        };
        return timeoutTask;
    }

    TestTaskSequencer::Task createTestTask(bool fulfillPromise = true, bool fulfillDelayed = false)
    {
        auto creationId = _creationCounter;
        _creationCounter++;
        return std::bind(&TaskSequencerTest::createTestFuture, this, creationId, fulfillPromise, fulfillDelayed);
    }

    std::shared_ptr<TestFuture> createTestFuture(const std::uint64_t& creationId,
                                                 bool fulfillPromise = true,
                                                 bool fulfillDelayed = false)
    {
        auto newFuture = std::make_shared<TestFuture>();
        if (fulfillPromise) {
            if (fulfillDelayed) {
                _fulfillFuture = std::async([this, newFuture, creationId](){
                    while (!_fulfillDelayedFuture) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                    newFuture->onSuccess(creationId);
                });
            } else {
                newFuture->onSuccess(creationId);
            }
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
        if (_sequence.size() >= expectedNumber) {
            return _sequence;
        }
        _sequenceChanged.wait_for(lock, waitDuration, [this, expectedNumber] {
            return expectedNumber <= _sequence.size();
        });
        return _sequence;
    }

    void waitForTimeouts(size_t expectedNumberOfTimeouts, TimePoint expectedTimeout);

    std::uint64_t _creationCounter;
    std::uint64_t _executionCounter;

    std::mutex _sequenceMutex;
    std::condition_variable _sequenceChanged;
    std::vector<TaskState> _sequence;

    std::vector<std::int64_t> _actualTimeoutDateMs;
    std::mutex _actualTimeoutDateMsMutex;

    std::atomic<bool> _fulfillDelayedFuture;
    std::future<void> _fulfillFuture;
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
        test.add(createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(10000)));
        expectedSequence.push_back({i, i});
    }
    const auto sequence = waitForTestFutureCreation(expectedSequence.size());
    EXPECT_THAT(sequence, ::testing::ContainerEq(expectedSequence));
}

TEST_F(TaskSequencerTest, addConcurrency)
{
    TestTaskSequencer test;
    static constexpr std::uint64_t numberOfTasks = 1000;
    std::vector<TestTaskSequencer::TaskWithExpiryDate> tasks;
    std::set<std::uint64_t> expectedExecutions;
    for (std::uint64_t i = 0; i < numberOfTasks; i++) {
        tasks.push_back(createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(10000)));
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
    test.add(createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(5000)));
    test.add(createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(6000), false));
    test.add(createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(8000)));
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
    test.add(createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(5000), false)); // Block execution, so next task remains in task queue
    test.add({[this, sharedPromise]() { return createTestFuture(*sharedPromise); },
              TimePoint::fromRelativeMs(10000), [](){}});
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
    test.add({[sharedPromise]() {
        auto newFuture = std::make_shared<TestFuture>();
        newFuture->_memorySharedByPromise = sharedPromise;
        return newFuture;
    }, TimePoint::fromRelativeMs(5000), [](){}});
    sharedPromise.reset();
    std::this_thread::yield();
    EXPECT_FALSE(refPromise.expired()) << "Test setup does not capture memory in future";
    test.cancel();
    EXPECT_TRUE(refPromise.expired());
}

TEST_F(TaskSequencerTest, runRobustness)
{
    TestTaskSequencer test;
    test.add({[]() { return std::shared_ptr<TestFuture>(); },
              TimePoint::fromRelativeMs(5000), [](){}});
    test.add({[]() -> std::shared_ptr<TestFuture> { throw 42; },
              TimePoint::fromRelativeMs(5000), [](){}});
    test.add({nullptr,
              TimePoint::fromRelativeMs(5000), [](){}});
    test.add(createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(5000)));
    auto sequence = waitForTestFutureCreation(1);
    EXPECT_EQ(1, sequence.size());
}

void TaskSequencerTest::waitForTimeouts(size_t expectedNumberOfTimeouts, TimePoint expectedTimeout)
{
    for (int i = 0; i < 10; i++) {
        std::unique_lock<std::mutex> lock(_actualTimeoutDateMsMutex);
        if (expectedNumberOfTimeouts == _actualTimeoutDateMs.size()) {
            break;
        }
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_EQ(expectedNumberOfTimeouts, _actualTimeoutDateMs.size());
    std::unique_lock<std::mutex> lock(_actualTimeoutDateMsMutex);
    for (size_t i = 0; i < expectedNumberOfTimeouts; i++) {
        EXPECT_TRUE(_actualTimeoutDateMs[i] - expectedTimeout.toMilliseconds() < 500);
        EXPECT_TRUE(_actualTimeoutDateMs[i] >= expectedTimeout.toMilliseconds());
    }
}

TEST_F(TaskSequencerTest, taskTimeoutCalledForQueuedTasksWhenOtherTaskIsRunning)
{
    // This test checks that starting a long running task does not delay the cancellation
    // of expired tasks that are already queued
    TestTaskSequencer test;
    TimePoint expectedTimeoutDateMs = TimePoint::fromRelativeMs(1500);
    // add a task that blocks the sequencer until the other tasks are enqueued
    TestTaskSequencer::TaskWithExpiryDate task1 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(1000), true, true);
    // add a task that won't be finished and blocks the following tasks
    TestTaskSequencer::TaskWithExpiryDate task2 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(600000), false);
    TestTaskSequencer::TaskWithExpiryDate task3 = createTestTaskWithExpiryDate(expectedTimeoutDateMs);
    TestTaskSequencer::TaskWithExpiryDate task4 = createTestTaskWithExpiryDate(expectedTimeoutDateMs);
    test.add(task1);
    auto sequence = waitForTestFutureCreation(1, std::chrono::seconds{10});
    // TaskSequencer is blocked now until the future of task1 is ready because it waits forever by default.
    test.add(task2);
    test.add(task3);
    test.add(task4);
    // fulfill future of task1
    _fulfillDelayedFuture = true;
    sequence = waitForTestFutureCreation(2, std::chrono::seconds{10});
    // TaskSequencer is now blocked in task2 that will never finish
    EXPECT_EQ(2, sequence.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // task3 and task4 are not yet expired
    std::unique_lock<std::mutex> lock(_actualTimeoutDateMsMutex);
    EXPECT_EQ(0, _actualTimeoutDateMs.size());
    lock.unlock();
    // wait for expiration of task3 and task4
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    waitForTimeouts(2, expectedTimeoutDateMs);
    // TaskSequencer is still blocked in task2
    EXPECT_EQ(2, sequence.size());
    
}

TEST_F(TaskSequencerTest, taskTimeoutCalledForNewTasksWhenOtherTaskIsRunning)
{
    // This test checks that a long running task does not delay the cancellation
    // of expired tasks that are added to the sequencer when the long running task
    // is already running
    TestTaskSequencer test(std::chrono::milliseconds(1000));
    TimePoint expectedTimeoutDateMs = TimePoint::fromRelativeMs(1500);
    // add a task that won't be finished and blocks the following tasks
    TestTaskSequencer::TaskWithExpiryDate task1 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(600000), false);
    TestTaskSequencer::TaskWithExpiryDate task2 = createTestTaskWithExpiryDate(expectedTimeoutDateMs);
    TestTaskSequencer::TaskWithExpiryDate task3 = createTestTaskWithExpiryDate(expectedTimeoutDateMs);
    test.add(task1);
    auto sequence = waitForTestFutureCreation(1, std::chrono::seconds{10});
    // TaskSequencer is blocked now in task1 that will never finish
    test.add(task2);
    test.add(task3);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // task2 and task3 are not yet expired
    std::unique_lock<std::mutex> lock(_actualTimeoutDateMsMutex);
    EXPECT_EQ(0, _actualTimeoutDateMs.size());
    lock.unlock();
    // wait for expiration of task2 and task3
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    waitForTimeouts(2, expectedTimeoutDateMs);
    // TaskSequencer is still blocked in task1
    EXPECT_EQ(1, sequence.size());
}

TEST_F(TaskSequencerTest, testTaskTimeoutNotCalledWithoutExpiredTasks)
{
    TestTaskSequencer test(std::chrono::milliseconds(1000));
    TestTaskSequencer::TaskWithExpiryDate task1 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(500));
    // add a task that won't be finished and blocks the following tasks
    TestTaskSequencer::TaskWithExpiryDate task2 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(1000), false);
    TestTaskSequencer::TaskWithExpiryDate task3 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(600000));
    TestTaskSequencer::TaskWithExpiryDate task4 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(600000));
    test.add(task1);
    test.add(task2);
    test.add(task3);
    test.add(task4);
    // wait for the future creation for task1 and task2
    auto sequence = waitForTestFutureCreation(2, std::chrono::seconds{5});
    // wait more than timeToWait (3000 ms) of task queue
    std::this_thread::sleep_for(std::chrono::milliseconds{1500});
    EXPECT_EQ(2, sequence.size());
    // none of tasks in queue are timeout because we waited less than the expiry
    // date of the enqueued tasks
    std::unique_lock<std::mutex> lock(_actualTimeoutDateMsMutex);
    EXPECT_EQ(0, _actualTimeoutDateMs.size());
}

/**
 * Test is used to check if the first element added in empty queue
 * that already has expiryDate less than current wall time will call timeout()
 * an will be erased from queue.
 */
TEST_F(TaskSequencerTest, testTaskTimeoutCalledForTaskToBeExecutedNext)
{
    TestTaskSequencer test;
    // add already expired task to the queue
    TestTaskSequencer::TaskWithExpiryDate task1 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(-10000));
    TestTaskSequencer::TaskWithExpiryDate task2 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(1000));
    TestTaskSequencer::TaskWithExpiryDate task3 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(5000));
    // wait some time before TaskSequncer starts to run
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TimePoint expectedTimeoutDateMs = TimePoint::now();
    test.add(task1);
    test.add(task2);
    test.add(task3);
    auto sequence = waitForTestFutureCreation(2, std::chrono::seconds{10});
    EXPECT_EQ(2, sequence.size());
    // task1 is timeout before the timeToWait calculation
    std::unique_lock<std::mutex> lock(_actualTimeoutDateMsMutex);
    EXPECT_EQ(1, _actualTimeoutDateMs.size());
    EXPECT_TRUE(_actualTimeoutDateMs[0] - expectedTimeoutDateMs.toMilliseconds() < 500);
    EXPECT_TRUE(_actualTimeoutDateMs[0] >= expectedTimeoutDateMs.toMilliseconds());
}

TEST_F(TaskSequencerTest, testTaskTimeoutCalledForQueuedTasksBeforeOtherTaskIsStarted) {
    TestTaskSequencer test;
    // add a task that blocks the sequencer until the other tasks are enqueued
    TestTaskSequencer::TaskWithExpiryDate task1 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(2000), true, true);
    TestTaskSequencer::TaskWithExpiryDate task2 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(1000));
    // add task that will be removed after the future of task1 is ready, during calculation of timeToWait for task2
    TestTaskSequencer::TaskWithExpiryDate task3 = createTestTaskWithExpiryDate(TimePoint::now());
    TestTaskSequencer::TaskWithExpiryDate task4 = createTestTaskWithExpiryDate(TimePoint::fromRelativeMs(10000));
    test.add(task1);
    auto sequence = waitForTestFutureCreation(1, std::chrono::seconds{10});
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // TaskSequencer is blocked now until the future of task1 is ready because it waits forever by default.
    test.add(task2);
    test.add(task3);
    test.add(task4);
    TimePoint expectedTimeoutDateMs = TimePoint::now();
    _fulfillDelayedFuture = true;
    sequence = waitForTestFutureCreation(3, std::chrono::seconds{10});
    EXPECT_EQ(3, sequence.size());
    std::unique_lock<std::mutex> lock(_actualTimeoutDateMsMutex);
    EXPECT_EQ(1, _actualTimeoutDateMs.size());
    EXPECT_TRUE(_actualTimeoutDateMs[0] - expectedTimeoutDateMs.toMilliseconds() < 500);
    EXPECT_TRUE(_actualTimeoutDateMs[0] >= expectedTimeoutDateMs.toMilliseconds());
}
