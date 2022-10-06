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

#ifndef MESSAGINGQOS_H
#define MESSAGINGQOS_H

#include <cstdint>
#include <iosfwd>
#include <string>
#include <unordered_map>

#include "joynr/JoynrExport.h"
#include "joynr/MessagingQosEffort.h"

namespace joynr
{

/**
 * @brief Class for messaging quality of service settings
 */
class JOYNR_EXPORT MessagingQos
{
public:
    /**
     * @brief Full constructor
     * @param ttl The time to live in milliseconds
     * @param effort The effort to expend during message delivery
     * @param encrypt Specifies, whether messages will be sent encrypted
     * @param compress Specifies, whether messages will be sent compressed
     */
    explicit MessagingQos(std::uint64_t ttl = _default_ttl,
                          MessagingQosEffort::Enum effort = MessagingQosEffort::Enum::NORMAL,
                          bool encrypt = false,
                          bool compress = false);

    /**
     * @brief Base constructor
     * @param effort The effort to expend during message delivery
     * @param encrypt Specifies, whether messages will be sent encrypted
     */
    explicit MessagingQos(MessagingQosEffort::Enum effort, bool encrypt = false);

    /**
     * @brief Base constructor
     * @param ttl The time to live in milliseconds
     * @param encrypt Specifies, whether messages will be sent encrypted
     */
    explicit MessagingQos(std::uint64_t ttl, bool encrypt);

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
    std::uint64_t getTtl() const;

    /**
     * @brief Sets the time to live
     * @param ttl Time to live in milliseconds
     */
    void setTtl(const std::uint64_t& ttl);

    /**
     * @brief get the effort to expend during message delivery
     */
    MessagingQosEffort::Enum getEffort() const;

    /**
     * @brief set the effort to expend during message delivery
     * @param effort the new value for effort
     */
    void setEffort(const MessagingQosEffort::Enum effort);

    /**
     * @brief Gets the encrypt flag
     * @return whether messages will be sent encrypted
     */
    bool getEncrypt() const;

    /**
     * @brief Sets the encrypt flag
     * @param encrypt specifies, whether messages will be sent encrypted
     */
    void setEncrypt(bool encrypt);

    /**
     * @brief Gets the compress flag
     * @return whether messages will be sent compressed
     */
    bool getCompress() const;

    /**
     * @brief Sets the compress flag
     * @param compress specifies, whether messages will be sent compressed
     */
    void setCompress(bool compress);

    /**
     * @brief Puts a header value for the given header key, replacing an existing value
     * if necessary.
     * @param key the header key for which to put the value.
     * @param value the value to put for the given header key.
     */
    void putCustomMessageHeader(const std::string& key, const std::string& value);

    /**
     * @brief Puts all key/value pairs from the map into the header value map,
     * replacing existing values where necessary. This operation is purely additive.
     * @param values the key/value map to add to the map of custom header values.
     */
    void putAllCustomMessageHeaders(const std::unordered_map<std::string, std::string>& values);

    /**
     * @brief returns the current map of custom message headers.
     * @return the current map of custom message headers.
     */
    const std::unordered_map<std::string, std::string>& getCustomMessageHeaders() const;

    /** @brief assignment operator */
    MessagingQos& operator=(const MessagingQos& other) = default;
    /** @brief equality operator */
    bool operator==(const MessagingQos& other) const;

private:
    /** @brief The default time to live in milliseconds */
    const static std::uint64_t _default_ttl = 60000;

    /** @brief The time to live in milliseconds */
    std::uint64_t _ttl;

    /** @brief The effort to expend during message delivery */
    MessagingQosEffort::Enum _effort;

    /** @brief Specifies, whether messages will be sent encrypted */
    bool _encrypt;

    /** @brief Specifies, whether messages will be sent compressed */
    bool _compress;

    /** @brief The map of custom message headers */
    std::unordered_map<std::string, std::string> _messageHeaders;

    /**
     * @brief Checks that the key and value of a custom message header contain
     * only valid characters and conform to the relevant patterns.
     * @param key may contain ascii alphanumeric or hyphen.
     * @param value may contain alphanumeric, space, semi-colon, colon, comma,
     * plus, ampersand, question mark, hyphen, dot, star, forward slash and back slash.
     */
    void checkCustomHeaderKeyValue(const std::string& key, const std::string& value) const;

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
void PrintTo(const MessagingQos& messagingQos, ::std::ostream* os);

std::ostream& operator<<(std::ostream& os, const MessagingQos& messagingQos);

} // namespace joynr
#endif // MESSAGINGQOS_H
