/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#include "cluster-controller/http-communication-manager/LongPollMessageSerializer.h"
#include "joynr/JoynrMessagingEndpointAddress.h"
#include "joynr/Util.h"
#include "joynr/JoynrMessage.h"
#include "joynr/MessageRouter.h"
#include "joynr/JsonSerializer.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/Directory.h"
#include "joynr/joynrlogging.h"

#include <QByteArray>

namespace joynr {

joynr_logging::Logger* LongPollMessageSerializer::logger = joynr_logging::Logging::getInstance()->getLogger("MSG", "LongPollMessageSerializer");

LongPollMessageSerializer::LongPollMessageSerializer(
        MessageRouter * messageRouter,
        Directory<QString, EndpointAddressBase>* partId2EndpointAddrDirectory
) :
    messageRouter(messageRouter),
    partId2EndpointAddrDirectory(partId2EndpointAddrDirectory)
{}

LongPollMessageSerializer::~LongPollMessageSerializer()
{

}

void LongPollMessageSerializer::serializedMessageReceived(
        const QByteArray& serializedMessage)
{
    JoynrMessage* msg = JsonSerializer::deserialize<JoynrMessage>(serializedMessage);
    if (msg->getType().isEmpty()) {
        LOG_ERROR(logger, "received empty message - dropping Messages");
        return;
    }
    if(!msg->containsHeaderExpiryDate()) {
        LOG_ERROR(logger, QString("received message [msgId=%1] without decay time - dropping message")
                  .arg(msg->getHeaderMessageId())
        );
    }

    if (msg->getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST || msg->getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST){
        //TODO ca: check if replyTo header info is available?
        QString replyChannelId = msg->getHeaderReplyChannelId();
        QSharedPointer<JoynrMessagingEndpointAddress> endPointAddress(new JoynrMessagingEndpointAddress(replyChannelId));
        partId2EndpointAddrDirectory->add(msg->getHeaderFrom(), endPointAddress);
    }

    QDateTime expiryDate = msg->getHeaderExpiryDate();
    // Set the Qos for the reply message
    qint64 ttl = DispatcherUtils::convertAbsoluteTimeToTtl(expiryDate);
    MessagingQos qos(ttl);

    // messageRouter.route passes the message reference to the MessageRunnable, which copies it.
    //TODO would be nicer if the pointer would be passed to messageRouter, on to MessageRunnable, and runnable should delete it.
    messageRouter->route(*msg, qos);
    delete msg;
}

} // namespace joynr
