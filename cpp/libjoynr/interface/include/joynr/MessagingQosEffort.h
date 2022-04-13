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

#ifndef MESSAGINGQOS_EFFORT_H
#define MESSAGINGQOS_EFFORT_H

#include <iosfwd>

#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

struct MessagingQosEffort
{
    enum class Enum { BEST_EFFORT = 0, NORMAL = 1 };

    static std::string getLiteral(const MessagingQosEffort::Enum& value)
    {
        switch (value) {
        case Enum::BEST_EFFORT:
            return "BEST_EFFORT";
        case Enum::NORMAL:
            return "NORMAL";
        default:
            throw exceptions::JoynrRuntimeException("Invalid messaging QoS effort value");
        }
    }

    static MessagingQosEffort::Enum getEnum(const std::string& effortString)
    {
        if (effortString == std::string("BEST_EFFORT")) {
            return Enum::BEST_EFFORT;
        }
        if (effortString == "NORMAL") {
            return Enum::NORMAL;
        }
        std::stringstream errorMessage(effortString);
        errorMessage << " is unknown literal for MessagingQosEffort";
        throw std::invalid_argument(errorMessage.str());
    }

    friend void PrintTo(const MessagingQosEffort::Enum& messagingQosEffortEnum, ::std::ostream* os);
};

void PrintTo(const MessagingQosEffort::Enum& messagingQosEffortEnum, ::std::ostream* os);
std::ostream& operator<<(std::ostream& os, const MessagingQosEffort::Enum& messagingQosEffortEnum);

} // namespace joynr

#endif /* MESSAGINGQOS_EFFORT_H */
