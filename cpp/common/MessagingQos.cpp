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
#include "joynr/MessagingQos.h"

#include <sstream>

namespace joynr
{

MessagingQos::MessagingQos(const MessagingQos& other) : ttl(other.ttl)
{
}

MessagingQos::MessagingQos(uint64_t ttl) : ttl(ttl)
{
}

uint64_t MessagingQos::getTtl() const
{
    return ttl;
}

void MessagingQos::setTtl(const uint64_t& ttl)
{
    this->ttl = ttl;
}

bool MessagingQos::operator==(const MessagingQos& other) const
{
    return this->getTtl() == other.getTtl();
}

std::string MessagingQos::toString() const
{
    std::ostringstream msgQosAsString;
    msgQosAsString << "MessagingQos{";
    msgQosAsString << "ttl:" << getTtl();
    msgQosAsString << "}";
    return msgQosAsString.str();
}

// printing StdRadioStation with google-test and google-mock
void PrintTo(const MessagingQos& messagingQos, ::std::ostream* os)
{
    *os << "MessagingQos::" << messagingQos.toString();
}

} // namespace joynr
