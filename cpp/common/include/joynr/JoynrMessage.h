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
#ifndef JOYNRMESSAGE_H_
#define JOYNRMESSAGE_H_

#include "joynr/JoynrCommonExport.h"

#include "joynr/joynrlogging.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QByteArray>


namespace joynr {

/**
  * JoynrMessage describes the message format used to send messages between Joynr applications
  * and a ClusterController as well as between multiple ClusterControllers
  *
  * fields
  *    type (one_way, request, reply, subscription_request, subscription_reply, subscription_stop)
  *    QVariant header, contains domain, interface, requestId, replyId, sourceParticpantId,
  *        destinationParicipantId, ...
  *    QVariant payload
  * JoynrMessages are serialized in JSON format
  */
class JOYNRCOMMON_EXPORT JoynrMessage: public QObject {
    Q_OBJECT

    Q_PROPERTY(QString type READ getType WRITE setType)
    Q_PROPERTY(QVariant header READ getHeader WRITE setHeader)
    Q_PROPERTY(QByteArray payload READ getPayload WRITE setPayload)

public:
    JoynrMessage();

    /**
     * @brief HEADER_CONTENT_TYPE The "content type" header contains the type information of the message.
     * @return the name/key for the "content type" header.
     */
    static const QString& HEADER_CONTENT_TYPE();

    /**
     * @brief HEADER_MESSAGE_ID The "message ID" header contains a unique ID of the message.
     * @return the name/key for the "message ID" header.
     */
    static const QString& HEADER_MESSAGE_ID();
    /**
     * @brief HEADER_NAME_TO The "to" header stores the receiver participant ID.
     * @return the name/key for the "to" header.
     */
    static const QString& HEADER_TO();
    /**
     * @brief HEADER_NAME_FROM The "from" header stores the sender participant ID.
     * @return the name/key for the "from" header.
     */
    static const QString& HEADER_FROM();
    /**
     * @brief HEADER_EXPIRY_DATE The "expiry date" header stores the expiry date of a message. The
     * message will be discarded by any Joynr Messaging component after this date. Make sure clocks
     * are in sync to avoid deletion of messages by mistake.
     * @return the name/key for the "expiry date" header.
     */
    static const QString& HEADER_EXPIRY_DATE();
    /**
     * @brief HEADER_REPLY_CHANNEL_ID The "reply channel ID" header stores the senders channel ID. It
     * is set in all messages that expect a response to be send by the receiver. When the receiver
     * sends the response, the reply channel ID determines the channel to send the response to.
     * @return
     */
    static const QString& HEADER_REPLY_CHANNEL_ID();

    static const QString VALUE_MESSAGE_TYPE_ONE_WAY;
    static const QString VALUE_MESSAGE_TYPE_REQUEST;
    static const QString VALUE_MESSAGE_TYPE_REPLY;
    static const QString VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST;
    static const QString VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY; //reply that the subscription was registered successfully
    static const QString VALUE_MESSAGE_TYPE_PUBLICATION;
    static const QString VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP;

    static const QString VALUE_CONTENT_TYPE_TEXT_PLAIN;
    static const QString VALUE_CONTENT_TYPE_APPLICATION_JSON;

    JoynrMessage(const JoynrMessage& message);
    JoynrMessage& operator=(const JoynrMessage& message);
    //deactivated to fix linker warnings. Not needed anywhere at the moment.
    bool operator==(const JoynrMessage& message) const;

    QString getType() const;
    void setType(const QString& type);
    QVariant getHeader() const;
    void setHeader(const QVariant& header);
    QByteArray getPayload() const;
    void setPayload(const QByteArray& payload);

    /**
     * @brief containsHeaderContentType Tests whether the "content type" header of the message is set or not.
     * @return true, if the "content type" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_CONTENT_TYPE()
     */
    bool containsHeaderContentType() const;
    /**
     * @brief getHeaderContentType Gets the content type of the message. Use JoynrMessage::containsHeaderContentType()
     * to check whether the header is available or not.
     * @return the "content type" header of the message, if the header is set; A default-constructed value, otherwise.
     * @see JoynrMessage::HEADER_CONTENT_TYPE()
     */
    QString getHeaderContentType() const;
    /**
     * @brief setHeaderContentType Sets the content type of the message. If the header is already set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderContentType() to check whether the header is
     * already set or not.
     * @param to the "content type" header to be set on the message.
     * @see JoynrMessage::HEADER_CONTENT_TYPE()
     */
    void setHeaderContentType(const QString& contentType);

    /**
     * @brief containsHeaderMessageId Tests whether the "message ID" header of the message is set or not.
     * @return true, if the "message ID" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_MESSAGE_ID()
     */
    bool containsHeaderMessageId() const;
    /**
     * @brief getHeaderMessageId Gets the ID of the message. Use JoynrMessage::containsHeaderMessageId()
     * to check whether the header is available or not.
     * @return the "message ID" header of the message, if the header is set; A default-constructed value, otherwise.
     * @see JoynrMessage::HEADER_MESSAGE_ID()
     */
    QString getHeaderMessageId() const;
    /**
     * @brief setHeaderMessageId Sets the ID of the message. If the header is already set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderMessageId() to check whether the header is
     * already set or not.
     * @param to the "message ID" header to be set on the message.
     * @see JoynrMessage::HEADER_MESSAGE_ID()
     */
    void setHeaderMessageId(const QString& msgId);

