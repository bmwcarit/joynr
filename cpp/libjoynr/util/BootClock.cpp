/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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

#include "joynr/BootClock.h"

#include <ctime>

namespace joynr
{

TimePoint BootClock::now()
{
    struct timespec ts;
    int ret = clock_gettime(CLOCK_BOOTTIME, &ts);
    if (ret == -1) {
        // take regular clock, if bootclock is not available
        return TimePoint::now();
    }
    std::int64_t ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return TimePoint::fromAbsoluteMs(ms);
}

} // namespace joynr
