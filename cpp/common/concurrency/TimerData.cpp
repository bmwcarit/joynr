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
#include "TimerData.h"

namespace joynr
{

TimerData::TimerData(const Timer::TimerId id,
                     std::function<void(Timer::TimerId)> expiryCallback,
                     std::function<void(Timer::TimerId)> removeCallback,
                     bool periodic)
        : periodic(periodic), id(id), expiryCallback(expiryCallback), removeCallback(removeCallback)
{
}

bool TimerData::isPeriodic() const
{
    return periodic;
}

std::function<void(Timer::TimerId)> TimerData::getExpiryCallback() const
{
    return expiryCallback;
}

std::function<void(Timer::TimerId)> TimerData::getRemoveCallback() const
{
    return removeCallback;
}

Timer::TimerId TimerData::getId() const
{
    return id;
}

OneShotTimerData::OneShotTimerData(const Timer::TimerId id,
                                   std::function<void(Timer::TimerId)> expiryCallback,
                                   std::function<void(Timer::TimerId)> removeCallback,
                                   const std::chrono::milliseconds delay)
        : TimerData(id, expiryCallback, removeCallback, false),
          expiry(std::chrono::steady_clock::now() + delay)
{
}

std::chrono::steady_clock::time_point OneShotTimerData::getNextExpiry()
{
    return expiry;
}

PeriodicTimerData::PeriodicTimerData(const Timer::TimerId id,
                                     std::function<void(Timer::TimerId)> expiryCallback,
                                     std::function<void(Timer::TimerId)> removeCallback,
                                     const std::chrono::milliseconds interval)
        : TimerData(id, expiryCallback, removeCallback, true),
          interval(interval),
          counter(0),
          creation(std::chrono::steady_clock::now())
{
}

std::chrono::steady_clock::time_point PeriodicTimerData::getNextExpiry()
{
    return creation + (interval * (++counter));
}

} // namespace joynr
