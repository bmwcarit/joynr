/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#ifndef MUTABLEMESSAGE_H
#define MUTABLEMESSAGE_H

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/optional.hpp>

#include "joynr/IKeychain.h"
#include "joynr/Message.h"
#include "joynr/TimePoint.h"
#include "joynr/Util.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class ImmutableMessage;

class MutableMessage final
{
public:
    MutableMessage();

    ~MutableMessage() = default;

    MutableMessage(const MutableMessage&) = default;
    MutableMessage& operator=(const MutableMessage&) = default;
    MutableMessage(MutableMessage&&) = default;
    MutableMessage& operator=(MutableMessage&&) = default;

    /**
     * @brief This serializes the contents of this message, then it is optionally signed and
     * encrypted.
     * @return an ImmutableMessage representing a frozen state of this message's content
     */
    std::unique_ptr<ImmutableMessage> getImmutableMessage() const;

    /**
     * @brief Sets Keychain to the message.
     * @param shared pointer to the Keychain
     */
    void setKeychain(std::shared_ptr<IKeychain> keyChain);

    /**
     * @brief Sets the sender of this message.
     * @param sender the participant ID of this message's sender
     */
    void setSender(const std::string& sender);
    void setSender(std::string&& sender);

    /**
     * @brief Gets the sender participant ID of the message.
     * @return the sender of the message, if the header is set; empty string otherwise.
     */
    const std::string& getSender() const;

    /**
     * @brief Sets the recipient of this message.
     * @param recipient the participant ID of this message's recipient
     */
    void setRecipient(const std::string& recipient);
    void setRecipient(std::string&& recipient);

    /**
     * @brief Gets the recipient participant ID of the message.
     * @return the recipient of the message, if the header is set; empty string otherwise.
     */
    const std::string& getRecipient() const;

    /**
     * @brief Sets the expiry date of the message. If the header is already set,
     * its value is replaced with the new one.
     * @param expiryDate the "expiry date" header to be set on the message.
     */
    void setExpiryDate(const TimePoint& expiryDate);

    /**
     * @brief Gets the expiry date of the message.
     * @return the "expiry date" header of the message, if the header is set;
     * A default-constructed value, otherwise.
     */
    TimePoint getExpiryDate() const;

    /**
     * @brief Sets the type of the message. If the type is
     * already set, its value is replaced with the new one.
     * @param type the type of this message
     */
    void setType(const std::string& type);
    void setType(std::string&& type);

    /**
     * @brief Gets the type of this message
     * @return string containing the message type;
     * default-constructed string if type has not been set
     */
    const std::string& getType() const;

    /**
     * @brief Gets the id of this message
     * @return string containing the message id
     */
    const std::string& getId() const;

    /**
     * @brief Sets the reply address of the message. If the header is
     * already set, its value is replaced with the new one.
     * @param replyTo the "reply address" header to be set on the message.
     * @see Message::HEADER_REPLY_TO()
     */
    void setReplyTo(const std::string& replyTo);
    void setReplyTo(std::string&& replyTo);

    /**
     * @brief Gets the reply address of the message.
     * @return an optional containing the "reply address" header of the message; this optional
     * is not initialized if the header has not been set
     * @see Message::HEADER_REPLY_TO()
     */
    const boost::optional<std::string>& getReplyTo() const;

    /**
     * @brief Sets a custom header for this message.
     * @param key key of the custom header
     * @param value value of the custom header
     */
    void setCustomHeader(const std::string& key, const std::string& value);
    void setCustomHeader(std::string&& key, std::string&& value);

    /**
     * @brief Sets custom headers for this message. The keys in the provided map
     * must already be prefixed with Message::CUSTOM_HEADER_PREFIX.
     * @param prefixedCustomHeaders Custom headers to add.
     */
    void setPrefixedCustomHeaders(
            const std::unordered_map<std::string, std::string>& prefixedCustomHeaders);
    void setPrefixedCustomHeaders(
            std::unordered_map<std::string, std::string>&& prefixedCustomHeaders);

