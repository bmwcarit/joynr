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
#include "gmock/gmock.h"

#include "joynr/Timer.h"
#include "joynr/joynrlogging.h"

#include <QString>

#include <chrono>
#include <thread>
#include <stdint.h>
#include <unordered_map>

using namespace joynr;
using namespace joynr_logging;

using namespace std::chrono;
using namespace std::this_thread;

using namespace ::testing;
using ::testing::StrictMock;

// Expected accuracy of the timer in milliseconds
static const int64_t timerAccuracy_ms = 10;

class TimerTest : public ::testing::Test {
public:

    static joynr_logging::Logger* logger;

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

    Timer::TimerId addTimer(uint64_t ms, bool periodic)
    {
         Timer::TimerId id = timer.addTimer(expired, removed, ms, periodic);
         uint64_t start = TimeUtils::getCurrentMillisSinceEpoch();
         timerStartMapping.emplace(id, start);
         return id;
    }

    // wrapper
    bool removeTimer(Timer::TimerId id) { return timer.removeTimer(id); }

    void timerExpired(Timer::TimerId id)
    {
        const uint64_t now_ms   = TimeUtils::getCurrentMillisSinceEpoch();
        const uint64_t start_ms = timerStartMapping.at(id);
        const int64_t  diff_ms  = now_ms - start_ms;

        LOG_TRACE(TimerTest::logger, QString("Timer %0 expired").arg(id));
        LOG_TRACE(TimerTest::logger, QString("  started    : %0").arg(start_ms));
        LOG_TRACE(TimerTest::logger, QString("  returned   : %0").arg(now_ms));
        LOG_TRACE(TimerTest::logger, QString("  difference : %0").arg(diff_ms));

        onTimerExpired(id, diff_ms);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
    MOCK_CONST_METHOD2(onTimerExpired, void (Timer::TimerId, int64_t));
    MOCK_CONST_METHOD1(onTimerRemoved, void (Timer::TimerId id));
#pragma GCC diagnostic pop

private:

    Timer timer;

    std::function<void(Timer::TimerId)> expired;
    std::function<void(Timer::TimerId)> removed;

    std::unordered_map<Timer::TimerId, uint64_t> timerStartMapping;
};

joynr_logging::Logger* TimerTest::logger = joynr_logging::Logging::getInstance()->getLogger("MSG", "TimerTest");

TEST_F(TimerTest, deinitializationWithoutRun_WaitSomeTime) {
    sleep_for(milliseconds(10));
}

TEST_F(TimerTest, deinitializationWithoutRun_Immediately) {
}

TEST_F(TimerTest, shutdownWithActiveTimer_WaitSomeTime) {

    Timer::TimerId id = addTimer(40, false);

    sleep_for(milliseconds(10));

    EXPECT_CALL(*this, onTimerExpired(_, _)).Times(0);
    EXPECT_CALL(*this, onTimerRemoved(id)).Times(1);
}

TEST_F(TimerTest, shutdownWithActiveTimer_Immediately) {

    Timer::TimerId id = addTimer(100, false);

    EXPECT_CALL(*this, onTimerExpired(_, _)).Times(0);
    EXPECT_CALL(*this, onTimerRemoved(id)).Times(1);

}

TEST_F(TimerTest, testCallbackAndAccuracy) {

    Timer::TimerId id = addTimer(20, false);

    EXPECT_CALL(*this, onTimerExpired(id, Lt(20 + timerAccuracy_ms))).Times(1);
    EXPECT_CALL(*this, onTimerRemoved(id)).Times(0);

    sleep_for(milliseconds(30));

}

TEST_F(TimerTest, reorganizeTimerByAddingAnEarlierTimer) {
    Timer::TimerId id50 = addTimer(50, false);

    sleep_for(milliseconds(10));

    Timer::TimerId id10 = addTimer(10, false);

    EXPECT_CALL(*this, onTimerExpired(id10, Lt(10 + timerAccuracy_ms))).Times(1);
    EXPECT_CALL(*this, onTimerRemoved(_)).Times(0);

    sleep_for(milliseconds(10));

    EXPECT_CALL(*this, onTimerExpired(id50, Lt(50 + timerAccuracy_ms))).Times(1);
    EXPECT_CALL(*this, onTimerRemoved(_)).Times(0);

    sleep_for(milliseconds(40));

}

TEST_F(TimerTest, reorganizeTimerByRemovingTheEarliest) {
    Timer::TimerId id50 = addTimer(50, false);

    Timer::TimerId id10 = addTimer(10, false);

    sleep_for(milliseconds(1));

    EXPECT_CALL(*this, onTimerExpired(_, _)).Times(0);
    EXPECT_CALL(*this, onTimerRemoved(id10)).Times(1);

    removeTimer(id10);

    EXPECT_CALL(*this, onTimerExpired(id50, Lt(50 + timerAccuracy_ms))).Times(1);
    EXPECT_CALL(*this, onTimerRemoved(_)).Times(0);

    sleep_for(milliseconds(70));
}

TEST_F(TimerTest, removingTheOnlyActiveTimer) {
    Timer::TimerId id10 = addTimer(10, false);

    sleep_for(milliseconds(1));

    EXPECT_CALL(*this, onTimerExpired(_, _)).Times(0);
    EXPECT_CALL(*this, onTimerRemoved(id10)).Times(1);

    removeTimer(id10);

    sleep_for(milliseconds(20));

}

TEST_F(TimerTest, usingIntervallTimer) {

    Timer::TimerId id150 = addTimer(150, false);

    Timer::TimerId id10 = addTimer(10, true);

    EXPECT_CALL(*this, onTimerExpired(id10, _)).Times(AtLeast(18));
    EXPECT_CALL(*this, onTimerRemoved(_)).Times(0);

    EXPECT_CALL(*this, onTimerExpired(id150, Lt(150 + timerAccuracy_ms))).Times(1);
    EXPECT_CALL(*this, onTimerRemoved(_)).Times(0);

    sleep_for(milliseconds(200));

    EXPECT_CALL(*this, onTimerRemoved(id10)).Times(1);

}

/* This test is disabled because it could fail sometimes ("Stress-Test") */
TEST_F(TimerTest, DISABLED_stressTest) {

    EXPECT_CALL(*this, onTimerRemoved(_)).Times(0);
    for (int i = 0; i < 500; ++i)
    {
        Timer::TimerId id = addTimer(1, false);
        EXPECT_CALL(*this, onTimerExpired(id, _)).Times(1);
        sleep_for(milliseconds(1));
    }

    sleep_for(milliseconds(300));
}
