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

#include <cstddef>
#include <string>
#include <unordered_map>

#include <boost/optional.hpp>
#include <smrf/ByteVector.h>
#include <smrf/MessageDeserializer.h>

#include "joynr/Logger.h"
#include "joynr/TimePoint.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

struct RequiredHeaders
{
    std::string id;
    std::string type;
    static constexpr std::size_t NUM_REQUIRED_HEADERS = 2;
};

class ImmutableMessage
{
public:
    explicit ImmutableMessage(smrf::ByteVector&& serializedMessage, bool verifyInput = true);

    explicit ImmutableMessage(const smrf::ByteVector& serializedMessage, bool verifyInput = true);

    // _messageDeserializer has deleted move constructor
    // ImmutableMessage(ImmutableMessage&&) = default;
    // ImmutableMessage& operator=(ImmutableMessage&&) = default;

    ~ImmutableMessage() = default;

    std::string getSender() const;

    std::string getRecipient() const;

    bool isTtlAbsolute() const;

    const std::unordered_map<std::string, std::string>& getHeaders() const;
    std::unordered_map<std::string, std::string> getPrefixedCustomHeaders() const;
    std::unordered_map<std::string, std::string> getCustomHeaders() const;

    bool isEncrypted() const;

    bool isSigned() const;

    bool isCompressed() const;

    smrf::ByteArrayView getUnencryptedBody() const;

    std::string toLogMessage() const;

    const std::string& getType() const;

    const std::string& getId() const;

    boost::optional<std::string> getReplyTo() const;

    boost::optional<std::string> getEffort() const;

    TimePoint getExpiryDate() const;

    const smrf::ByteVector& getSerializedMessage() const;

    std::size_t getMessageSize() const;

    smrf::ByteArrayView getSignature() const;

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
    void setReceivedFromGlobal(bool recFromGlobal);

    void setCreator(const std::string& creatorLocal);

    void setCreator(std::string&& creatorLocal);

    const std::string& getCreator() const;

    std::string getTrackingInfo() const;

    template <typename Archive>
    void save(Archive& archive)
    {
        const auto& recipient = getRecipient();
        const auto& sender = getSender();
        const auto expiryDate = _messageDeserializer.getTtlMs();
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

    bool isAccessControlChecked() const;
    void setAccessControlChecked();

private:
    boost::optional<std::string> getOptionalHeaderByKey(const std::string& key) const;

    void init();
    bool isCustomHeaderKey(const std::string& key) const;

    smrf::ByteVector _serializedMessage;
    smrf::MessageDeserializer _messageDeserializer;
    std::unordered_map<std::string, std::string> headers;
    mutable boost::optional<smrf::ByteArrayView> _bodyView;
    mutable boost::optional<smrf::ByteVector> _decompressedBody;

    // receivedFromGlobal is a transient attribute which will not be serialized.
    // It is only used locally for routing decisions.
    bool receivedFromGlobal;
    std::atomic<bool> _accessControlChecked;

    std::string creator;
    RequiredHeaders _requiredHeaders;
    ADD_LOGGER(ImmutableMessage)
};

} // namespace joynr

#endif // IMMUTABLEMESSAGE_H
