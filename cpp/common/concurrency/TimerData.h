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
#ifndef TIMERDATA_H_
#define TIMERDATA_H_
#include <chrono>
#include <joynr/Timer.h>

using namespace std::chrono;

namespace joynr
{

/**
 * @class TimerData
 * @brief Data object used by @ref Timer to store timer configuration
 */
class TimerData
{
public:
    /**
     * @brief Constructor
     * @param id Unique ID of the Timer
     * @param expiryCallback Callback called on expiry
     * @param removeCallback Callback called if timer got removed
     * @param periodic Flag indicating whether timer is periodic or not
     */
    TimerData(const Timer::TimerId id,
              std::function<void(joynr::Timer::TimerId)> expiryCallback,
              std::function<void(joynr::Timer::TimerId)> removeCallback,
              bool periodic);

    /**
     * @brief Destructor
     */
    virtual ~TimerData() = default;

    /**
     * Returns if this timer should be put to queue again
     * @return If @c true the timer needs to be added again to the queue
     */
    bool isPeriodic() const;

    /**
     * Returns the next expire date of the timer
     * @return Next absolute time point the timer will expire
     * @note Calling this method multiple times will always return a different
     *      time point if @ref isPeriodic returns @c true. In case
     *      @ref isPeriodic returns @c false this method will return always the
     *      one expiry date. This date could also be in the past.
     */
    virtual system_clock::time_point getNextExpiry() = 0;

    /**
     * Returns the expiry callback
     * @return The function pointer on expiry
     */
    std::function<void(joynr::Timer::TimerId)> getExpiryCallback() const;

    /**
     * Returns the removal callback
     * @return The function pointer on removal
     */
    std::function<void(joynr::Timer::TimerId)> getRemoveCallback() const;

    /**
     * Returns the unique ID
     * @return ID of the timer
     */
    Timer::TimerId getId() const;

private:
    /*! Flag indicating whether timer is periodic or not */
    const bool periodic;
    /*! Unique ID of the timer */
    const Timer::TimerId id;
    /*! Expiry callback of the timer */
    std::function<void(joynr::Timer::TimerId)> expiryCallback;
    /*! Remove callback of the timer */
    std::function<void(joynr::Timer::TimerId)> removeCallback;
};

/**
 * @class OneShotTimerData
 * @brief Data object used by @ref Timer to store configuration for a timer that is triggered once.
 */
class OneShotTimerData : public TimerData
{
public:
    /**
     * @brief Constructor
     * @param id Unique ID of the Timer
     * @param expiryCallback Callback called on expiry
     * @param removeCallback Callback called if timer got removed
     * @param delay Delay of the timer in milliseconds
     */
    OneShotTimerData(const Timer::TimerId id,
                     std::function<void(joynr::Timer::TimerId)> expiryCallback,
                     std::function<void(joynr::Timer::TimerId)> removeCallback,
                     const milliseconds delay);
    virtual ~OneShotTimerData() = default;

    virtual system_clock::time_point getNextExpiry();

private:
    /*! Time point of exiry */
    const system_clock::time_point expiry;
};

/**
 * @class PeriodicTimerData
 * @brief Data object used by @ref Timer to store configuration for a timer that is triggered
 * periodically.
 */
class PeriodicTimerData : public TimerData
{
public:
    /**
     * @brief Constructor
     * @param id Unique ID of the Timer
     * @param expiryCallback Callback called on expiry
     * @param removeCallback Callback called if timer got removed
     * @param interval Interval of the timer in milliseconds
     */
    PeriodicTimerData(const Timer::TimerId id,
                      std::function<void(joynr::Timer::TimerId)> expiryCallback,
                      std::function<void(joynr::Timer::TimerId)> removeCallback,
                      const milliseconds interval);
    virtual ~PeriodicTimerData() = default;

    virtual system_clock::time_point getNextExpiry();

private:
    /*! The regular interval the timer expires */
    const milliseconds interval;
    /*! Counter the will be increased by each call to @ref getNextExpiry */
    int counter;
    /*! Time point of creation */
    const system_clock::time_point creation;
};

} // namespace joynr

#endif // TIMERDATA_H_
