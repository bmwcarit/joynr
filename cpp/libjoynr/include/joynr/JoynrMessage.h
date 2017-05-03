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
#ifndef JOYNRMESSAGE_H
#define JOYNRMESSAGE_H

#include <map>
#include <string>

#include "joynr/DispatcherUtils.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

/**
  * JoynrMessage describes the message format used to send messages between Joynr applications
  * and a ClusterController as well as between multiple ClusterControllers
  *
  * fields
  *    type (one_way, request, reply, subscription_request, subscription_reply, subscription_stop)
  *    std::map<std::string, std::string> header, contains domain, interface, requestId, replyId,
  *sourceParticpantId,
  *        destinationParicipantId, ...
  *    std::string payload
  * JoynrMessages are serialized in JSON format
  */
class JOYNR_EXPORT JoynrMessage
{

public:
    JoynrMessage();

    /**
     * @brief HEADER_CONTENT_TYPE The "content type" header contains the type information of the
     * message.
     * @return the name/key for the "content type" header.
     */
    static const std::string& HEADER_CONTENT_TYPE();

    /**
     * @brief HEADER_MESSAGE_ID The "message ID" header contains a unique ID of the message.
     * @return the name/key for the "message ID" header.
     */
    static const std::string& HEADER_MESSAGE_ID();
    /**
     * @brief HEADER_CREATOR_USER_ID The "creator user ID" header contains a unique user ID
     * of the message creator.
     * @return the name/key for the "creator user ID" header.
     */
    static const std::string& HEADER_CREATOR_USER_ID();
    /**
     * @brief HEADER_NAME_TO The "to" header stores the receiver participant ID.
     * @return the name/key for the "to" header.
     */
    static const std::string& HEADER_TO();
    /**
     * @brief HEADER_NAME_FROM The "from" header stores the sender participant ID.
     * @return the name/key for the "from" header.
     */
    static const std::string& HEADER_FROM();
    /**
     * @brief HEADER_EXPIRY_DATE The "expiry date" header stores the expiry date of a message. The
     * message will be discarded by any Joynr Messaging component after this date. Make sure clocks
     * are in sync to avoid deletion of messages by mistake.
     * @return the name/key for the "expiry date" header.
     */
    static const std::string& HEADER_EXPIRY_DATE();
    /**
     * @brief HEADER_REPLY_ADDRESS The "reply address" header stores the serialized (json) sender
     * address.
     *
     * Is set in all messages that expect a response to be send by the receiver. When the receiver
     * sends the response, the reply address determines the address to send the response to.
     * @return
     */
    static const std::string& HEADER_REPLY_ADDRESS();
    /**
     * @brief HEADER_EFFOR The "effort" header is set when the effort spent on delivering the
     * message
     * should be changed.
     * @return
     */
    static const std::string& HEADER_EFFORT();

    static const std::string CUSTOM_HEADER_REQUEST_REPLY_ID;
    static const std::string VALUE_MESSAGE_TYPE_ONE_WAY;
    static const std::string VALUE_MESSAGE_TYPE_REQUEST;
    static const std::string VALUE_MESSAGE_TYPE_REPLY;
    static const std::string VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST;
    static const std::string VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST;
    static const std::string VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
    static const std::string
            VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY; // reply that the subscription was
                                                   // registered successfully
    static const std::string VALUE_MESSAGE_TYPE_PUBLICATION;
    static const std::string VALUE_MESSAGE_TYPE_MULTICAST;
    static const std::string VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP;

    static const std::string VALUE_CONTENT_TYPE_TEXT_PLAIN;
    static const std::string VALUE_CONTENT_TYPE_APPLICATION_JSON;

    JoynrMessage(const JoynrMessage& message);
    JoynrMessage& operator=(const JoynrMessage& message);

    JoynrMessage(JoynrMessage&& message);
    JoynrMessage& operator=(JoynrMessage&& message);

    bool operator==(const JoynrMessage& message) const;

    std::string getType() const;
    void setType(const std::string& type);
    std::map<std::string, std::string> getHeader() const;
    /**
     * @brief JoynrMessage::setHeader Adds header entries to the already existing header map.
     * If a header entry was already set, its value is replaced with the new one.
     * @param newHeaders the header entries to add
     */
    void setHeader(const std::map<std::string, std::string>& newHeaders);
    std::string getPayload() const;
    void setPayload(const std::string& payload);
    void setPayload(std::string&& payload);

    /**
     * @brief determines if a custom header with the given key is present
     * in the headers.
     * @return true if a custom header value is present for the key, otherwise false.
     */
    bool containsCustomHeader(const std::string& key) const;

    /**
     * @brief JoynrMessage::getCustomHeader retrieves the value for a custom
     * header previously added with JoynrMessage::setCustomHeader.
     * @param key the key of the custom header value to get.
     */
    std::string getCustomHeader(const std::string& key) const;

