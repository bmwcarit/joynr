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
#include <thread>

#include "tests/utils/Gtest.h"

#include "joynr/Semaphore.h"
#include "tests/utils/TimeUtils.h"

using joynr::Semaphore;

TEST(SemaphoreTest, createAndDestroy)
{
    Semaphore sem(2);
}

TEST(SemaphoreTest, singleWait_accept)
{
    Semaphore sem(1);

    sem.wait();
}

TEST(SemaphoreTest, singleWait_locking)
{
    Semaphore sem(0);

    std::thread t1(&Semaphore::wait, &sem);

    EXPECT_TRUE(t1.joinable());

    sem.notify();

    EXPECT_TRUE(t1.joinable());
    t1.join();
}

TEST(SemaphoreTest, multiWait_allAccept)
{
    Semaphore sem(3);

    std::thread t1(&Semaphore::wait, &sem);
    std::thread t2(&Semaphore::wait, &sem);
    std::thread t3(&Semaphore::wait, &sem);

    EXPECT_TRUE(t1.joinable());
    EXPECT_TRUE(t2.joinable());
    EXPECT_TRUE(t3.joinable());

    t1.join();
    t2.join();
    t3.join();
}

TEST(SemaphoreTest, multiWait_onlyAcceptFirst)
{
    Semaphore sem(1);

    std::thread t1(&Semaphore::wait, &sem);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::thread t2(&Semaphore::wait, &sem);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::thread t3(&Semaphore::wait, &sem);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_TRUE(t1.joinable());
    EXPECT_TRUE(t2.joinable());
    EXPECT_TRUE(t3.joinable());

    t1.join();

    sem.notify();

    EXPECT_TRUE(t2.joinable());
    EXPECT_TRUE(t3.joinable());
    t2.join();

    sem.notify();

    EXPECT_TRUE(t3.joinable());
    t3.join();
}

TEST(SemaphoreTest, timedWait_timeout)
{
    Semaphore sem(0);

    const std::uint64_t expectedTimeout = 100;
    const std::uint64_t start = joynr::TimeUtils::getCurrentMillisSinceEpoch();
    std::chrono::milliseconds expectedTimeoutMs(expectedTimeout);
    sem.waitFor(expectedTimeoutMs);
    const std::uint64_t duration = joynr::TimeUtils::getCurrentMillisSinceEpoch() - start;

    const std::uint64_t diff =
            (expectedTimeout > duration) ? expectedTimeout - duration : duration - expectedTimeout;

    EXPECT_GT(10, diff);
}

TEST(SemaphoreTest, timedWait_unlockAfterSomeTime)
{
    Semaphore sem(0);

    const std::uint64_t expectedTimeout = 1000;
    const std::uint64_t expectedUnlock = 80;
    std::uint64_t duration = 0;
    bool result = false;

    std::thread t1([&](Semaphore* semaphore, std::uint64_t /*timeout*/, std::uint64_t* dur, bool* res) {
                       const std::uint64_t start = joynr::TimeUtils::getCurrentMillisSinceEpoch();
                       (*res) = semaphore->waitFor(std::chrono::milliseconds(expectedTimeout));
                       (*dur) = joynr::TimeUtils::getCurrentMillisSinceEpoch() - start;
                   },
                   &sem,
                   expectedTimeout,
                   &duration,
                   &result);

    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    sem.notify();

    EXPECT_TRUE(t1.joinable());
    t1.join();

    const std::uint64_t diff =
            (expectedUnlock > duration) ? expectedUnlock - duration : duration - expectedUnlock;

    EXPECT_GT(10, diff);
    EXPECT_TRUE(result);
}
