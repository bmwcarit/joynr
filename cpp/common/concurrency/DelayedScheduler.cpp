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
#include "joynr/DelayedScheduler.h"
#include "joynr/Runnable.h"

#include <cassert>
#include <functional>

namespace joynr
{

INIT_LOGGER(DelayedScheduler);

DelayedScheduler::DelayedScheduler(std::function<void(Runnable*)> onWorkAvailable,
                                   std::chrono::milliseconds defaultDelayMs)
        : defaultDelayMs(defaultDelayMs),
          onWorkAvailable(onWorkAvailable),
          stoppingDelayedScheduler(false),
          timedRunnables(),
          writeLock(),
          timer()
{
}

DelayedScheduler::~DelayedScheduler()
{
    JOYNR_LOG_TRACE(logger, "Dtor called");
    // check if DelayedScheduler::shutdown() was called first
    assert(timedRunnables.empty());
}

DelayedScheduler::RunnableHandle DelayedScheduler::schedule(Runnable* runnable,
                                                            std::chrono::milliseconds delay)
{
    JOYNR_LOG_TRACE(logger, "schedule: enter with {} ms delay", delay.count());

    if (stoppingDelayedScheduler) {
        if (runnable->isDeleteOnExit()) {
            delete runnable;
        }
        return INVALID_RUNNABLE_HANDLE;
    }

    if (delay == std::chrono::milliseconds::zero()) {
        JOYNR_LOG_TRACE(logger, "Forward runnable directly (no delay)");
        onWorkAvailable(runnable);
        return INVALID_RUNNABLE_HANDLE;
    }

    using std::placeholders::_1;
    RunnableHandle currentHandle =
            timer.addTimer(std::bind(&DelayedScheduler::timerForRunnableExpired, this, _1),
                           std::bind(&DelayedScheduler::timerForRunnableRemoved, this, _1),
                           delay.count(),
                           false);

    JOYNR_LOG_TRACE(logger, "Added timer with ID {}", currentHandle);

    std::lock_guard<std::mutex> lock(writeLock);
    timedRunnables.emplace(currentHandle, runnable);

    return currentHandle;
}

DelayedScheduler::RunnableHandle DelayedScheduler::schedule(Runnable* runnable)
{
    return schedule(runnable, defaultDelayMs);
}

void DelayedScheduler::unschedule(const RunnableHandle runnableHandle)
{
    if (runnableHandle == INVALID_RUNNABLE_HANDLE) {
        JOYNR_LOG_WARN(logger, "unschedule() called with invalid runnable handle");
        return;
    }

    if (!timer.removeTimer(runnableHandle)) {
        JOYNR_LOG_TRACE(logger, "Failed to remove timer {}", runnableHandle);
    }

    std::lock_guard<std::mutex> lock(writeLock);
    auto it = timedRunnables.find(runnableHandle);
    if (it == timedRunnables.end()) {
        JOYNR_LOG_WARN(logger, "Timed runnable with ID {} not found.", runnableHandle);
        return;
    }

    Runnable* runnable = it->second;
    if (runnable != nullptr && runnable->isDeleteOnExit()) {
        delete runnable;
    }
    timedRunnables.erase(it);

    JOYNR_LOG_TRACE(logger, "runnable with handle {} unscheduled", runnableHandle);
}

void DelayedScheduler::shutdown()
{
    JOYNR_LOG_TRACE(logger, "Shutdown called");
    timer.shutdown();
    {
        std::lock_guard<std::mutex> lock(writeLock);
        stoppingDelayedScheduler = true;

        for (auto it = timedRunnables.begin(); it != timedRunnables.end(); ++it) {
            if (it->second != nullptr && it->second->isDeleteOnExit()) {
                delete it->second;
                it->second = nullptr;
            }
        }
        timedRunnables.clear();
    }
}

void DelayedScheduler::timerForRunnableExpired(Timer::TimerId timerId)
{
    JOYNR_LOG_TRACE(logger, "timerForRunnableExpired({})", timerId);

    std::lock_guard<std::mutex> lock(writeLock);
    auto it = timedRunnables.find(timerId);
    if (it == timedRunnables.end()) {
        JOYNR_LOG_WARN(logger, "Timed runnable with ID {} not found.", timerId);
        return;
    }
    Runnable* tmp = it->second;
    if (tmp != nullptr) {
        if (stoppingDelayedScheduler) {
            if (tmp->isDeleteOnExit()) {
                delete tmp;
            }

        } else {
            onWorkAvailable(tmp);
        }
        timedRunnables.erase(it);
    }
}

void DelayedScheduler::timerForRunnableRemoved(Timer::TimerId timerId)
{
    JOYNR_LOG_INFO(logger, "timerForRunnableRemoved({}). Doing a cleanup", timerId);

    std::lock_guard<std::mutex> lock(writeLock);
    auto it = timedRunnables.find(timerId);
    if (it == timedRunnables.end()) {
        JOYNR_LOG_WARN(logger, "Timed runnable with ID {} not found.", timerId);
        return;
    }
    Runnable* tmp = it->second;
    if (tmp != nullptr && tmp->isDeleteOnExit()) {
        delete it->second;
        it->second = nullptr;
    }
    timedRunnables.erase(it);
}

} // namespace joynr
