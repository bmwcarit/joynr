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
#ifndef DISPATCHERUTILS_H
#define DISPATCHERUTILS_H

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"

#include <chrono>
#include <cstdint>
#include <map>
#include <string>

namespace joynr
{

// UTC time used in Joynr
typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
        JoynrTimePoint;

/**
 * @brief The DispatcherUtils class implements untility functions for ttl manipulation
 */
class JOYNR_EXPORT DispatcherUtils
{
public:
    DispatcherUtils() = default;

    /**
     * @brief convertTtlToAbsoluteTime converts given ttl to UTC time
     * @param ttl_ms time to live given in miliseconds
     * @return UTC time
     */
    static JoynrTimePoint convertTtlToAbsoluteTime(std::int64_t ttl_ms);
    /**
     * @brief getMaxAbsoluteTime
     * @return maximum UTC time
     */
    static JoynrTimePoint getMaxAbsoluteTime();
    /**
     * @brief getMinAbsoluteTime
     * @return
     */
    static JoynrTimePoint getMinAbsoluteTime();
    /**
     * @brief convertAbsoluteTimeToTtl converts given UTC time to ttl
     * @param date UTC time
     * @return ttl in miliseconds
     */
    static std::int64_t convertAbsoluteTimeToTtl(JoynrTimePoint date);
    /**
     * @brief convertAbsoluteTimeToTtlString converts UTC time to ttl string
     * (calculates difference between given UTC time and current time)
     * @param date UTC time
     * @return ttl as string
     */
    static std::string convertAbsoluteTimeToTtlString(JoynrTimePoint date);
    /**
     * @brief convertAbsoluteTimeToTtlString converts UTC time to formated printable string
     * @param date UTC time
     * @return UTC time as formated printable string
     */
    static std::string convertAbsoluteTimeToString(JoynrTimePoint date);

    /**
     * @brief returns the current time in chrono time_point representation
     * @return current time in chrono time_point representation
     */
    static std::chrono::system_clock::time_point now();

    /**
     * @brief returns the current time since epoch in milliseconds
     * @return current time in milliseconds as std::uint64_t
     */
    static std::uint64_t nowInMilliseconds();
    ADD_LOGGER(DispatcherUtils)
};

} // namespace joynr
#endif // DISPATCHERUTILS_H
