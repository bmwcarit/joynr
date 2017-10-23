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

#include <gtest/gtest.h>

#include "joynr/ThreadPool.h"

#include "tests/mock/MockRunnable.h"
#include "tests/mock/MockRunnableBlocking.h"

using namespace ::testing;
using namespace joynr;

using ::testing::StrictMock;

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

    EXPECT_CALL(*runnable2, run()).Times(1);
    pool->execute(runnable2);

    EXPECT_CALL(*runnable1, run()).Times(1);
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
    pool->execute(runnable1);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    pool->shutdown();

    std::this_thread::sleep_for(std::chrono::milliseconds(3));

    EXPECT_CALL(*runnable2, dtorCalled()).Times(1);
}

TEST(ThreadPoolTest, callDtorOfRunnabeAfterWorkHasDone)
{
    auto pool = std::make_shared<ThreadPool>("ThreadPoolTest", 1);
    pool->init();

    auto runnable1 = std::make_shared<StrictMock<MockRunnable>>();

    EXPECT_CALL(*runnable1, run()).Times(1);
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);

    pool->execute(runnable1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    pool->shutdown();
}

TEST(ThreadPoolTest, testEndlessRunningRunnableToQuitWithShutdownCall)
{
    auto pool = std::make_shared<ThreadPool>("ThreadPoolTest", 1);
    pool->init();

    auto runnable1 = std::make_shared<StrictMock<MockRunnableBlocking>>();

    EXPECT_CALL(*runnable1, runEntry()).Times(1);
    pool->execute(runnable1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_CALL(*runnable1, shutdownCalled()).Times(1);
    EXPECT_CALL(*runnable1, runExit()).Times(1);
    pool->shutdown();

    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
}

TEST(ThreadPoolTest, shutdownThreadPoolWhileRunnableIsInQueue)
{
    auto pool = std::make_shared<ThreadPool>("ThreadPoolTest", 1);
    pool->init();

    auto runnable1 = std::make_shared<StrictMock<MockRunnableBlocking>>();
    auto runnable2 = std::make_shared<StrictMock<MockRunnableBlocking>>();

    EXPECT_CALL(*runnable1, runEntry()).Times(1);
    pool->execute(runnable1);
    pool->execute(runnable2);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_CALL(*runnable1, shutdownCalled()).Times(1);
    EXPECT_CALL(*runnable1, runExit()).Times(1);
    pool->shutdown();

    EXPECT_CALL(*runnable2, dtorCalled()).Times(1);
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
}

TEST(ThreadPoolTest, finishWorkWhileAnotherRunnableIsInTheQueue)
{
    auto pool = std::make_shared<ThreadPool>("ThreadPoolTest", 1);
    pool->init();

    auto runnable1 = std::make_shared<StrictMock<MockRunnableBlocking>>();
    auto runnable2 = std::make_shared<StrictMock<MockRunnableBlocking>>();

    EXPECT_CALL(*runnable1, runEntry()).Times(1);
    pool->execute(runnable1);
    pool->execute(runnable2);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Shutdown will not be called because we do it manually here
    EXPECT_CALL(*runnable1, runExit()).Times(1);
    EXPECT_CALL(*runnable2, runEntry()).Times(1);
    runnable1->manualShutdown();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_CALL(*runnable2, shutdownCalled()).Times(1);
    EXPECT_CALL(*runnable2, runExit()).Times(1);
    pool->shutdown();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_CALL(*runnable2, dtorCalled()).Times(1);
    EXPECT_CALL(*runnable1, dtorCalled()).Times(1);
}