    /**
     * @brief containsHeaderTo Tests whether the "to" header of the message is set or not.
     * @return true, if the "to" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_TO()
     */
    bool containsHeaderTo() const;
    /**
     * @brief getHeaderTo Gets the receiver participant ID of the message. Use JoynrMessage::containsHeaderTo()
     * to check whether the header is available or not.
     * @return the "to" header of the message, if the header is set; A default-constructed value, otherwise.
     * @see JoynrMessage::HEADER_TO()
     */
    QString getHeaderTo() const;
    /**
     * @brief setHeaderTo Sets the receiver participant ID of the message. If the header is already set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderTo() to check whether the header is
     * already set or not.
     * @param to the "to" header to be set on the message.
     * @see JoynrMessage::HEADER_TO()
     */
    void setHeaderTo(const QString& to);

    /**
     * @brief containsHeaderFrom Tests whether the "from" header of the message is set or not.
     * @return true, if the "from" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_FROM()
     */
    bool containsHeaderFrom() const;
    /**
     * @brief getHeaderFrom Gets the sender participant ID of the message. Use JoynrMessage::containsHeaderFrom()
     * to check whether the header is available or not.
     * @return the "from" header of the message, if the header is set; A default-constructed value, otherwise.
     * @see JoynrMessage::HEADER_FROM()
     */
    QString getHeaderFrom() const;
    /**
     * @brief setHeaderFrom Sets the sender participant ID of the message. If the header is already set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderFrom() to check whether the header is
     * already set or not.
     * @param to the "from" header to be set on the message.
     * @see JoynrMessage::HEADER_FROM()
     */
    void setHeaderFrom(const QString& from);

    /**
     * @brief containsHeaderExpiryDate Tests whether the "expiry date" header of the message is set or not.
     * @return true, if the "expiry date" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_EXPIRY_DATE()
     */
    bool containsHeaderExpiryDate() const;
    /**
     * @brief getHeaderExpiryDate Gets the expiry date of the message. Use JoynrMessage::containsHeaderExpiryDate()
     * to check whether the header is available or not.
     * @return the "expiry date" header of the message, if the header is set; A default-constructed value, otherwise.
     * @see JoynrMessage::HEADER_EXPIRY_DATE()
     */
    QDateTime getHeaderExpiryDate() const;
    /**
     * @brief setHeaderExpiryDate Sets the expiry date of the message. If the header is already set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderExpiryDate() to check whether the header is
     * already set or not.
     * @param to the "expiry date" header to be set on the message.
     * @see JoynrMessage::HEADER_EXPIRY_DATE()
     */
    void setHeaderExpiryDate(const QDateTime& expiryDate);

    /**
     * @brief containsHeaderReplyChannelId Tests whether the "reply channel ID" header of the message is set or not.
     * @return true, if the "reply channel ID" header is set; false, otherwise.
     * @see JoynrMessage::HEADER_REPLY_CHANNEL_ID()
     */
    bool containsHeaderReplyChannelId() const;
    /**
     * @brief getHeaderReplyChannelId Gets the reply channel ID of the message. Use JoynrMessage::containsHeaderReplyChannelId()
     * to check whether the header is available or not.
     * @return the "reply channel ID" header of the message, if the header is set; A default-constructed value, otherwise.
     * @see JoynrMessage::HEADER_REPLY_CHANNEL_ID()
     */
    QString getHeaderReplyChannelId() const;
    /**
     * @brief setHeaderReplyChannelId Sets the reply channel ID of the message. If the header is already set, its
     * value is replaced with the new one. Use JoynrMessage::containsHeaderReplyChannelId() to check whether the header is
     * already set or not.
     * @param to the "reply channel ID" header to be set on the message.
     * @see JoynrMessage::HEADER_REPLY_CHANNEL_ID()
     */
    void setHeaderReplyChannelId(const QString& replyChannelId);

private:
    /**
     * @brief containsHeader checks whether key is defined in the header map
     * @param key the header name key to lookup
     * @return true if the key was found in the header map
     */
    bool containsHeader(const QString& key) const;

    template <class T>
    T getHeader(const QString& key) const {
        return header.toMap().value(key).value<T>();
    }

    template <class T>
    void setHeader(const QString& key, const T& value) {
        QVariantMap headerMap = header.toMap();
        headerMap.insert(key, value);
        header = QVariant::fromValue(headerMap);
    }

    QString type;
    QVariant header;
    QByteArray payload;
    static joynr_logging::Logger* logger;

    void generateAndSetMsgIdHeaderIfAbsent();
};

// printing JoynrMessage with google-test and google-mock
void PrintTo(const joynr::JoynrMessage& value, ::std::ostream* os);

} // namespace joynr

Q_DECLARE_METATYPE(joynr::JoynrMessage)
#endif /* JOYNRMESSAGE_H_ */

