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
#ifndef TIMEPOINT_H
#define TIMEPOINT_H

#include <chrono>
#include <cstdint>
#include <string>

#include "joynr/JoynrExport.h"

namespace joynr
{

class JOYNR_EXPORT TimePoint
{
public:
    TimePoint();
    explicit TimePoint(const std::chrono::system_clock::time_point& timePoint);

    ~TimePoint() = default;
    static TimePoint now();
    static TimePoint fromAbsoluteMs(std::int64_t absoluteMs);
    static TimePoint fromRelativeMs(std::int64_t relativeMs);
    static TimePoint max();
    static TimePoint min();

    bool operator<(const TimePoint& rhs) const;
    bool operator>(const TimePoint& rhs) const;

    std::chrono::milliseconds operator-(const TimePoint& rhs) const;
    TimePoint operator+(std::int64_t durationMs) const;
    TimePoint operator+(const std::chrono::milliseconds& duration) const;
    TimePoint operator-(const std::chrono::milliseconds& duration) const;
    bool operator==(const TimePoint& other) const;
    bool operator!=(const TimePoint& other) const;

    std::chrono::milliseconds relativeFromNow() const;
    std::int64_t toMilliseconds() const;
    std::string toString() const;

private:
    std::chrono::system_clock::time_point _timePoint;
};
} // namespace joynr

#endif // TIMEPOINT_H