    /**
     * @brief JoynrMessage::setCustomHeader allows setting arbitrary header values for the
     * message. The header key will be prefixed with 'custom-' when being set.
     * @param key the key for the header to set.
     * @param value the value to set for the header.
     */
    void setCustomHeader(const std::string& key, const std::string& value);

    /**
     * @brief containsHeaderContentType Tests whether the "content type" header of the message is
     * set or not.
     * @return true, if the "content type" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_CONTENT_TYPE()
     */
    bool containsHeaderContentType() const;
    /**
     * @brief getHeaderContentType Gets the content type of the message. Use
     * JoynrMessage::containsHeaderContentType()
     * to check whether the header is available or not.
     * @return the "content type" header of the message, if the header is set; A default-constructed
     * value, otherwise.
     * @see JoynrMessage::HEADER_CONTENT_TYPE()
     */
    std::string getHeaderContentType() const;
    /**
     * @brief setHeaderContentType Sets the content type of the message. If the header is already
     * set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderContentType() to check
     * whether the header is
     * already set or not.
     * @param contentType the "content type" header to be set on the message.
     * @see JoynrMessage::HEADER_CONTENT_TYPE()
     */
    void setHeaderContentType(const std::string& contentType);

    /**
     * @brief containsHeaderMessageId Tests whether the "message ID" header of the message is set or
     * not.
     * @return true, if the "message ID" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_MESSAGE_ID()
     */
    bool containsHeaderMessageId() const;
    /**
     * @brief getHeaderMessageId Gets the ID of the message. Use
     * JoynrMessage::containsHeaderMessageId()
     * to check whether the header is available or not.
     * @return the "message ID" header of the message, if the header is set; A default-constructed
     * value, otherwise.
     * @see JoynrMessage::HEADER_MESSAGE_ID()
     */
    std::string getHeaderMessageId() const;
    /**
     * @brief setHeaderMessageId Sets the ID of the message. If the header is already set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderMessageId() to check
     * whether the header is
     * already set or not.
     * @param msgId the "message ID" header to be set on the message.
     * @see JoynrMessage::HEADER_MESSAGE_ID()
     */
    void setHeaderMessageId(const std::string& msgId);

    /**
     * @brief containsHeaderCreatorUserId Tests whether the "creator user ID" header of the message
     * is set or not.
     * @return true, if the "creator user ID" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_CREATOR_USER_ID()
     */
    bool containsHeaderCreatorUserId() const;
    /**
     * @brief getHeaderCreatorUserId Gets the message creator user ID. Use
     * JoynrMessage::containsHeaderCreatorUserId() to check whether the header is available or not.
     * @return the "creator user ID" header of the message.
     * @see JoynrMessage::HEADER_CREATOR_USER_ID()
     */
    std::string getHeaderCreatorUserId() const;
    /**
     * @brief setHeaderCreatorUserId Sets the ID of the message. If the header is already set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderCreatorUserId() to check
     * whether the header is already set or not.
     * @param creatorUserId the "creator user ID" header to be set on the message.
     * @see JoynrMessage::HEADER_CREATOR_USER_ID()
     */
    void setHeaderCreatorUserId(const std::string& creatorUserId);
    /**
     * @brief containsHeaderTo Tests whether the "to" header of the message is set or not.
     * @return true, if the "to" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_TO()
     */
    bool containsHeaderTo() const;
    /**
     * @brief getHeaderTo Gets the receiver participant ID of the message. Use
     * JoynrMessage::containsHeaderTo()
     * to check whether the header is available or not.
     * @return the "to" header of the message, if the header is set; A default-constructed value,
     * otherwise.
     * @see JoynrMessage::HEADER_TO()
     */
    std::string getHeaderTo() const;
    /**
     * @brief setHeaderTo Sets the receiver participant ID of the message. If the header is already
     * set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderTo() to check whether the
     * header is
     * already set or not.
     * @param to the "to" header to be set on the message.
     * @see JoynrMessage::HEADER_TO()
     */
    void setHeaderTo(const std::string& to);

    /**
     * @brief containsHeaderFrom Tests whether the "from" header of the message is set or not.
     * @return true, if the "from" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_FROM()
     */
    bool containsHeaderFrom() const;
    /**
     * @brief getHeaderFrom Gets the sender participant ID of the message. Use
     * JoynrMessage::containsHeaderFrom()
     * to check whether the header is available or not.
     * @return the "from" header of the message, if the header is set; A default-constructed value,
     * otherwise.
     * @see JoynrMessage::HEADER_FROM()
     */
    std::string getHeaderFrom() const;
    /**
     * @brief setHeaderFrom Sets the sender participant ID of the message. If the header is already
     * set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderFrom() to check whether
     * the header is
     * already set or not.
     * @param from the "from" header to be set on the message.
     * @see JoynrMessage::HEADER_FROM()
     */
    void setHeaderFrom(const std::string& from);

