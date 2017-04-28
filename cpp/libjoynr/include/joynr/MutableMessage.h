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

#include "joynr/DispatcherUtils.h"
#include "joynr/Message.h"
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

    std::unique_ptr<ImmutableMessage> getImmutableMessage() const;

    void setSender(const std::string& sender);

    void setSender(std::string&& sender);

    /**
     * @brief Gets the sender participant ID of the message.
     * @return the sender of the message, if the header is set; empty string otherwise.
     */
    const std::string& getSender() const;

    void setRecipient(const std::string& recipient);

    void setRecipient(std::string&& recipient);

    /**
     * @brief Gets the recipient participant ID of the message.
     * @return the recipient of the message, if the header is set; empty string otherwise.
     */
    const std::string& getRecipient() const;

    void setExpiryDate(JoynrTimePoint expiryDate);

    JoynrTimePoint getExpiryDate() const;

    const std::unordered_map<std::string, std::string>& getCustomHeaders() const;

    boost::optional<std::string> getCustomHeader(const std::string& key) const;

    void setType(const std::string& type);

    void setType(std::string&& type);

    const std::string& getType() const;

    const std::string& getId() const;

    void setReplyTo(const std::string& replyTo);

    void setReplyTo(std::string&& replyTo);

    const boost::optional<std::string>& getReplyTo() const;

    void setCustomHeader(const std::string& key, const std::string& value);

    void setCustomHeader(std::string&& key, std::string&& value);

    void setEffort(const std::string& effort);

    void setEffort(std::string&& effort);

    const boost::optional<std::string>& getEffort() const;

    void setPayload(const std::string& payload);

    void setPayload(std::string&& payload);

    const std::string& getPayload() const;

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
    void setLocalMessage(bool localMessage);

    template <typename Archive>
    void save(Archive& archive)
    {
        const auto expiryDate = DispatcherUtils::convertAbsoluteTimeToTtl(this->expiryDate);
        const std::string effort = this->effort.get_value_or(std::string());
        const std::string replyTo = this->replyTo.get_value_or(std::string());
        archive(MUESLI_NVP(sender),
                MUESLI_NVP(recipient),
                MUESLI_NVP(expiryDate),
                MUESLI_NVP(type),
                MUESLI_NVP(id),
                MUESLI_NVP(replyTo),
                MUESLI_NVP(effort),
                MUESLI_NVP(customHeaders),
                MUESLI_NVP(payload));
    }

private:
    std::string sender;
    std::string recipient;
    JoynrTimePoint expiryDate;
    std::string type;
    std::string id;
    boost::optional<std::string> replyTo;
    boost::optional<std::string> effort;
    std::unordered_map<std::string, std::string> customHeaders;
    std::string payload;
    bool ttlAbsolute;

    // Transient flag which marks message that are send to a provider which is registered
    // on the local cluster-controller.
    bool localMessage;
};

} // namespace joynr

#endif // MUTABLEMESSAGE_H
