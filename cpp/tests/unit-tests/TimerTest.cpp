/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "gmock/gmock.h"

#include "joynr/Timer.h"
#include "joynr/Logger.h"

#include "tests/utils/TimeUtils.h"
#include "joynr/Semaphore.h"
#include <cstdint>
#include <unordered_map>

using namespace joynr;

using namespace ::testing;
using ::testing::StrictMock;

// Expected accuracy of the timer in milliseconds
static const std::int64_t timerAccuracy_ms = 10;

ACTION_P(ReleaseSemaphore, semaphore)
{
    semaphore->notify();
}


class TimerTest : public ::testing::Test {
public:

    ADD_LOGGER(TimerTest);

    TimerTest() :
        ::testing::Test(),
        timer(),
        expired([this](Timer::TimerId id) { this->timerExpired(id); }),
        removed([this](Timer::TimerId id) { this->onTimerRemoved(id); }),
        timerStartMapping()
    {}

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        timer.shutdown();
    }

    Timer::TimerId addTimer(std::uint64_t ms, bool periodic)
    {
         Timer::TimerId id = timer.addTimer(expired, removed, ms, periodic);
         std::uint64_t start = TimeUtils::getCurrentMillisSinceEpoch();
         timerStartMapping.emplace(id, start);
         return id;
    }

    // wrapper
    bool removeTimer(Timer::TimerId id) { return timer.removeTimer(id); }

    void timerExpired(Timer::TimerId id)
    {
        const std::uint64_t now_ms   = TimeUtils::getCurrentMillisSinceEpoch();
        const std::uint64_t start_ms = timerStartMapping.at(id);
        const std::int64_t  diff_ms  = now_ms - start_ms;

        JOYNR_LOG_TRACE(logger, "Timer {} expired",id);
        JOYNR_LOG_TRACE(logger, "  started    : {}",start_ms);
        JOYNR_LOG_TRACE(logger, "  returned   : {}",now_ms);
        JOYNR_LOG_TRACE(logger, "  difference : {}",diff_ms);

        onTimerExpired(id, diff_ms);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
    MOCK_CONST_METHOD2(onTimerExpired, void (Timer::TimerId, std::int64_t));
    MOCK_CONST_METHOD1(onTimerRemoved, void (Timer::TimerId id));
#pragma GCC diagnostic pop

protected:
    Semaphore semaphore;

private:

    Timer timer;
    std::function<void(Timer::TimerId)> expired;
    std::function<void(Timer::TimerId)> removed;

    std::unordered_map<Timer::TimerId, std::uint64_t> timerStartMapping;
};

INIT_LOGGER(TimerTest);

TEST_F(TimerTest, deinitializationWithoutRun_WaitSomeTime) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST_F(TimerTest, deinitializationWithoutRun_Immediately) {
}

TEST_F(TimerTest, shutdownWithActiveTimer) {

    Timer::TimerId id = addTimer(100000, false);

    EXPECT_CALL(*this, onTimerExpired(_, _)).Times(0);
    EXPECT_CALL(*this, onTimerRemoved(id)).Times(1);

}

TEST_F(TimerTest, testCallback) {

    Timer::TimerId id = addTimer(20, false);

    EXPECT_CALL(*this, onTimerExpired(id, Lt(20 + timerAccuracy_ms)))
            .Times(1)
            .WillOnce(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*this, onTimerRemoved(id)).Times(0);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
}

TEST_F(TimerTest, reorganizeTimerByAddingAnEarlierTimer) {
    Sequence s;

    Timer::TimerId id250 = addTimer(250, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    Timer::TimerId id10 = addTimer(10, false);

    EXPECT_CALL(*this, onTimerExpired(id10, _)).Times(1).InSequence(s).WillOnce(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*this, onTimerRemoved(_)).Times(0);

    EXPECT_CALL(*this, onTimerExpired(id250, _)).Times(1).InSequence(s).WillOnce(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*this, onTimerRemoved(_)).Times(0);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));
}

TEST_F(TimerTest, reorganizeTimerByRemovingTheEarliest) {

    Sequence s;

    Timer::TimerId id250 = addTimer(250, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    Timer::TimerId id1000 = addTimer(1000, false);

    EXPECT_CALL(*this, onTimerExpired(_, _)).Times(0);
    EXPECT_CALL(*this, onTimerRemoved(id1000)).Times(1).InSequence(s).WillOnce(ReleaseSemaphore(&semaphore));

    removeTimer(id1000);

    EXPECT_CALL(*this, onTimerExpired(id250, _)).Times(1).InSequence(s).WillOnce(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*this, onTimerRemoved(_)).Times(0);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));
}

TEST_F(TimerTest, removingTheOnlyActiveTimer) {
    Timer::TimerId id10 = addTimer(1000, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_CALL(*this, onTimerExpired(_, _)).Times(0);
    EXPECT_CALL(*this, onTimerRemoved(id10)).Times(1).WillOnce(ReleaseSemaphore(&semaphore));

    removeTimer(id10);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(500)));
}

TEST_F(TimerTest, usingIntervallTimer) {

    Timer::TimerId id10 = addTimer(10, true);

    EXPECT_CALL(*this, onTimerExpired(id10, _)).Times(AtLeast(2));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // timer.shutdown() will be called from TearDown()
    // It is ensured that onTimerRemoved is invoked before TearDown() returns
    EXPECT_CALL(*this, onTimerRemoved(id10)).Times(1);
}

/* This test is disabled because it could fail sometimes ("Stress-Test") */
TEST_F(TimerTest, DISABLED_stressTest) {

    EXPECT_CALL(*this, onTimerRemoved(_)).Times(0);
    for (int i = 0; i < 500; ++i)
    {
        Timer::TimerId id = addTimer(1, false);
        EXPECT_CALL(*this, onTimerExpired(id, _)).Times(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}
