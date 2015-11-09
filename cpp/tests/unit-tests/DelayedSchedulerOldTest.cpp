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
#include "joynr/Directory.h"
#include "utils/QThreadSleep.h"
#include <QRunnable>
#include <chrono>
#include <stdint.h>
#include "joynr/DelayedSchedulerOld.h"
#include "joynr/joynrlogging.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr_logging;
using namespace std::chrono;

Logger* logger = Logging::getInstance()->getLogger("MSG", "DelayedSchedulerOldTest");

// Expected accuracy of the timer in milliseconds
static qint64 timerAccuracy_ms = 25;


class DummyRunnable : public QRunnable {
public:
    DummyRunnable(qint64 delay, bool& wasTooEarly) :
        eta_ms(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() + delay),
        wasTooEarly(wasTooEarly)
    {}
    void run() {
        int64_t now_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

        LOG_TRACE(logger, "Running runnable");
        LOG_TRACE(logger, QString(" ETA        : %1").arg(eta_ms));
        LOG_TRACE(logger, QString(" current    : %1").arg(now_ms));
        LOG_TRACE(logger, QString(" difference : %1").arg(now_ms - eta_ms));
        if (now_ms < eta_ms - timerAccuracy_ms) {
            wasTooEarly = true;
        } else {
            wasTooEarly = false;
        }
    }
    qint64 eta_ms; //time at which the runnable should be run
    bool& wasTooEarly;
};

class TriggerRunnable : public QRunnable {
public:
    TriggerRunnable(qint64 delay, bool& triggered) :
        eta_ms(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() + delay),
        triggered(triggered)
    {}
    void run() {
        LOG_TRACE(logger, "Running trigger runnable");
        triggered = true;
    }
    qint64 eta_ms; //time at which the runnable should be run
    bool& triggered;
};

TEST(SingleThreadedDelayedSchedulerOldTest, runnableDoesNotRunTooEarly) {
    LOG_TRACE(logger, "starting test");
    qint64 delay = 500;
    DelayedSchedulerOld* scheduler = new SingleThreadedDelayedSchedulerOld(QString("SingleThreadedDelayedScheduler"));
    bool wasTooEarly(false);
    DummyRunnable* runnable = new DummyRunnable(delay, wasTooEarly);
    LOG_TRACE(logger, "starting runnable");
    scheduler->schedule(runnable, delay);
    LOG_TRACE(logger, "started runnable");
    LOG_TRACE(logger, "starting sleep");
    QThreadSleep::msleep(delay+100);
    LOG_TRACE(logger, "finished sleep");
    EXPECT_FALSE(wasTooEarly);

    // Cleanup the test objects 
    delete scheduler;
}

TEST(SingleThreadedDelayedSchedulerOldTest, runnableCanBeStoppedBeforeRun) {
    LOG_TRACE(logger, "starting test");
    qint64 delay = 300;
    DelayedSchedulerOld* scheduler = new SingleThreadedDelayedSchedulerOld(QString("SingleThreadedDelayedScheduler"));
    bool triggered(false);
    TriggerRunnable* runnable = new TriggerRunnable(delay, triggered);
    LOG_TRACE(logger, "starting runnable");
    scheduler->schedule(runnable, delay);
    LOG_TRACE(logger, "started runnable");
    LOG_TRACE(logger, "starting sleep");
    QThreadSleep::msleep(delay+100);
    LOG_TRACE(logger, "finished sleep");
    EXPECT_TRUE(triggered);

    triggered = false;
    //now let's create a runnable, remove it from the scheduler and expect not to be triggered
    TriggerRunnable* runnableNotToBeTriggered = new TriggerRunnable(delay, triggered);
    LOG_TRACE(logger, "starting runnable");
    quint32 runnableHandle = scheduler->schedule(runnableNotToBeTriggered, delay);

    QThreadSleep::msleep(100);
    LOG_TRACE(logger, "unschedule runnable");
    scheduler->unschedule(runnableHandle);
    LOG_TRACE(logger, "starting sleep");
    QThreadSleep::msleep(delay);
    LOG_TRACE(logger, "finished sleep");
    // runnable shall not be triggered
    EXPECT_FALSE(triggered);

    // Cleanup the test objects
    delete scheduler;
}

TEST(MultiThreadedDelayedSchedulerOldTest, runnableDoesNotRunTooEarly) {
    qint64 delay = 100;
    QThreadPool threadpool;
    threadpool.setMaxThreadCount(5);
    DelayedSchedulerOld* scheduler = new ThreadPoolDelayedSchedulerOld(threadpool, QString("ThreadPoolDelayedScheduler"));
    bool wasTooEarly;
    DummyRunnable* runnable = new DummyRunnable(delay, wasTooEarly);
    scheduler->schedule(runnable, delay);
    QThreadSleep::msleep(delay+500);
    EXPECT_FALSE(wasTooEarly);
    
    // Cleanup the test objects 
    delete scheduler;
}
