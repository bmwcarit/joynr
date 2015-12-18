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
#include "joynr/TimeUtils.h"

joynr::TimerData::TimerData(const Timer::TimerId id,
                            std::function<void(joynr::Timer::TimerId)> expiryCallback,
                            std::function<void(joynr::Timer::TimerId)> removeCallback,
                            bool periodic)
        : periodic(periodic), id(id), expiryCallback(expiryCallback), removeCallback(removeCallback)
{
}

joynr::TimerData::~TimerData()
{
}

bool joynr::TimerData::isPeriodic() const
{
    return periodic;
}

std::function<void(joynr::Timer::TimerId)> joynr::TimerData::getExpiryCallback() const
{
    return expiryCallback;
}

std::function<void(joynr::Timer::TimerId)> joynr::TimerData::getRemoveCallback() const
{
    return removeCallback;
}

joynr::Timer::TimerId joynr::TimerData::getId() const
{
    return id;
}

joynr::OneShotTimerData::OneShotTimerData(const Timer::TimerId id,
                                          std::function<void(joynr::Timer::TimerId)> expiryCallback,
                                          std::function<void(joynr::Timer::TimerId)> removeCallback,
                                          const milliseconds delay)
        : joynr::TimerData(id, expiryCallback, removeCallback, false),
          expiry(joynr::TimeUtils::getCurrentTime() + delay)
{
}

joynr::OneShotTimerData::~OneShotTimerData()
{
}

system_clock::time_point joynr::OneShotTimerData::getNextExpiry()
{
    return expiry;
}

joynr::PeriodicTimerData::PeriodicTimerData(
        const Timer::TimerId id,
        std::function<void(joynr::Timer::TimerId)> expiryCallback,
        std::function<void(joynr::Timer::TimerId)> removeCallback,
        const milliseconds interval)
        : joynr::TimerData(id, expiryCallback, removeCallback, true),
          interval(interval),
          counter(0),
          creation(joynr::TimeUtils::getCurrentTime())
{
}

joynr::PeriodicTimerData::~PeriodicTimerData()
{
}

system_clock::time_point joynr::PeriodicTimerData::getNextExpiry()
{
    return creation + (interval * (++counter));
}
