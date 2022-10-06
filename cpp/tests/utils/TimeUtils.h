/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <chrono>
#include <cstdint>

namespace joynr
{

namespace TimeUtils
{
/**
 * @brief Returns the current time as a relative duration in MS since epoch
 * @return Current time in milliseconds since epoch
 */
inline static std::uint64_t getCurrentMillisSinceEpoch()
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::system_clock::now().time_since_epoch())
                                              .count());
}
} // namespace TimeUtils

} // namespace joynr

#endif // TIMEUTILS_H
