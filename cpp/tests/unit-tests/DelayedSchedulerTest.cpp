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

#include "gtest/gtest.h"
#include "joynr/Logger.h"

#include "joynr/DelayedScheduler.h"

#include "utils/MockObjects.h"
#include "joynr/TimeUtils.h"

#include <stdint.h>
#include <cassert>

using namespace joynr;

using namespace ::testing;
using ::testing::StrictMock;


// Expected accuracy of the timer in milliseconds
static const uint64_t timerAccuracy_ms = 5U;

class SimpleDelayedScheduler :
    public joynr::DelayedScheduler
{

public:

    SimpleDelayedScheduler()
        : joynr::DelayedScheduler(std::bind(&SimpleDelayedScheduler::workAvailable, this, std::placeholders::_1)),
          est_ms(0)
    {
    }

    SimpleDelayedScheduler(const uint64_t delay)
        : joynr::DelayedScheduler(std::bind(&SimpleDelayedScheduler::workAvailable, this, std::placeholders::_1)),
          est_ms(TimeUtils::getCurrentMillisSinceEpoch() + delay)
    {
    }

    ~SimpleDelayedScheduler() = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
    MOCK_CONST_METHOD1(workAvailableCalled, void (joynr::Runnable*));
    MOCK_CONST_METHOD0(workAvailableInTime, void ());
#pragma GCC diagnostic pop

    void workAvailable(joynr::Runnable* runnable)
    {
        workAvailableCalled(runnable);

        if(est_ms > 0)
        {
            const uint64_t now_ms = TimeUtils::getCurrentMillisSinceEpoch();
            const uint64_t diff_ms = (now_ms > est_ms) ? now_ms - est_ms : est_ms - now_ms;

            JOYNR_LOG_TRACE(logger, "Runnable is available");
            JOYNR_LOG_TRACE(logger, " ETA        : {}",est_ms);
            JOYNR_LOG_TRACE(logger, " current    : {}",now_ms);
            JOYNR_LOG_TRACE(logger, " difference : {}",diff_ms);

            if (diff_ms <= timerAccuracy_ms)
            {
                workAvailableInTime();
            }
        }
        else
        {
            JOYNR_LOG_TRACE(logger, "No delay given but work available called.");
        }
    }

private:
    const uint64_t est_ms;
};

TEST(DelayedSchedulerTest, startAndShutdownWithoutWork)
{
    SimpleDelayedScheduler scheduler;

    scheduler.shutdown();
}

TEST(DelayedSchedulerTest, startAndShutdownWithPendingWork_callDtorOfRunnablesCorrect)
{
    SimpleDelayedScheduler scheduler;

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

TEST(DelayedSchedulerTest, testAccuracyOfDelayedScheduler)
{
    SimpleDelayedScheduler scheduler(5);
    StrictMock<MockRunnable> runnable1(false);
    scheduler.schedule(&runnable1, std::chrono::milliseconds(5));

    EXPECT_CALL(scheduler, workAvailableCalled(&runnable1)).Times(1);
    EXPECT_CALL(scheduler, workAvailableInTime()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    EXPECT_CALL(runnable1, dtorCalled()).Times(1);
}

TEST(DelayedSchedulerTest, avoidCallingDtorOfRunnablesAfterSchedulerHasExpired)
{
    SimpleDelayedScheduler scheduler;
    StrictMock<MockRunnable> runnable1(true);
    scheduler.schedule(&runnable1, std::chrono::milliseconds(5));

    EXPECT_CALL(scheduler, workAvailableCalled(&runnable1)).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    EXPECT_CALL(runnable1, dtorCalled()).Times(1);
}

TEST(DelayedSchedulerTest, scheduleAndUnscheduleRunnable_NoCallToRunnable)
{
    SimpleDelayedScheduler scheduler(5);
    StrictMock<MockRunnable> runnable1(false);
    joynr::DelayedScheduler::RunnableHandle handle = scheduler.schedule(&runnable1, std::chrono::milliseconds(5));

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    scheduler.unschedule(handle);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    EXPECT_CALL(runnable1, dtorCalled()).Times(1);
}

TEST(DelayedSchedulerTest, scheduleAndUnscheduleRunnable_CallDtorOnUnschedule)
{
    SimpleDelayedScheduler scheduler(5);
    StrictMock<MockRunnable>* runnable1 = new StrictMock<MockRunnable>(true);
    joynr::DelayedScheduler::RunnableHandle handle = scheduler.schedule(runnable1, std::chrono::milliseconds(5));

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    scheduler.unschedule(handle);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();
}
