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
#include <cstdint>
#include <cassert>
#include <chrono>

#include <gtest/gtest.h>

#include "joynr/SingleThreadedDelayedScheduler.h"
#include "joynr/SingleThreadedIOService.h"

#include "tests/mock/MockRunnable.h"
#include "tests/utils/TestRunnable.h"
#include "tests/mock/MockRunnableWithAccuracy.h"

using namespace ::testing;
using namespace joynr;

using ::testing::StrictMock;

class SingleThreadedDelayedSchedulerTest : public testing::Test
{
public:
    SingleThreadedDelayedSchedulerTest() : singleThreadedIOService()
    {
        singleThreadedIOService.start();
    }
protected:
    SingleThreadedIOService singleThreadedIOService;
};

TEST_F(SingleThreadedDelayedSchedulerTest, startAndShutdownWithoutWork)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    scheduler.shutdown();
}

TEST_F(SingleThreadedDelayedSchedulerTest, startAndShutdownWithPendingWork_callDtorOfRunnablesCorrect)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    // Dtor should be called
    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();
    scheduler.schedule(runnable1, std::chrono::milliseconds(100));

    // Dtor called after scheduler was cleaned
    auto runnable2 = std::make_shared<StrictMock<MockRunnable>>();
    scheduler.schedule(runnable2, std::chrono::milliseconds(100));

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_CALL(*runnable2, dtorCalled()).Times(1);
}

TEST_F(SingleThreadedDelayedSchedulerTest, testAccuracyOfDelayedScheduler)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    auto runnable1 = std::make_shared<StrictMock<MockRunnableWithAccuracy>>(5);

    scheduler.schedule(runnable1, std::chrono::milliseconds(5));

    EXPECT_CALL(*runnable1, runCalled()).Times(1);
    EXPECT_CALL(*runnable1, runCalledInTime()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
}

TEST_F(SingleThreadedDelayedSchedulerTest, callDtorOfRunnablesAfterSchedulerHasExpired)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();

    scheduler.schedule(runnable1, std::chrono::milliseconds(5));

    EXPECT_CALL(*runnable1, run()).Times(1);
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();
}

TEST_F(SingleThreadedDelayedSchedulerTest, scheduleAndUnscheduleRunnable)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    auto runnable1 = std::make_shared<StrictMock<MockRunnableWithAccuracy>>(5);

    DelayedScheduler::RunnableHandle handle = scheduler.schedule(runnable1, std::chrono::milliseconds(5));

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    scheduler.unschedule(handle);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
}

TEST_F(SingleThreadedDelayedSchedulerTest, scheduleAndUnscheduleRunnable_CallDtorOnUnschedule)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds::zero());

    auto runnable1 = std::make_shared<StrictMock<MockRunnableWithAccuracy>>(5u);

    DelayedScheduler::RunnableHandle handle = scheduler.schedule(runnable1, std::chrono::milliseconds(5));

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    scheduler.unschedule(handle);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();
}

TEST_F(SingleThreadedDelayedSchedulerTest, useDefaultDelay)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", singleThreadedIOService.getIOService(), std::chrono::milliseconds(10));

    auto runnable1 = std::make_shared<StrictMock<MockRunnableWithAccuracy>>(10u);

    scheduler.schedule(runnable1);

    EXPECT_CALL(*runnable1, runCalled()).Times(1);
    EXPECT_CALL(*runnable1, runCalledInTime()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    scheduler.shutdown();

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
}

TEST_F(SingleThreadedDelayedSchedulerTest, schedule_deletingRunnablesCorrectly)
{
    SingleThreadedDelayedScheduler scheduler("SingleThread", singleThreadedIOService.getIOService());
    auto runnable = std::make_shared<TestRunnable>();
    scheduler.schedule(runnable, std::chrono::milliseconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler.shutdown();
}
