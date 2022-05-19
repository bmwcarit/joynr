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

#include "joynr/Semaphore.h"
#include "joynr/ThreadPool.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockRunnable.h"
#include "tests/mock/MockRunnableBlocking.h"

using namespace ::testing;
using namespace joynr;

using ::testing::StrictMock;

// General hints:
//
// dtorCalled() EXPECT_CALL checks are placed in front of
// pool->shutdown() calls because our own shared_ptr copies are
// reset via std::move(...) and destruction of runnables happens
// already internally and not just after passing the end of the
// test sections as it was in earlier versions of this test.
//
// Also note that the ThreadPools are initialized with only
// a single thread. This is required for the tests to work
// so that some runnable gets blocked while another one is
// already in progress.

TEST(ThreadPoolTest, startAndShutdownWithoutWork)
{
    auto pool = std::make_shared<ThreadPool>("ThreadPoolTest", 10);
    pool->init();
    pool->shutdown();
}

TEST(ThreadPoolTest, startAndShutdown_callDtorOfRunnablesCorrect)
{
    auto pool = std::make_shared<ThreadPool>("ThreadPoolTest", 1);
    pool->init();

    // Dtor should be called after execution has finished
    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();

    // Dtor called after the test
    auto runnable2 = std::make_shared<StrictMock<MockRunnable>>();

    joynr::Semaphore semaphore1(0);
    EXPECT_CALL(*runnable1, run()).Times(1).WillOnce(ReleaseSemaphore(&semaphore1));
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
    pool->execute(std::move(runnable1));

    joynr::Semaphore semaphore2(0);
    EXPECT_CALL(*runnable2, run()).Times(1).WillOnce(ReleaseSemaphore(&semaphore2));
    EXPECT_CALL(*runnable2, dtorCalled()).Times(1);
    pool->execute(std::move(runnable2));

    EXPECT_TRUE(semaphore1.waitFor(std::chrono::milliseconds(1000)));
    EXPECT_TRUE(semaphore2.waitFor(std::chrono::milliseconds(1000)));

    // At this point both Runnables have been removed from scheduler
    // and were executed by a thread from Threadpool.
    // Afterwards shared_ptr to Runnable is erased from
    // _currentlyRunning and local copy of the thread gets released
    // as well, which should invoke the destructor.

    pool->shutdown();
}

TEST(ThreadPoolTest, callDtorOfRunnabeAfterWorkHasDone)
{
    auto pool = std::make_shared<ThreadPool>("ThreadPoolTest", 1);
    pool->init();

    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();

    joynr::Semaphore semaphore(0);

    EXPECT_CALL(*runnable1, run()).Times(1).WillOnce(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*runnable1, shutdown()).Times(AtMost(1));
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    pool->execute(std::move(runnable1));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));

    pool->shutdown();
}

TEST(ThreadPoolTest, testEndlessRunningRunnableToQuitWithShutdownCall)
{
    auto pool = std::make_shared<ThreadPool>("ThreadPoolTest", 1);
    pool->init();

    auto runnable1 = std::make_shared<StrictMock<MockRunnableBlocking>>();

    joynr::Semaphore semaphore(0);

    EXPECT_CALL(*runnable1, runEntry()).Times(1).WillOnce(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*runnable1, shutdownCalled()).Times(1);
    EXPECT_CALL(*runnable1, runExit()).Times(1);
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    pool->execute(std::move(runnable1));

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));

    // the ThreadPool shutdown should find the Runnable in the
    // _currentlyRunning set and invoke its shutdown. This unblocks
    // the Runnables run() so that it can finish and get destructed

    pool->shutdown();
}

TEST(ThreadPoolTest, shutdownThreadPoolWhileRunnableIsInQueue)
{
    auto pool = std::make_shared<ThreadPool>("ThreadPoolTest", 1);
    pool->init();

    auto runnable1 = std::make_shared<StrictMock<MockRunnableBlocking>>();
    auto runnable2 = std::make_shared<StrictMock<MockRunnableBlocking>>();

    joynr::Semaphore semaphore1(0);

    EXPECT_CALL(*runnable1, runEntry()).Times(1).WillOnce(ReleaseSemaphore(&semaphore1));
    EXPECT_CALL(*runnable1, shutdownCalled()).Times(1);
    EXPECT_CALL(*runnable1, runExit()).Times(1);
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
    pool->execute(std::move(runnable1));
    EXPECT_TRUE(semaphore1.waitFor(std::chrono::milliseconds(1000)));

    // the runnable1 is being processed and runnable2 still
    // sits in the queue of the scheduler. The ThreadPool shutdown
    // unblocks the runnable1 but thread will not attempt
    // to continue with runnable2 since shutdown has been
    // signaled meanwhile. runnable2 is just getting destructed
    // without ever have been run or shutdown.
    EXPECT_CALL(*runnable2, runEntry()).Times(0);
    EXPECT_CALL(*runnable2, shutdownCalled()).Times(0);
    EXPECT_CALL(*runnable2, dtorCalled()).Times(1);
    pool->execute(std::move(runnable2));

    pool->shutdown();
}

TEST(ThreadPoolTest, finishWorkWhileAnotherRunnableIsInTheQueue)
{
    auto pool = std::make_shared<ThreadPool>("ThreadPoolTest", 1);
    pool->init();

    auto runnable1 = std::make_shared<StrictMock<MockRunnableBlocking>>();
    auto runnable2 = std::make_shared<StrictMock<MockRunnableBlocking>>();

    joynr::Semaphore semaphore1(0);
    EXPECT_CALL(*runnable1, runEntry()).Times(1).WillOnce(ReleaseSemaphore(&semaphore1));
    EXPECT_CALL(*runnable1, runExit()).Times(1);
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
    pool->execute(runnable1);

    joynr::Semaphore semaphore2(0);
    EXPECT_CALL(*runnable2, runEntry()).Times(1).WillOnce(ReleaseSemaphore(&semaphore2));
    EXPECT_CALL(*runnable2, shutdownCalled()).Times(1);
    EXPECT_CALL(*runnable2, runExit()).Times(1);
    EXPECT_CALL(*runnable2, dtorCalled()).Times(1);
    pool->execute(std::move(runnable2));

    EXPECT_TRUE(semaphore1.waitFor(std::chrono::milliseconds(1000)));

    // runnable1 has reached blocking state in run() and is part of
    // _currentlyRunning of ThreadPool.
    // Manually unblocking runnable1 causes thread to finish work on
    // runnable1 and then pick up runnable2 from scheduler.
    // runnable1 is also destructed by this action, latest after
    // giving up the shared_ptr reference by the reset() below.
    runnable1->manualShutdown();
    runnable1.reset();

    EXPECT_TRUE(semaphore2.waitFor(std::chrono::milliseconds(1000)));

    // runnable2 has now been inserted in _currentlyRunning and gets blocked
    // in its run() until notified by calling shutdown

    pool->shutdown();
}
