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

#ifndef MESSAGINGQOS_H
#define MESSAGINGQOS_H

#include <cstdint>
#include <string>

#include "joynr/JoynrCommonExport.h"

namespace joynr
{

/**
  * Data Class that stores QoS Settings like Ttl
  */
class JOYNRCOMMON_EXPORT MessagingQos
{
public:
    MessagingQos(uint64_t ttl = 60000);
    MessagingQos(const MessagingQos& other);
    virtual ~MessagingQos() = default;

    virtual std::string toString() const;

    uint64_t getTtl() const;
    void setTtl(const uint64_t& ttl);

    MessagingQos& operator=(const MessagingQos& other) = default;
    bool operator==(const MessagingQos& other) const;

private:
    uint64_t ttl;

    // printing MessagingQos with google-test and google-mock
    friend void PrintTo(const MessagingQos& messagingQos, ::std::ostream* os);
};

// printing MessagingQos with google-test and google-mock
void PrintTo(const joynr::MessagingQos& value, ::std::ostream* os);

} // namespace joynr
#endif // MESSAGINGQOS_H
