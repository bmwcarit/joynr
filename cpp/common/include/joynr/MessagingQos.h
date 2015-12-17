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
  * @brief Class for messaging quality of service settings
  */
class JOYNRCOMMON_EXPORT MessagingQos
{
public:
    /**
     * @brief Base constructor
     * @param ttl The time to live in milliseconds
     */
    MessagingQos(uint64_t ttl = 60000);
    /** @brief Copy constructor */
    MessagingQos(const MessagingQos& other) = default;

    /** @brief Destructor */
    virtual ~MessagingQos() = default;

    /**
     * @brief Stringifies the class
     * @return stringified class content
     */
    virtual std::string toString() const;

    /**
     * @brief Gets the current time to live settings
     * @return time to live in milliseconds
     */
    uint64_t getTtl() const;

    /**
     * @brief Sets the time to live
     * @param ttl Time to live in milliseconds
     */
    void setTtl(const uint64_t& ttl);

    /** @brief assignment operator */
    MessagingQos& operator=(const MessagingQos& other) = default;
    /** @brief equality operator */
    bool operator==(const MessagingQos& other) const;

private:
    /** @brief The time to live in milliseconds */
    uint64_t ttl;

    /**
      * @brief printing MessagingQos with google-test and google-mock
      * @param messagingQos the object to be printed
      * @param os the destination output stream the print should go into
      */
    friend void PrintTo(const MessagingQos& messagingQos, ::std::ostream* os);
};

// printing MessagingQos with google-test and google-mock
/**
 * @brief Print values of MessagingQos object
 * @param messagingQos The current object instance
 * @param os The output stream to send the output to
 */
void PrintTo(const joynr::MessagingQos& messagingQos, ::std::ostream* os);

} // namespace joynr
#endif // MESSAGINGQOS_H
