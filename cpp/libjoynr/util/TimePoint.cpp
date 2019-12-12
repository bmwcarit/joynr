/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

#include "joynr/TimePoint.h"

#include <ctime>

namespace joynr
{

TimePoint::TimePoint() : _timePoint()
{
}

TimePoint::TimePoint(const std::chrono::system_clock::time_point& timePoint) : _timePoint(timePoint)
{
}

TimePoint TimePoint::now()
{
    return TimePoint(std::chrono::system_clock::now());
}

TimePoint TimePoint::fromAbsoluteMs(std::int64_t absoluteMs)
{
    return TimePoint(std::chrono::system_clock::time_point(std::chrono::milliseconds(absoluteMs)));
}

TimePoint TimePoint::fromRelativeMs(std::int64_t relativeMs)
{
    return now() + relativeMs;
}

TimePoint TimePoint::max()
{
    return TimePoint(std::chrono::system_clock::time_point::max());
}

TimePoint TimePoint::min()
{
    return TimePoint(std::chrono::system_clock::time_point::min());
}

std::int64_t TimePoint::toMilliseconds() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(_timePoint.time_since_epoch())
            .count();
}

bool TimePoint::operator<(const TimePoint& rhs) const
{
    return this->_timePoint < rhs._timePoint;
}

bool TimePoint::operator>(const TimePoint& rhs) const
{
    return this->_timePoint > rhs._timePoint;
}

std::chrono::milliseconds TimePoint::operator-(const TimePoint& rhs) const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(this->_timePoint - rhs._timePoint);
}

TimePoint TimePoint::operator+(std::int64_t durationMs) const
{
    const auto result = this->_timePoint + std::chrono::milliseconds(durationMs);
    // check for overflow
    if (durationMs > 0) {
        bool positiveOverflow = result < this->_timePoint;
        if (positiveOverflow) {
            return max();
        }
    } else if (durationMs < 0) {
        bool negativeOverflow = result > this->_timePoint;
        if (negativeOverflow) {
            return min();
        }
    }
    return TimePoint(result);
}

TimePoint TimePoint::operator+(const std::chrono::milliseconds& duration) const
{
    return ((*this) + duration.count());
}

TimePoint TimePoint::operator-(const std::chrono::milliseconds& duration) const
{
    return TimePoint(this->_timePoint - duration);
}

bool TimePoint::operator==(const TimePoint& other) const
{
    return this->toMilliseconds() == other.toMilliseconds();
}

bool TimePoint::operator!=(const TimePoint& other) const
{
    return !(*this == other);
}

std::string TimePoint::toString() const
{
    std::time_t time = std::chrono::system_clock::to_time_t(_timePoint);
    return std::ctime(&time);
}

std::chrono::milliseconds TimePoint::relativeFromNow() const
{
    return *this - TimePoint::now();
}

} // namespace joynr
