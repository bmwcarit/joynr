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

#include "joynr/joynrlogging.h"

#include <cassert>
#include <functional>

using namespace std::placeholders;

namespace joynr
{

using namespace joynr_logging;
Logger* DelayedScheduler::logger = Logging::getInstance()->getLogger("MSG", "DelayedScheduler");

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
    LOG_TRACE(logger, "Dtor called");
    // check if DelayedScheduler::shutdown() was called first
    assert(timedRunnables.empty());
}

DelayedScheduler::RunnableHandle DelayedScheduler::schedule(
        Runnable* runnable,
        OptionalDelay optionalDelayMs /* = OptionalDelay::createNull()*/)
{
    if (!optionalDelayMs) {
        optionalDelayMs = OptionalDelay(this->defaultDelayMs);
    }

    LOG_TRACE(logger, FormatString("schedule: enter with %0 ms delay").arg(optionalDelayMs.getValue().count()).str());

    if (stoppingDelayedScheduler) {
        if (runnable->isDeleteOnExit()) {
            delete runnable;
        }
        return INVALID_RUNNABLE_HANDLE;
    }

    if (optionalDelayMs.getValue() == std::chrono::milliseconds::zero()) {
        LOG_TRACE(logger, "Forward runnable directly (no delay)");
        onWorkAvailable(runnable);
        return INVALID_RUNNABLE_HANDLE;
    }

    RunnableHandle currentHandle =
            timer.addTimer(std::bind(&DelayedScheduler::timerForRunnableExpired, this, _1),
                           std::bind(&DelayedScheduler::timerForRunnableRemoved, this, _1),
                           optionalDelayMs.getValue().count(),
                           false);

    LOG_TRACE(logger, FormatString("Added timer with ID %0").arg(currentHandle).str());

    std::lock_guard<std::mutex> lock(writeLock);
    timedRunnables.emplace(currentHandle, runnable);

    return currentHandle;
}

void DelayedScheduler::unschedule(const RunnableHandle runnableHandle)
{
    if (runnableHandle == INVALID_RUNNABLE_HANDLE) {
        LOG_WARN(logger, "unschedule() called with invalid runnable handle");
        return;
    }

    if (!timer.removeTimer(runnableHandle)) {
        LOG_TRACE(logger, FormatString("Failed to remove timer %0").arg(runnableHandle).str());
    }

    std::lock_guard<std::mutex> lock(writeLock);
    auto it = timedRunnables.find(runnableHandle);
    if (it == timedRunnables.end()) {
        LOG_WARN(logger,
                 FormatString("Timed runnable with ID %0 not found.").arg(runnableHandle).str());
        return;
    }

    Runnable* runnable = it->second;
    if (runnable != NULL && runnable->isDeleteOnExit()) {
        delete runnable;
    }
    timedRunnables.erase(it);

    LOG_TRACE(
            logger, FormatString("runnable with handle %0 unscheduled").arg(runnableHandle).str());
}

void DelayedScheduler::shutdown()
{
    LOG_TRACE(logger, "Shutdown called");
    timer.shutdown();
    {
        std::lock_guard<std::mutex> lock(writeLock);
        stoppingDelayedScheduler = true;

        for (auto it = timedRunnables.begin(); it != timedRunnables.end(); ++it) {
            if (it->second != NULL && it->second->isDeleteOnExit()) {
                delete it->second;
                it->second = NULL;
            }
        }
        timedRunnables.clear();
    }
}

void DelayedScheduler::timerForRunnableExpired(Timer::TimerId timerId)
{
    LOG_TRACE(logger, FormatString("timerForRunnableExpired(%0)").arg(timerId).str());

    std::lock_guard<std::mutex> lock(writeLock);
    auto it = timedRunnables.find(timerId);
    if (it == timedRunnables.end()) {
        LOG_WARN(logger, FormatString("Timed runnable with ID %0 not found.").arg(timerId).str());
        return;
    }
    Runnable* tmp = it->second;
    if (tmp != NULL) {
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
    LOG_INFO(logger,
             FormatString("timerForRunnableRemoved(%0). Doing a cleanup").arg(timerId).str());

    std::lock_guard<std::mutex> lock(writeLock);
    auto it = timedRunnables.find(timerId);
    if (it == timedRunnables.end()) {
        LOG_WARN(logger, FormatString("Timed runnable with ID %0 not found.").arg(timerId).str());
        return;
    }
    Runnable* tmp = it->second;
    if (tmp != NULL && tmp->isDeleteOnExit()) {
        delete it->second;
        it->second = NULL;
    }
    timedRunnables.erase(it);
}

} // namespace joynr
