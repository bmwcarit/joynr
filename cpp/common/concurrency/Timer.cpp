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

#include <QString>
#include "joynr/joynrlogging.h"

using namespace joynr::joynr_logging;
Logger* joynr::Timer::logger = Logging::getInstance()->getLogger("MSG", "Timer");

joynr::Timer::Timer()
        : currentId(0),
          timers(),
          waitCondition(),
          mutex(),
          keepRunning(true),
          workerThread(&joynr::Timer::runTimer, this)
{
}

joynr::Timer::~Timer()
{
    LOG_TRACE(logger, "Dtor called");
    assert(!keepRunning);
    assert(!workerThread.joinable());
}

joynr::Timer::TimerId joynr::Timer::addTimer(
        std::function<void(joynr::Timer::TimerId)> onTimerExpired,
        std::function<void(joynr::Timer::TimerId)> onActiveTimerRemoved,
        uint64_t msToBeExpired,
        bool periodic)
{
    const milliseconds interval(msToBeExpired);
    TimerData* newTimer = NULL;

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
        LOG_TRACE(logger, QString("New timer %0 has the earliest deadline").arg(currentId));
        waitCondition.notify_one();
    }
    return currentId;
}

bool joynr::Timer::removeTimer(TimerId id)
{
    for (auto it = timers.begin(); it != timers.end(); ++it) {
        if (it->second->getId() == id) {
            const bool reorganize = (it == timers.begin());

            // Send "remove" callback to event receiver
            auto callback = it->second->getRemoveCallback();
            callback(it->second->getId());

            // Call Dtor and remove from map
            {
                std::unique_lock<std::mutex> lock(mutex);
                delete it->second;
                timers.erase(it);
            }

            // Only reorganize if timer is the current timer
            if (reorganize) {
                LOG_TRACE(logger, QString("Reorganize after %0 was removed.").arg(id));
                waitCondition.notify_one();
            }
            return true;
        }
    }
    LOG_TRACE(logger, "Timer not removed");
    return false;
}

void joynr::Timer::shutdown()
{
    LOG_TRACE(logger, "shutdown() called");

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

void joynr::Timer::runTimer()
{
    while (keepRunning) {
        {
            // Wait for initial work / timer
            std::unique_lock<std::mutex> lock(mutex);
            if (timers.empty()) {
                LOG_TRACE(logger, "List of timers is empty. Waiting.");
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
                LOG_TRACE(logger, "Timer conditional wait was interrupted");
            }
        }
    }
    LOG_DEBUG(logger, "Leaving timer loop");
}
