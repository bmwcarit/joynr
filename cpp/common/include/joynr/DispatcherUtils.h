/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

#include "joynr/JoynrCommonExport.h"

#include "joynr/DeclareMetatypeUtil.h"
#include "joynr/joynrlogging.h"

#include <stdint.h>
#include <string>
#include <chrono>

namespace joynr
{

// UTC time used in Joynr
typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
        JoynrTimePoint;

class JoynrMessage;

/**
 * @brief The DispatcherUtils class implements untility functions for ttl manipulation
 */
class JOYNRCOMMON_EXPORT DispatcherUtils
{
public:
    DispatcherUtils();
    // todo some of those could be moved  to other classes (e.g. a HeaderMap Dataclass)
    typedef QMap<QString, QVariant>
            HeaderMap; // todo refactor this,  remove Headermap and create dataclass

    /**
     * @brief convertTtlToAbsoluteTime converts given ttl to UTC time
     * @param ttl_ms time to live given in miliseconds
     * @return UTC time
     */
    static JoynrTimePoint convertTtlToAbsoluteTime(int64_t ttl_ms);
    /**
     * @brief getMaxAbsoluteTime
     * @return maximum UTC time
     */
    static JoynrTimePoint getMaxAbsoluteTime();
    /**
     * @brief convertAbsoluteTimeToTtl converts given UTC time to ttl
     * @param date UTC time
     * @return ttl in miliseconds
     */
    static int64_t convertAbsoluteTimeToTtl(JoynrTimePoint date);
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

    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // DISPATCHERUTILS_H
