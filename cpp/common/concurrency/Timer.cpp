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
#include <concurrency/TimerData.h>
#include "joynr/Timer.h"
#include <tuple>
#include <chrono>
#include <cassert>

namespace joynr
{

INIT_LOGGER(Timer);

Timer::Timer()
        : currentId(0),
          timers(),
          waitCondition(),
          mutex(),
          keepRunning(true),
          workerThread(&Timer::runTimer, this)
{
}

Timer::~Timer()
{
    JOYNR_LOG_TRACE(logger, "Dtor called");
    assert(!keepRunning);
    assert(!workerThread.joinable());
}

Timer::TimerId Timer::addTimer(std::function<void(Timer::TimerId)> onTimerExpired,
                               std::function<void(Timer::TimerId)> onActiveTimerRemoved,
                               uint64_t msToBeExpired,
                               bool periodic)
{
    const milliseconds interval(msToBeExpired);
    TimerData* newTimer = nullptr;

    {
        std::unique_lock<std::mutex> lock(mutex);
        if (periodic) {
            newTimer = new PeriodicTimerData(
                    ++currentId, onTimerExpired, onActiveTimerRemoved, interval);
        } else {
            newTimer = new OneShotTimerData(
                    ++currentId, onTimerExpired, onActiveTimerRemoved, interval);
        }
        timers.emplace(newTimer->getNextExpiry(), newTimer);
    }

    // If the new timer the next timer in the list we need to reorganize
    if (timers.begin()->second == newTimer) {
        JOYNR_LOG_TRACE(logger, "New timer {} has the earliest deadline", currentId);
        waitCondition.notify_one();
    }
    return currentId;
}

bool Timer::removeTimer(TimerId id)
{
    bool reorganize = false;
    // Call Dtor and remove from map
    {
        std::unique_lock<std::mutex> lock(mutex);
        auto it = timers.begin();
        while (it != timers.end()) {
            if (it->second->getId() == id) {
                break;
            }
            ++it;
        }
        if (it == timers.end()) {
            JOYNR_LOG_TRACE(logger, "Timer {} not found. Unable to remove.", id);
            return false;
        }
        reorganize = (it == timers.begin());

        // Send "remove" callback to event receiver
        auto callback = it->second->getRemoveCallback();
        callback(it->second->getId());

        delete it->second;
        timers.erase(it);
    }

    // Only reorganize if timer is the current timer
    if (reorganize) {
        JOYNR_LOG_TRACE(logger, "Reorganize after {}  was removed.", id);
        waitCondition.notify_one();
    }
    JOYNR_LOG_TRACE(logger, "Timer {}  removed.", id);
    return true;
}

void Timer::shutdown()
{
    JOYNR_LOG_TRACE(logger, "shutdown() called");

    // shutdown thread to go for shure no other event fires
    keepRunning = false;
    waitCondition.notify_all();
    if (workerThread.joinable()) {
        workerThread.join();
    }

    // Notify event receivers the timer will not expire
    for (auto it = timers.begin(); it != timers.end(); ++it) {
        auto callback = it->second->getRemoveCallback();
        callback(it->second->getId());
        delete it->second;
    }
    timers.clear();
}

void Timer::runTimer()
{
    while (keepRunning) {
        {
            // Wait for initial work / timer
            std::unique_lock<std::mutex> lock(mutex);
            if (timers.empty()) {
                JOYNR_LOG_TRACE(logger, "List of timers is empty. Waiting.");
                waitCondition.wait(lock, [this] { return !timers.empty() || !keepRunning; });
            }
        }

        if (keepRunning && !timers.empty()) {

            // Get the first timer in the sorted map and lock until expiry
            std::unique_lock<std::mutex> lock(mutex);
            const system_clock::time_point& tp = timers.begin()->first;
            if (waitCondition.wait_until(lock, tp) == std::cv_status::timeout) {

                // In case of a race condition we should check if the time
                // point is still valid
                auto it = timers.find(tp);
                if (it == timers.end()) {
                    continue;
                }

                // send callback
                TimerData* const timer = it->second;
                auto callback = it->second->getExpiryCallback();
                callback(it->second->getId());

                // if periodic do reschedule else delete object
                if (timer->isPeriodic()) {
                    timers.emplace(timer->getNextExpiry(), timer);
                } else {
                    delete timer;
                }
                timers.erase(tp);
            } else {
                // Either a new timer with earlier deadline was added or a shutdown was triggered
                JOYNR_LOG_TRACE(logger, "Timer conditional wait was interrupted");
            }
        }
    }
    JOYNR_LOG_DEBUG(logger, "Leaving timer loop");
}

} // namespace joynr
