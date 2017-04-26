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

#include "joynr/MessageSender.h"

#include <cassert>

#include "joynr/IDispatcher.h"
#include "joynr/IMessageRouter.h"
#include "joynr/MulticastPublication.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/exceptions/MethodInvocationException.h"

namespace joynr
{

INIT_LOGGER(MessageSender);

MessageSender::MessageSender(std::shared_ptr<IMessageRouter> messageRouter,
                             std::uint64_t ttlUpliftMs)
        : dispatcher(nullptr), messageRouter(messageRouter), messageFactory(ttlUpliftMs)
{
}

void MessageSender::registerDispatcher(IDispatcher* dispatcher)
{
    this->dispatcher = dispatcher;
}

void MessageSender::sendRequest(const std::string& senderParticipantId,
                                const std::string& receiverParticipantId,
                                const MessagingQos& qos,
                                const Request& request,
                                std::shared_ptr<IReplyCaller> callback,
                                bool isLocalMessage)
{
    assert(dispatcher != nullptr);

    dispatcher->addReplyCaller(request.getRequestReplyId(), callback, qos);
    JoynrMessage message = messageFactory.createRequest(
            senderParticipantId, receiverParticipantId, qos, request, isLocalMessage);
    assert(messageRouter);
    messageRouter->route(message);
}

void MessageSender::sendOneWayRequest(const std::string& senderParticipantId,
                                      const std::string& receiverParticipantId,
                                      const MessagingQos& qos,
                                      const OneWayRequest& request,
                                      bool isLocalMessage)
{
    try {
        JoynrMessage message = messageFactory.createOneWayRequest(
                senderParticipantId, receiverParticipantId, qos, request, isLocalMessage);
        assert(messageRouter);
        messageRouter->route(message);
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    }
}

void MessageSender::sendReply(const std::string& senderParticipantId,
                              const std::string& receiverParticipantId,
                              const MessagingQos& qos,
                              const Reply& reply)
{
    try {
        JoynrMessage message =
                messageFactory.createReply(senderParticipantId, receiverParticipantId, qos, reply);
        assert(messageRouter);
        messageRouter->route(message);
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    } catch (const exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_ERROR(logger,
                        "Reply with RequestReplyId {} could not be sent to {}. Error: {}",
                        reply.getRequestReplyId(),
                        receiverParticipantId,
                        e.getMessage());
    }
}

void MessageSender::sendSubscriptionRequest(const std::string& senderParticipantId,
                                            const std::string& receiverParticipantId,
                                            const MessagingQos& qos,
                                            const SubscriptionRequest& subscriptionRequest,
                                            bool isLocalMessage)
{
    try {
        JoynrMessage message = messageFactory.createSubscriptionRequest(senderParticipantId,
                                                                        receiverParticipantId,
                                                                        qos,
                                                                        subscriptionRequest,
                                                                        isLocalMessage);
        assert(messageRouter);
        messageRouter->route(message);
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    }
}

void MessageSender::sendBroadcastSubscriptionRequest(
        const std::string& senderParticipantId,
        const std::string& receiverParticipantId,
        const MessagingQos& qos,
        const BroadcastSubscriptionRequest& subscriptionRequest,
        bool isLocalMessage)
{
    try {
        JoynrMessage message =
                messageFactory.createBroadcastSubscriptionRequest(senderParticipantId,
                                                                  receiverParticipantId,
                                                                  qos,
                                                                  subscriptionRequest,
                                                                  isLocalMessage);
        assert(messageRouter);
        messageRouter->route(message);
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    }
}

void MessageSender::sendMulticastSubscriptionRequest(
        const std::string& senderParticipantId,
        const std::string& receiverParticipantId,
        const MessagingQos& qos,
        const MulticastSubscriptionRequest& subscriptionRequest,
        bool isLocalMessage)
{
    try {
        JoynrMessage message =
                messageFactory.createMulticastSubscriptionRequest(senderParticipantId,
                                                                  receiverParticipantId,
                                                                  qos,
                                                                  subscriptionRequest,
                                                                  isLocalMessage);
        assert(messageRouter);
        messageRouter->route(message);
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    }
}

void MessageSender::sendSubscriptionReply(const std::string& senderParticipantId,
                                          const std::string& receiverParticipantId,
                                          const MessagingQos& qos,
                                          const SubscriptionReply& subscriptionReply)
{
    try {
        JoynrMessage message = messageFactory.createSubscriptionReply(
                senderParticipantId, receiverParticipantId, qos, subscriptionReply);
        assert(messageRouter);
        messageRouter->route(message);
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    } catch (const exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_ERROR(
                logger,
                "SubscriptionReply with SubscriptionId {} could not be sent to {}. Error: {}",
                subscriptionReply.getSubscriptionId(),
                receiverParticipantId,
                e.getMessage());
    }
}

void MessageSender::sendSubscriptionStop(const std::string& senderParticipantId,
                                         const std::string& receiverParticipantId,
                                         const MessagingQos& qos,
                                         const SubscriptionStop& subscriptionStop)
{
    try {
        JoynrMessage message = messageFactory.createSubscriptionStop(
                senderParticipantId, receiverParticipantId, qos, subscriptionStop);
        assert(messageRouter);
        messageRouter->route(message);
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    }
}

void MessageSender::sendSubscriptionPublication(const std::string& senderParticipantId,
                                                const std::string& receiverParticipantId,
                                                const MessagingQos& qos,
                                                SubscriptionPublication&& subscriptionPublication)
{
    try {
        JoynrMessage message = messageFactory.createSubscriptionPublication(
                senderParticipantId, receiverParticipantId, qos, subscriptionPublication);
        assert(messageRouter);
        messageRouter->route(message);
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    } catch (const exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_ERROR(
                logger,
                "SubscriptionPublication with SubscriptionId {} could not be sent to {}. Error: {}",
                subscriptionPublication.getSubscriptionId(),
                receiverParticipantId,
                e.getMessage());
    }
}

void MessageSender::sendMulticast(const std::string& fromParticipantId,
                                  const MulticastPublication& multicastPublication,
                                  const MessagingQos& messagingQos)
{
    try {
        JoynrMessage message = messageFactory.createMulticastPublication(
                fromParticipantId, messagingQos, multicastPublication);
        assert(messageRouter);
        messageRouter->route(message);
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    } catch (const exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_ERROR(logger,
                        "MulticastPublication with multicastId {} could not be sent. Error: {}",
                        multicastPublication.getMulticastId(),
                        e.getMessage());
    }
}

} // namespace joynr
