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

#ifndef IMMUTABLEMESSAGE_H
#define IMMUTABLEMESSAGE_H

#include <string>
#include <stdexcept>

#include <boost/optional.hpp>
#include <smrf/ByteVector.h>
#include <smrf/MessageDeserializer.h>

#include "joynr/DispatcherUtils.h"
#include "joynr/Logger.h"
#include "serializer/Serializer.h"

namespace joynr
{

class ImmutableMessage
{
public:
    explicit ImmutableMessage(smrf::ByteVector&& serializedMessage, bool verifyInput = true);

    explicit ImmutableMessage(const smrf::ByteVector& serializedMessage, bool verifyInput = true);

    ImmutableMessage(ImmutableMessage&&) = default;
    ImmutableMessage& operator=(ImmutableMessage&&) = default;

    ~ImmutableMessage() = default;

    std::string getSender() const;

    std::string getRecipient() const;

    bool isTtlAbsolute() const;

    const std::unordered_map<std::string, std::string>& getHeaders() const;

    bool isEncrypted() const;

    bool isSigned() const;

    smrf::ByteArrayView getUnencryptedBody() const;

    std::string toLogMessage() const;

    const std::string& getType() const;

    const std::string& getId() const;

    boost::optional<std::string> getReplyTo() const;

    boost::optional<std::string> getEffort() const;

    JoynrTimePoint getExpiryDate() const;

    const smrf::ByteVector& getSerializedMessage() const;

    std::size_t getMessageSize() const;

    /**
     * @brief Check whether receivedFromGlobal is set (default: false).
     * ReceivedFromGlobal indicates whether a multicast message is originating from the current
     * runtime and should hence be transmitted globally, or whether it is a message received via
     * a global transport middleware and needs to be routed internally.
     * @return true, if receivedFromGlobal is set; false, otherwise
     * @see setReceivedFromGlobal()
     */
    bool isReceivedFromGlobal() const;

    /**
     * @brief set receivedFromGlobal.
     * @param receivedFromGlobal the new value of receivedFromGlobal
     * @see isReceivedFromGlobal()
     */
    void setReceivedFromGlobal(bool receivedFromGlobal);

    void setCreator(const std::string& creator);

    void setCreator(std::string&& creator);

    const std::string& getCreator() const;

    template <typename Archive>
    void save(Archive& archive)
    {
        const auto& recipient = getRecipient();
        const auto& sender = getSender();
        const auto expiryDate = messageDeserializer.getTtlMs();
        smrf::ByteArrayView body = getUnencryptedBody();
        const std::string payload(body.data(), body.data() + body.size());
        archive(MUESLI_NVP(sender),
                MUESLI_NVP(recipient),
                MUESLI_NVP(expiryDate),
                MUESLI_NVP(headers),
                MUESLI_NVP(receivedFromGlobal),
                MUESLI_NVP(creator),
                MUESLI_NVP(payload));
    }

private:
    boost::optional<std::string> getOptionalHeaderByKey(const std::string& key) const;

    void init();

    smrf::ByteVector serializedMessage;
    smrf::MessageDeserializer messageDeserializer;
    std::unordered_map<std::string, std::string> headers;
    mutable boost::optional<smrf::ByteArrayView> bodyView;
    mutable boost::optional<smrf::ByteVector> decompressedBody;

    // receivedFromGlobal is a transient attribute which will not be serialized.
    // It is only used locally for routing decisions.
    bool receivedFromGlobal;

    std::string creator;
    std::string id;
    std::string type;
    ADD_LOGGER(ImmutableMessage);
};

} // namespace joynr

#endif // IMMUTABLEMESSAGE_H