    /**
     * @brief Gets all custom headers that have been set for this message
     * @return a map containing all custom headers of this message
     */
    const std::unordered_map<std::string, std::string>& getCustomHeaders() const;

    /**
     * @brief Gets the value of a custom header.
     * @param key key of the custom header that shall be retrieved
     * @return an optional containing the value for the specified key; this optional is not
     * initialized if a header for this key has not been set
     */
    boost::optional<std::string> getCustomHeader(const std::string& key) const;

    /**
     * @brief Sets the effort to be expent on delivering the message.
     * If the header is already set, its value is replaced with the new one.
     * @param effort the "effort" header to be set on the message.
     * @see Message::HEADER_EFFORT()
     */
    void setEffort(const std::string& effort);
    void setEffort(std::string&& effort);

    /**
     * @brief Gets the effort which should be expent delivering the message.
     * @return an optional containing the "effort" header of the message; this optional is not
     * initialized if the header has not been set
     * @see Message::HEADER_EFFORT()
     */
    const boost::optional<std::string>& getEffort() const;

    /**
     * @brief Sets the payload of this message.
     * If the payload is already set, its value is replaced with the new one.
     * @param payload payload of this message.
     */
    void setPayload(const std::string& payload);
    void setPayload(std::string&& payload);

    /**
     * @brief Gets the payload of this message
     * @return string containing the payload; default-constructed string if payload has not been
     * set
     */
    const std::string& getPayload() const;

    /**
     * @brief  Used to print log messages.
     * @return a string representing this message used for logging.
     */
    std::string toLogMessage() const;

    /**
     * @return Returns whether the message is send to a provider that is registered on the local
     * cluster-controller.
     */
    bool isLocalMessage() const;

    /**
     * Sets a flag which indiciates whether the message is send to a provider that is registered on
     * the local cluster-controller.
     * Won't be transmitted over the network (transient flag). Default is false.
     * @param localMessage
     */
    void setLocalMessage(bool _localMessage);

    /**
     * @brief Sets the encrypt flag
     * @param encrypt specifies whether message will be sent encrypted
     */
    void setEncrypt(bool encrypt);

    /**
     * @brief Gets the encrypt flag
     * @return whether message will be sent encrypted
     */
    bool getEncrypt() const;

    /**
     * @brief Sets the compress flag
     * @param compress specifies whether message will be sent compressed
     */
    void setCompress(bool compress);

    /**
     * @brief Gets the compress flag
     * @return whether message will be sent compressed
     */
    bool getCompress() const;

    template <typename Archive>
    void save(Archive& archive)
    {
        const auto expiryDate = this->_expiryDate.toMilliseconds();
        const std::string effort = this->_effort.get_value_or(std::string());
        const std::string replyTo = this->_replyTo.get_value_or(std::string());
        archive(MUESLI_NVP(_sender),
                MUESLI_NVP(_recipient),
                MUESLI_NVP(expiryDate),
                MUESLI_NVP(_type),
                MUESLI_NVP(_id),
                MUESLI_NVP(replyTo),
                MUESLI_NVP(effort),
                MUESLI_NVP(_customHeaders),
                MUESLI_NVP(_payload));
    }

private:
    std::string _sender;
    std::string _recipient;
    std::shared_ptr<IKeychain> _keyChain;
    TimePoint _expiryDate;
    std::string _type;
    std::string _id;
    boost::optional<std::string> _replyTo;
    boost::optional<std::string> _effort;
    std::unordered_map<std::string, std::string> _customHeaders;
    std::string _payload;
    bool _ttlAbsolute;

    // Transient flag which marks message that are send to a provider which is registered
    // on the local cluster-controller.
    bool _localMessage;

    /** @brief Specifies whether message will be sent encrypted */
    bool _encrypt;

    /** @brief Specifies whether message will be sent compressed */
    bool _compress;
};

} // namespace joynr

#endif // MUTABLEMESSAGE_H
