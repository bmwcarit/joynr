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

#include "joynr/JoynrMessageSender.h"
#include "joynr/IMessaging.h"
#include "joynr/IDispatcher.h"
#include "joynr/Request.h"
#include "joynr/MessageRouter.h"

#include <QUuid>
#include <cassert>

namespace joynr {

joynr_logging::Logger* JoynrMessageSender::logger = joynr_logging::Logging::getInstance()->getLogger("JOYNR", "JoynrMessageSender");


JoynrMessageSender::JoynrMessageSender(QSharedPointer<MessageRouter> messageRouter)
    : dispatcher(NULL),
      messageRouter(messageRouter),
      messageFactory() {

}

JoynrMessageSender::~JoynrMessageSender()
{
}

void JoynrMessageSender::registerDispatcher(IDispatcher *dispatcher){
    this->dispatcher = dispatcher;
}

void JoynrMessageSender::sendRequest(
        const QString& senderParticipantId,
        const QString& receiverParticipantId,
        const MessagingQos& qos,
        const Request& request,
        QSharedPointer<IReplyCaller> callback
) {
    assert(dispatcher!=NULL);

    dispatcher->addReplyCaller(
                request.getRequestReplyId(),
                callback,
                qos
    );
    JoynrMessage message = messageFactory.createRequest(
                senderParticipantId,
                receiverParticipantId,
                qos,
                request
    );
    assert(!messageRouter.isNull());
    messageRouter->route(message, qos);

}

void JoynrMessageSender::sendReply(
        const QString& senderParticipantId,
        const QString& receiverParticipantId,
        const MessagingQos& qos,
        const Reply& reply
) {
    JoynrMessage message = messageFactory.createReply(
                senderParticipantId,
                receiverParticipantId,
                qos,
                reply
    );
    assert(!messageRouter.isNull());
    messageRouter->route(message, qos);
}

void JoynrMessageSender::sendSubscriptionRequest(
        const QString& senderParticipantId,
        const QString& receiverParticipantId,
        const MessagingQos& qos,
        const SubscriptionRequest& subscriptionRequest
) {
    JoynrMessage message = messageFactory.createSubscriptionRequest(
                senderParticipantId,
                receiverParticipantId,
                qos,
                subscriptionRequest
    );
    assert(!messageRouter.isNull());
    messageRouter->route(message, qos);
}

void JoynrMessageSender::sendBroadcastSubscriptionRequest(
        const QString& senderParticipantId,
        const QString& receiverParticipantId,
        const MessagingQos& qos,
        const BroadcastSubscriptionRequest& subscriptionRequest
) {
    JoynrMessage message = messageFactory.createBroadcastSubscriptionRequest(
                senderParticipantId,
                receiverParticipantId,
                qos,
                subscriptionRequest
    );
    assert(!messageRouter.isNull());
    messageRouter->route(message, qos);
}

void JoynrMessageSender::sendSubscriptionReply(
        const QString& senderParticipantId,
        const QString& receiverParticipantId,
        const MessagingQos& qos,
        const SubscriptionReply& subscriptionReply
){
    JoynrMessage message = messageFactory.createSubscriptionReply(
                senderParticipantId,
                receiverParticipantId,
                qos,
                subscriptionReply
    );
    assert(!messageRouter.isNull());
    messageRouter->route(message, qos);
}

void JoynrMessageSender::sendSubscriptionStop(
        const QString& senderParticipantId,
        const QString& receiverParticipantId,
        const MessagingQos& qos,
        const SubscriptionStop& subscriptionStop
) {
    JoynrMessage message = messageFactory.createSubscriptionStop(
                senderParticipantId,
                receiverParticipantId,
                qos,
                subscriptionStop
    );
    assert(!messageRouter.isNull());
    messageRouter->route(message, qos);
}

void JoynrMessageSender::sendSubscriptionPublication(
        const QString& senderParticipantId,
        const QString& receiverParticipantId,
        const MessagingQos& qos,
        const SubscriptionPublication& subscriptionPublication
) {
    JoynrMessage message = messageFactory.createSubscriptionPublication(
                senderParticipantId,
                receiverParticipantId,
                qos,
                subscriptionPublication
    );
    assert(!messageRouter.isNull());
    messageRouter->route(message, qos);
}

} // namespace joynr
