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
#include "gtest/gtest.h"

#include "joynr/ThreadPoolDelayedScheduler.h"
#include "utils/MockObjects.h"
#include "joynr/SingleThreadedIOService.h"
#include "utils/TestRunnable.h"

#include <cstdint>
#include <cassert>
#include <chrono>

using namespace ::testing;
using namespace joynr;

using ::testing::StrictMock;

// Expected accuracy of the timer in milliseconds

class ThreadPoolDelayedSchedulerTest : public testing::Test
{
public:
    ThreadPoolDelayedSchedulerTest() : singleThreadedIOService()
    {
        singleThreadedIOService.start();
    }
protected:
    SingleThreadedIOService singleThreadedIOService;
};

TEST_F(ThreadPoolDelayedSchedulerTest, startAndShutdownWithoutWork)
{
    ThreadPoolDelayedScheduler scheduler(1, "ThreadPoolDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    scheduler.shutdown();
}

TEST_F(ThreadPoolDelayedSchedulerTest, startAndShutdownWithPendingWork_callDtorOfRunnablesCorrect)
{
    ThreadPoolDelayedScheduler scheduler(1, "ThreadPoolDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    // Dtor should be called
    StrictMock<MockRunnable>* runnable1 = new StrictMock<MockRunnable>(true);
    scheduler.schedule(runnable1, std::chrono::milliseconds(100));

    // Dtor called after scheduler was cleaned
    StrictMock<MockRunnable> runnable2(false);
    scheduler.schedule(&runnable2, std::chrono::milliseconds(100));

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_CALL(runnable2, dtorCalled()).Times(1);
}

TEST_F(ThreadPoolDelayedSchedulerTest, testAccuracyOfDelayedScheduler)
{
    ThreadPoolDelayedScheduler scheduler(1, "ThreadPoolDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    StrictMock<MockRunnableWithAccuracy> runnable1(false, 5);

    scheduler.schedule(&runnable1, std::chrono::milliseconds(5));

    EXPECT_CALL(runnable1, runCalled()).Times(1);
    EXPECT_CALL(runnable1, runCalledInTime()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    EXPECT_CALL(runnable1, dtorCalled()).Times(1);
}

TEST_F(ThreadPoolDelayedSchedulerTest, callDtorOfRunnablesAfterSchedulerHasExpired)
{
    ThreadPoolDelayedScheduler scheduler(1, "ThreadPoolDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    StrictMock<MockRunnable>* runnable1 = new StrictMock<MockRunnable>(true);

    scheduler.schedule(runnable1, std::chrono::milliseconds(5));

    EXPECT_CALL(*runnable1, run()).Times(1);
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();
}

TEST_F(ThreadPoolDelayedSchedulerTest, scheduleAndUnscheduleRunnable)
{
    ThreadPoolDelayedScheduler scheduler(1, "ThreadPoolDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    StrictMock<MockRunnableWithAccuracy> runnable1(false, 5);

    joynr::DelayedScheduler::RunnableHandle handle = scheduler.schedule(&runnable1, std::chrono::milliseconds(5));

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    scheduler.unschedule(handle);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    EXPECT_CALL(runnable1, dtorCalled()).Times(1);
}

TEST_F(ThreadPoolDelayedSchedulerTest, scheduleAndUnscheduleRunnable_CallDtorOnUnschedule)
{
    ThreadPoolDelayedScheduler scheduler(1, "ThreadPoolDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    StrictMock<MockRunnableWithAccuracy>* runnable1 = new StrictMock<MockRunnableWithAccuracy>(true, 5);

    joynr::DelayedScheduler::RunnableHandle handle = scheduler.schedule(runnable1, std::chrono::milliseconds(5));

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    scheduler.unschedule(handle);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();
}

TEST_F(ThreadPoolDelayedSchedulerTest, useDefaultDelay)
{
    ThreadPoolDelayedScheduler scheduler(1, "ThreadPoolDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds(10));

    StrictMock<MockRunnableWithAccuracy> runnable1(false, 10);

    scheduler.schedule(&runnable1);

    EXPECT_CALL(runnable1, runCalled()).Times(1);
    EXPECT_CALL(runnable1, runCalledInTime()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    scheduler.shutdown();

    EXPECT_CALL(runnable1, dtorCalled()).Times(1);
}

TEST_F(ThreadPoolDelayedSchedulerTest, schedule_deletingRunnablesCorrectly)
{
    ThreadPoolDelayedScheduler scheduler(3, "ThreadPool", singleThreadedIOService.getIOService());
    TestRunnable* runnable = new TestRunnable();
    scheduler.schedule(runnable, std::chrono::milliseconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler.shutdown();
}
