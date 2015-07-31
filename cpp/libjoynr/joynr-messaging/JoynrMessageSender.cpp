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

namespace joynr
{

joynr_logging::Logger* JoynrMessageSender::logger =
        joynr_logging::Logging::getInstance()->getLogger("JOYNR", "JoynrMessageSender");

JoynrMessageSender::JoynrMessageSender(QSharedPointer<MessageRouter> messageRouter)
        : dispatcher(NULL), messageRouter(messageRouter), messageFactory()
{
}

JoynrMessageSender::~JoynrMessageSender()
{
}

void JoynrMessageSender::registerDispatcher(IDispatcher* dispatcher)
{
    this->dispatcher = dispatcher;
}

void JoynrMessageSender::sendRequest(const std::string& senderParticipantId,
                                     const std::string& receiverParticipantId,
                                     const MessagingQos& qos,
                                     const Request& request,
                                     QSharedPointer<IReplyCaller> callback)
{
    assert(dispatcher != NULL);

    dispatcher->addReplyCaller(request.getRequestReplyId().toStdString(), callback, qos);
    JoynrMessage message =
            messageFactory.createRequest(QString::fromStdString(senderParticipantId),
                                         QString::fromStdString(receiverParticipantId),
                                         qos,
                                         request);
    assert(!messageRouter.isNull());
    messageRouter->route(message);
}

void JoynrMessageSender::sendReply(const std::string& senderParticipantId,
                                   const std::string& receiverParticipantId,
                                   const MessagingQos& qos,
                                   const Reply& reply)
{
    JoynrMessage message = messageFactory.createReply(QString::fromStdString(senderParticipantId),
                                                      QString::fromStdString(receiverParticipantId),
                                                      qos,
                                                      reply);
    assert(!messageRouter.isNull());
    messageRouter->route(message);
}

void JoynrMessageSender::sendSubscriptionRequest(const std::string& senderParticipantId,
                                                 const std::string& receiverParticipantId,
                                                 const MessagingQos& qos,
                                                 const SubscriptionRequest& subscriptionRequest)
{
    JoynrMessage message =
            messageFactory.createSubscriptionRequest(QString::fromStdString(senderParticipantId),
                                                     QString::fromStdString(receiverParticipantId),
                                                     qos,
                                                     subscriptionRequest);
    assert(!messageRouter.isNull());
    messageRouter->route(message);
}

void JoynrMessageSender::sendBroadcastSubscriptionRequest(
        const std::string& senderParticipantId,
        const std::string& receiverParticipantId,
        const MessagingQos& qos,
        const BroadcastSubscriptionRequest& subscriptionRequest)
{
    JoynrMessage message = messageFactory.createBroadcastSubscriptionRequest(
            QString::fromStdString(senderParticipantId),
            QString::fromStdString(receiverParticipantId),
            qos,
            subscriptionRequest);
    assert(!messageRouter.isNull());
    messageRouter->route(message);
}

void JoynrMessageSender::sendSubscriptionReply(const std::string& senderParticipantId,
                                               const std::string& receiverParticipantId,
                                               const MessagingQos& qos,
                                               const SubscriptionReply& subscriptionReply)
{
    JoynrMessage message =
            messageFactory.createSubscriptionReply(QString::fromStdString(senderParticipantId),
                                                   QString::fromStdString(receiverParticipantId),
                                                   qos,
                                                   subscriptionReply);
    assert(!messageRouter.isNull());
    messageRouter->route(message);
}

void JoynrMessageSender::sendSubscriptionStop(const std::string& senderParticipantId,
                                              const std::string& receiverParticipantId,
                                              const MessagingQos& qos,
                                              const SubscriptionStop& subscriptionStop)
{
    JoynrMessage message =
            messageFactory.createSubscriptionStop(QString::fromStdString(senderParticipantId),
                                                  QString::fromStdString(receiverParticipantId),
                                                  qos,
                                                  subscriptionStop);
    assert(!messageRouter.isNull());
    messageRouter->route(message);
}

void JoynrMessageSender::sendSubscriptionPublication(
        const std::string& senderParticipantId,
        const std::string& receiverParticipantId,
        const MessagingQos& qos,
        const SubscriptionPublication& subscriptionPublication)
{
    JoynrMessage message = messageFactory.createSubscriptionPublication(
            QString::fromStdString(senderParticipantId),
            QString::fromStdString(receiverParticipantId),
            qos,
            subscriptionPublication);
    assert(!messageRouter.isNull());
    messageRouter->route(message);
}

} // namespace joynr
