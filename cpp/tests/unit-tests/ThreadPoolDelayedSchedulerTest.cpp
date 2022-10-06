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
#include <cassert>
#include <chrono>
#include <cstdint>

#include "tests/utils/Gtest.h"

#include "joynr/Semaphore.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/ThreadPoolDelayedScheduler.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockRunnable.h"
#include "tests/mock/MockRunnableWithAccuracy.h"
#include "tests/utils/PtrUtils.h"
#include "tests/utils/TestRunnable.h"

using namespace ::testing;
using namespace joynr;

using ::testing::StrictMock;

// Expected accuracy of the timer in milliseconds

class ThreadPoolDelayedSchedulerTest : public testing::Test
{
public:
    ThreadPoolDelayedSchedulerTest()
            : singleThreadedIOService(std::make_shared<SingleThreadedIOService>()), semaphore(0)
    {
        singleThreadedIOService->start();
    }

    ~ThreadPoolDelayedSchedulerTest()
    {
        singleThreadedIOService->stop();
    }

protected:
    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    Semaphore semaphore;
};

TEST_F(ThreadPoolDelayedSchedulerTest, startAndShutdownWithoutWork)
{
    auto scheduler =
            std::make_shared<ThreadPoolDelayedScheduler>(1,
                                                         "ThreadPoolDelayedScheduler",
                                                         singleThreadedIOService->getIOService(),
                                                         std::chrono::milliseconds::zero());

    scheduler->shutdown();
}

TEST_F(ThreadPoolDelayedSchedulerTest, startAndShutdownWithPendingWork_callDtorOfRunnablesCorrect)
{
    auto scheduler =
            std::make_shared<ThreadPoolDelayedScheduler>(1,
                                                         "ThreadPoolDelayedScheduler",
                                                         singleThreadedIOService->getIOService(),
                                                         std::chrono::milliseconds::zero());

    // Dtor should be called
    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();

    // Dtor called after scheduler was cleaned
    auto runnable2 = std::make_shared<StrictMock<MockRunnable>>();

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
    EXPECT_CALL(*runnable2, dtorCalled()).Times(1);

    EXPECT_CALL(*runnable1, shutdown()).Times(AnyNumber());
    EXPECT_CALL(*runnable2, shutdown()).Times(AnyNumber());

    scheduler->schedule(runnable1, std::chrono::seconds(2));
    scheduler->schedule(runnable2, std::chrono::seconds(2));

    EXPECT_CALL(*runnable1, run()).Times(0);
    EXPECT_CALL(*runnable2, run()).Times(0);

    EXPECT_EQ(2, runnable1.use_count());
    EXPECT_EQ(2, runnable2.use_count());

    scheduler->shutdown();
    test::util::resetAndWaitUntilDestroyed(runnable1);
    test::util::resetAndWaitUntilDestroyed(runnable2);
}

TEST_F(ThreadPoolDelayedSchedulerTest, testAccuracyOfDelayedScheduler)
{
    auto scheduler =
            std::make_shared<ThreadPoolDelayedScheduler>(1,
                                                         "ThreadPoolDelayedScheduler",
                                                         singleThreadedIOService->getIOService(),
                                                         std::chrono::milliseconds::zero());

    auto runnable1 = std::make_shared<StrictMock<MockRunnableWithAccuracy>>(1000);

    EXPECT_CALL(*runnable1, runCalled()).Times(1);
    EXPECT_CALL(*runnable1, runCalledInTime()).Times(1).WillOnce(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
    EXPECT_CALL(*runnable1, shutdown()).Times(AnyNumber());

    scheduler->schedule(runnable1, std::chrono::seconds(1));

    // wait for the runnable's execution
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));

    scheduler->shutdown();
    test::util::resetAndWaitUntilDestroyed(runnable1);
}

TEST_F(ThreadPoolDelayedSchedulerTest, callDtorOfRunnablesAfterSchedulerHasExpired)
{
    auto scheduler =
            std::make_shared<ThreadPoolDelayedScheduler>(1,
                                                         "ThreadPoolDelayedScheduler",
                                                         singleThreadedIOService->getIOService(),
                                                         std::chrono::milliseconds::zero());

    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();

    EXPECT_CALL(*runnable1, run()).Times(1).WillOnce(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
    EXPECT_CALL(*runnable1, shutdown()).Times(AnyNumber());

    scheduler->schedule(runnable1, std::chrono::milliseconds(5));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));

    scheduler->shutdown();
    test::util::resetAndWaitUntilDestroyed(runnable1);
}

TEST_F(ThreadPoolDelayedSchedulerTest, scheduleAndUnscheduleRunnable)
{
    auto scheduler =
            std::make_shared<ThreadPoolDelayedScheduler>(1,
                                                         "ThreadPoolDelayedScheduler",
                                                         singleThreadedIOService->getIOService(),
                                                         std::chrono::milliseconds::zero());

    auto runnable1 = std::make_shared<StrictMock<MockRunnableWithAccuracy>>(1000);

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1).WillOnce(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*runnable1, runCalled()).Times(0);
    EXPECT_CALL(*runnable1, shutdown()).Times(AnyNumber());

    joynr::DelayedScheduler::RunnableHandle handle =
            scheduler->schedule(runnable1, std::chrono::seconds(1));

    EXPECT_EQ(2, runnable1.use_count());

    runnable1.reset();

    scheduler->unschedule(handle);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));

    scheduler->shutdown();
}

TEST_F(ThreadPoolDelayedSchedulerTest, useDefaultDelay)
{
    auto scheduler =
            std::make_shared<ThreadPoolDelayedScheduler>(1,
                                                         "ThreadPoolDelayedScheduler",
                                                         singleThreadedIOService->getIOService(),
                                                         std::chrono::milliseconds(1000));

    auto runnable1 = std::make_shared<StrictMock<MockRunnableWithAccuracy>>(1000);

    EXPECT_CALL(*runnable1, runCalled()).Times(1);
    EXPECT_CALL(*runnable1, runCalledInTime()).Times(1).WillOnce(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
    EXPECT_CALL(*runnable1, shutdown()).Times(AnyNumber());

    scheduler->schedule(runnable1);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));

    scheduler->shutdown();
    test::util::resetAndWaitUntilDestroyed(runnable1);
}

TEST_F(ThreadPoolDelayedSchedulerTest, schedule_deletingRunnablesCorrectly)
{
    auto scheduler = std::make_shared<ThreadPoolDelayedScheduler>(
            3, "ThreadPool", singleThreadedIOService->getIOService());
    auto runnable = std::make_shared<TestRunnable>();
    scheduler->schedule(runnable, std::chrono::milliseconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler->shutdown();
    test::util::resetAndWaitUntilDestroyed(runnable);
}
