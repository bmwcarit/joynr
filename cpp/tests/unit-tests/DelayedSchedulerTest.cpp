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

#include "tests/utils/Gtest.h"

#include "joynr/DelayedScheduler.h"
#include "joynr/Logger.h"
#include "joynr/Semaphore.h"
#include "joynr/SingleThreadedIOService.h"

#include "tests/JoynrTest.h"
#include "tests/utils/TimeUtils.h"
#include "tests/mock/MockRunnable.h"

using namespace joynr;

using namespace ::testing;
using ::testing::StrictMock;

// Expected accuracy of the timer in milliseconds
static const std::uint64_t timerAccuracy_ms = 15U;

class SimpleDelayedScheduler : public DelayedScheduler
{

public:
    SimpleDelayedScheduler(std::shared_ptr<SingleThreadedIOService> singleThreadedIOService)
            : DelayedScheduler(std::bind(&SimpleDelayedScheduler::workAvailable,
                                         this,
                                         std::placeholders::_1),
                               singleThreadedIOService->getIOService()),
              est_ms(0)
    {
    }

    ~SimpleDelayedScheduler() = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
    MOCK_CONST_METHOD1(workAvailableCalled, void(std::shared_ptr<Runnable>));
    MOCK_CONST_METHOD0(workAvailableInTime, void());
#pragma GCC diagnostic pop

    DelayedScheduler::RunnableHandle schedule(std::shared_ptr<Runnable> runnable,
                                              std::chrono::milliseconds delay)
    {
        RunnableHandle currentHandle = DelayedScheduler::schedule(runnable, delay);
        est_ms = TimeUtils::getCurrentMillisSinceEpoch() + static_cast<std::uint64_t>(delay.count());
        return currentHandle;
    }

    void workAvailable(std::shared_ptr<Runnable> runnable)
    {
        const std::uint64_t now_ms = TimeUtils::getCurrentMillisSinceEpoch();
        workAvailableCalled(runnable);

        if (est_ms > 0) {
            const std::uint64_t diff_ms = (now_ms > est_ms) ? now_ms - est_ms : est_ms - now_ms;

            JOYNR_LOG_TRACE(logger(), "Runnable is available");
            JOYNR_LOG_TRACE(logger(), " ETA        : {}", est_ms);
            JOYNR_LOG_TRACE(logger(), " current    : {}", now_ms);
            JOYNR_LOG_TRACE(logger(), " difference : {}", diff_ms);

            if (diff_ms <= timerAccuracy_ms) {
                workAvailableInTime();
            }
        } else {
            JOYNR_LOG_TRACE(logger(), "No delay given but work available called.");
        }
        // if (runnable->isDeleteOnExit()) {
        //    delete runnable;
        //}
    }

private:
    std::uint64_t est_ms;
};

TEST(DelayedSchedulerTest, startAndShutdownWithoutWork)
{
    auto singleThreadedIOService = std::make_shared<SingleThreadedIOService>();
    singleThreadedIOService->start();
    auto scheduler = std::make_shared<SimpleDelayedScheduler>(singleThreadedIOService);

    scheduler->shutdown();
    singleThreadedIOService->stop();
}

TEST(DelayedSchedulerTest, startAndShutdownWithPendingWork_callDtorOfRunnablesCorrect)
{
    auto singleThreadedIOService = std::make_shared<SingleThreadedIOService>();
    singleThreadedIOService->start();
    auto scheduler = std::make_shared<SimpleDelayedScheduler>(singleThreadedIOService);

    // Dtor should be called
    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();
    scheduler->schedule(runnable1, std::chrono::milliseconds(100));

    // Dtor called after scheduler was cleaned
    auto runnable2 = std::make_shared<StrictMock<MockRunnable>>();
    scheduler->schedule(runnable2, std::chrono::milliseconds(100));

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler->shutdown();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_CALL(*runnable2, dtorCalled()).Times(1);
    singleThreadedIOService->stop();
}

TEST(DelayedSchedulerTest, testAccuracyOfDelayedScheduler)
{
    joynr::Semaphore semaphore;
    auto singleThreadedIOService = std::make_shared<SingleThreadedIOService>();
    singleThreadedIOService->start();
    auto scheduler = std::make_shared<SimpleDelayedScheduler>(singleThreadedIOService);
    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();

    EXPECT_CALL(*scheduler, workAvailableCalled(std::dynamic_pointer_cast<Runnable>(runnable1)))
            .Times(1);
    EXPECT_CALL(*scheduler, workAvailableInTime()).Times(1).WillOnce(ReleaseSemaphore(&semaphore));

    scheduler->schedule(runnable1, std::chrono::milliseconds(5));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));

    scheduler->shutdown();

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
    singleThreadedIOService->stop();
}

TEST(DelayedSchedulerTest, avoidCallingDtorOfRunnablesAfterSchedulerHasExpired)
{
    joynr::Semaphore semaphore;
    auto singleThreadedIOService = std::make_shared<SingleThreadedIOService>();
    singleThreadedIOService->start();
    auto scheduler = std::make_shared<SimpleDelayedScheduler>(singleThreadedIOService);
    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();

    EXPECT_CALL(*scheduler, workAvailableCalled(std::dynamic_pointer_cast<Runnable>(runnable1)))
            .Times(1)
            .WillOnce(ReleaseSemaphore(&semaphore));

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    scheduler->schedule(runnable1, std::chrono::milliseconds(5));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));

    scheduler->shutdown();
    singleThreadedIOService->stop();
}

TEST(DelayedSchedulerTest, scheduleAndUnscheduleRunnable_NoCallToRunnable)
{
    auto singleThreadedIOService = std::make_shared<SingleThreadedIOService>();
    singleThreadedIOService->start();
    auto scheduler = std::make_shared<SimpleDelayedScheduler>(singleThreadedIOService);
    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();
    DelayedScheduler::RunnableHandle handle =
            scheduler->schedule(runnable1, std::chrono::milliseconds(50));

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
    EXPECT_CALL(*scheduler, workAvailableCalled(std::dynamic_pointer_cast<Runnable>(runnable1)))
            .Times(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    scheduler->unschedule(handle);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    scheduler->shutdown();
    singleThreadedIOService->stop();
}

TEST(DelayedSchedulerTest, scheduleAndUnscheduleRunnable_CallDtorOnUnschedule)
{
    joynr::Semaphore semaphore;
    auto singleThreadedIOService = std::make_shared<SingleThreadedIOService>();
    singleThreadedIOService->start();
    auto scheduler = std::make_shared<SimpleDelayedScheduler>(singleThreadedIOService);
    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();
    DelayedScheduler::RunnableHandle handle =
            scheduler->schedule(runnable1, std::chrono::milliseconds(50));

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1).WillOnce(ReleaseSemaphore(&semaphore));

    scheduler->unschedule(handle);
    runnable1.reset();

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(100)));

    scheduler->shutdown();
    singleThreadedIOService->stop();
}