    /**
     * @brief containsHeaderExpiryDate Tests whether the "expiry date" header of the message is set
     * or not.
     * @return true, if the "expiry date" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_EXPIRY_DATE()
     */
    bool containsHeaderExpiryDate() const;
    /**
     * @brief getHeaderExpiryDate Gets the expiry date of the message. Use
     * JoynrMessage::containsHeaderExpiryDate()
     * to check whether the header is available or not.
     * @return the "expiry date" header of the message, if the header is set; A default-constructed
     * value, otherwise.
     * @see JoynrMessage::HEADER_EXPIRY_DATE()
     */
    JoynrTimePoint getHeaderExpiryDate() const;
    /**
     * @brief setHeaderExpiryDate Sets the expiry date of the message. If the header is already set,
     * its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderExpiryDate() to check
     * whether the header is
     * already set or not.
     * @param expiryDate the "expiry date" header to be set on the message.
     * @see JoynrMessage::HEADER_EXPIRY_DATE()
     */
    void setHeaderExpiryDate(const JoynrTimePoint& expiryDate);

    /**
     * @brief containsHeaderReplyAddress Tests whether the "reply address" header of the
     * message is set or not.
     * @return true, if the "reply address" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_REPLY_ADDRESS()
     */
    bool containsHeaderReplyAddress() const;
    /**
     * @brief getHeaderReplyAddress Gets the reply address of the message. Use
     * JoynrMessage::containsHeaderReplyAddress()
     * to check whether the header is available or not.
     * @return the "reply address" header of the message, if the header is set; A
     * default-constructed value, otherwise.
     * @see JoynrMessage::HEADER_REPLY_ADDRESS()
     */
    std::string getHeaderReplyAddress() const;
    /**
     * @brief setHeaderReplyAddress Sets the reply address of the message. If the header is
     * already set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderReplyAddress() to check
     * whether the header is
     * already set or not.
     * @param replyAddress the "reply address" header to be set on the message.
     * @see JoynrMessage::HEADER_REPLY_ADDRESS()
     */
    void setHeaderReplyAddress(const std::string& replyAddress);

    /**
     * @brief containsHeaderEffort Tests whether the "effort" header of the
     * message is set or not.
     * @return true, if the "effort" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_EFFORT()
     */
    bool containsHeaderEffort() const;
    /**
     * @brief getHeaderEffort Gets the effort which should be expent delivering the message.
     * Use JoynrMessage::containsHeaderEffort() to check whether the header is available or not.
     * @return the "effort" header of the message, if the header is set; A
     * default-constructed value, otherwise.
     * @see JoynrMessage::HEADER_EFFORT()
     */
    std::string getHeaderEffort() const;
    /**
     * @brief setHeaderEffort Sets the effort to be expent on delivering the message.
     * If the header is already set, its value is replaced with the new one.
     * Use JoynrMessage::containsHeaderEffort() to check whether the header is
     * already set or not.
     * @param effort the "effort" header to be set on the message.
     * @see JoynrMessage::HEADER_EFFORT()
     */
    void setHeaderEffort(const std::string& effort);

    /**
     * @brief isReceivedFromGlobal Check whether receivedFromGlobal is set (default: false).
     * ReceivedFromGlobal indicates whether a multicast message is originating from the current
     * runtime and should hence be transmitted globally, or whether it is a message received via
     * a global transport middleware and needs to be routed internally.
     * @return true, if receivedFromGlobal is set; false, otherwise
     * @see JoynrMessage::setReceivedFromGlobal()
     */
    bool isReceivedFromGlobal() const;
    /**
     * @brief setReceivedFromGlobal set receivedFromGlobal.
     * @param receivedFromGlobal the new value of receivedFromGlobal
     * @see JoynrMessage::isReceivedFromGlobal()
     */
    void setReceivedFromGlobal(bool receivedFromGlobal);

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

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(type), MUESLI_NVP(header), MUESLI_NVP(payload));
    }

    /**
     * @brief toLogMessage Used to print log messages.
     * @return a string representing this message used for logging.
     */
    std::string toLogMessage() const;

private:
    /**
     * @brief CUSTOM_HEADER_PREFIX The prefix used when adding or retrieving custom headers
     * from the headers map.
     * @return the prefix for the custom header entries.
     */
    static const std::string& CUSTOM_HEADER_PREFIX();

    /**
     * @brief containsHeader checks whether key is defined in the header map
     * @param key the header name key to lookup
     * @return true if the key was found in the header map
     */
    bool containsHeader(const std::string& key) const;

    std::string getHeaderForKey(const std::string& key) const;

    void setHeaderForKey(const std::string& key, const std::string& value);

    std::string type;
    std::map<std::string, std::string> header;
    std::string payload;
    // receivedFromGlobal is a transient attribute which will not be serialized.
    // It is only used locally for routing decisions.
    bool receivedFromGlobal;

    // Transient flag which marks message that are send to a provider which is registered
    // on the local cluster-controller.
    bool localMessage;
    ADD_LOGGER(JoynrMessage);

    void generateAndSetMsgIdHeaderIfAbsent();
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::JoynrMessage, "joynr.JoynrMessage")

#endif // JOYNRMESSAGE_H
