/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "joynr/joynrlogging.h"

#include "joynr/SingleThreadedDelayedScheduler.h"

#include "utils/MockObjects.h"

#include <stdint.h>
#include <cassert>
#include <chrono>

#include <QString>

using namespace ::testing;
using namespace joynr;
using namespace joynr_logging;

using ::testing::StrictMock;

namespace SingleThreadedDelayedSchedulerTest
{
Logger* logger = Logging::getInstance()->getLogger("MSG", "SingleThreadedDelayedSchedulerTest");
}

TEST(SingleThreadedDelayedSchedulerTest, startAndShutdownWithoutWork)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", std::chrono::milliseconds::zero());

    scheduler.shutdown();
}

TEST(SingleThreadedDelayedSchedulerTest, startAndShutdownWithPendingWork_callDtorOfRunnablesCorrect)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", std::chrono::milliseconds::zero());

    // Dtor should be called
    StrictMock<MockRunnable>* runnable1 = new StrictMock<MockRunnable>(true);
    scheduler.schedule(runnable1, joynr::OptionalDelay(std::chrono::milliseconds(100)));

    // Dtor called after scheduler was cleaned
    StrictMock<MockRunnable> runnable2(false);
    scheduler.schedule(&runnable2, joynr::OptionalDelay(std::chrono::milliseconds(100)));

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_CALL(runnable2, dtorCalled()).Times(1);
}

TEST(SingleThreadedDelayedSchedulerTest, testAccuracyOfDelayedScheduler)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", std::chrono::milliseconds::zero());

    StrictMock<MockRunnableWithAccuracy> runnable1(false, 5);

    scheduler.schedule(&runnable1, joynr::OptionalDelay(std::chrono::milliseconds(5)));

    EXPECT_CALL(runnable1, runCalled()).Times(1);
    EXPECT_CALL(runnable1, runCalledInTime()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    EXPECT_CALL(runnable1, dtorCalled()).Times(1);
}

TEST(SingleThreadedDelayedSchedulerTest, callDtorOfRunnablesAfterSchedulerHasExpired)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", std::chrono::milliseconds::zero());

    StrictMock<MockRunnable>* runnable1 = new StrictMock<MockRunnable>(true);

    scheduler.schedule(runnable1, joynr::OptionalDelay(std::chrono::milliseconds(5)));

    EXPECT_CALL(*runnable1, run()).Times(1);
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();
}

TEST(SingleThreadedDelayedSchedulerTest, testRunnableWithoutDelay)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", std::chrono::milliseconds::zero());

    StrictMock<MockRunnableWithAccuracy> runnable1(false, 0);

    EXPECT_CALL(runnable1, runCalled()).Times(1);
    EXPECT_CALL(runnable1, runCalledInTime()).Times(1);

    scheduler.schedule(&runnable1, joynr::OptionalDelay(std::chrono::milliseconds::zero()));

    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    scheduler.shutdown();

    EXPECT_CALL(runnable1, dtorCalled()).Times(1);
}

TEST(SingleThreadedDelayedSchedulerTest, scheduleAndUnscheduleRunnable)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", std::chrono::milliseconds::zero());

    StrictMock<MockRunnableWithAccuracy> runnable1(false, 5);

    joynr::DelayedScheduler::RunnableHandle handle = scheduler.schedule(&runnable1, joynr::OptionalDelay(std::chrono::milliseconds(5)));

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    scheduler.unschedule(handle);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();

    EXPECT_CALL(runnable1, dtorCalled()).Times(1);
}

TEST(SingleThreadedDelayedSchedulerTest, scheduleAndUnscheduleRunnable_CallDtorOnUnschedule)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", std::chrono::milliseconds::zero());

    StrictMock<MockRunnableWithAccuracy>* runnable1 = new StrictMock<MockRunnableWithAccuracy>(true, 5u);

    joynr::DelayedScheduler::RunnableHandle handle = scheduler.schedule(runnable1, joynr::OptionalDelay(std::chrono::milliseconds(5)));

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    scheduler.unschedule(handle);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.shutdown();
}

TEST(SingleThreadedDelayedSchedulerTest, useDefaultDelay)
{
    SingleThreadedDelayedScheduler scheduler("SingleThreadedDelayedScheduler", std::chrono::milliseconds(10));

    StrictMock<MockRunnableWithAccuracy> runnable1(false, 10u);

    scheduler.schedule(&runnable1);

    EXPECT_CALL(runnable1, runCalled()).Times(1);
    EXPECT_CALL(runnable1, runCalledInTime()).Times(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    scheduler.shutdown();

    EXPECT_CALL(runnable1, dtorCalled()).Times(1);
}
