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
#ifndef TIMER_H_
#define TIMER_H_

#include "joynr/JoynrCommonExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Logger.h"

#include <stdint.h>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

using namespace std::chrono;

namespace joynr
{
class TimerData;
/**
 * @class Timer
 * @brief Implementation of a single threaded timer container
 */
class JOYNRCOMMON_EXPORT Timer
{
public:
    /*! Identifier for an active timer */
    typedef uint64_t TimerId;

    /**
     * @brief Constructor
     */
    Timer();

    /**
     * @brief Destructor
     * @note Before the object could be deleted, you will need to call @ref shutdown
     */
    virtual ~Timer();

    /**
     * Adds a new timer
     * @param onTimerExpired Function to be called if the timer expires
     * @param onActiveTimerRemoved Function to be called if the timer was
     *      removed while it was active
     * @param msToBeExpired Duration of the timer
     * @param periodic Flag defining whether this timer should be fired once
     *      or periodic
     * @return ID of the timer. Only valid as long as timer is alive
     *
     * @note If the timer is not periodic, it will be deleted after it got
     *      expired. A periodic timer will stay alive till either @ref shutdown
     *      or @ref removeTimer is called.
     */
    TimerId addTimer(std::function<void(joynr::Timer::TimerId)> onTimerExpired,
                     std::function<void(joynr::Timer::TimerId)> onActiveTimerRemoved,
                     uint64_t msToBeExpired,
                     bool periodic = false);

    /**
     * Removes a already created and still alive timer
     * @param id ID of the timer. This value was returned by @ref addTimer
     * @return If @c true was returned the timer was removed
     */
    bool removeTimer(TimerId id);

    /**
     * Does an ordinary shutdown of all timers and joins the running thread
     */
    virtual void shutdown();

private:
    /*! Disallow copy and assign */
    DISALLOW_COPY_AND_ASSIGN(Timer);

    /*! Loop for the timer */
    void runTimer();

private:
    /*! Logger */
    ADD_LOGGER(Timer);

    /*! Last published ID to be increased by one for every new timer */
    TimerId currentId;

    /*! Sorted map by absolute time_point of active timers */
    std::map<const system_clock::time_point, TimerData*> timers;

    /*! Wait condition used as a timer */
    std::condition_variable waitCondition;

    /*! Write lock */
    std::mutex mutex;

    /*! Flag indicating @ref workerThread to stay in the loop */
    bool keepRunning;

    /*! Thread processing timers and will notify @ref TimerCallback */
    std::thread workerThread;
};

} // namespace joynr

#endif // TIMER_H_
