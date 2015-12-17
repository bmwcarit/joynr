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
// We need to tell boost we will use the std placeholders
#define BOOST_BIND_NO_PLACEHOLDERS

#include "gtest/gtest.h"
#include "joynr/joynrlogging.h"

#include "joynr/DelayedScheduler.h"

#include "utils/MockObjects.h"
#include "joynr/TimeUtils.h"

#include <stdint.h>
#include <cassert>

using namespace joynr;
using namespace joynr_logging;

using namespace ::testing;
using ::testing::StrictMock;

using namespace std::placeholders;

namespace DelayedSchedulerTest
{
Logger* logger = Logging::getInstance()->getLogger("MSG",
    "DelayedSchedulerTest");
}

// Expected accuracy of the timer in milliseconds
static const uint64_t timerAccuracy_ms = 5U;

class SimpleDelayedScheduler :
    public joynr::DelayedScheduler
{

public:

    SimpleDelayedScheduler()
        : joynr::DelayedScheduler(std::bind(&SimpleDelayedScheduler::workAvailable, this, _1)),
          est_ms(0)
    {
    }

    SimpleDelayedScheduler(const uint64_t delay)
        : joynr::DelayedScheduler(std::bind(&SimpleDelayedScheduler::workAvailable, this, _1)),
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

            LOG_TRACE(DelayedSchedulerTest::logger, FormatString("Runnable is available").str());
            LOG_TRACE(DelayedSchedulerTest::logger, FormatString(" ETA        : %1").arg(est_ms).str());
            LOG_TRACE(DelayedSchedulerTest::logger, FormatString(" current    : %1").arg(now_ms).str());
            LOG_TRACE(DelayedSchedulerTest::logger, FormatString(" difference : %1").arg(diff_ms).str());

            if (diff_ms <= timerAccuracy_ms)
            {
                workAvailableInTime();
            }
        }
        else
        {
            LOG_TRACE(DelayedSchedulerTest::logger, "No delay given but work available called.");
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

TEST(DelayedSchedulerTest, testRunnableWithoutDelay)
{
    SimpleDelayedScheduler scheduler(0);
    StrictMock<MockRunnable> runnable1(false);

    EXPECT_CALL(scheduler, workAvailableCalled(&runnable1)).Times(1);
    EXPECT_CALL(scheduler, workAvailableInTime()).Times(1);

    scheduler.schedule(&runnable1, std::chrono::milliseconds::zero());

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
