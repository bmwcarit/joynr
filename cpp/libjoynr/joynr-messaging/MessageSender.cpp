/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/IDispatcher.h"
#include "joynr/IMessageRouter.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MulticastPublication.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/MutableMessage.h"
#include "joynr/OneWayRequest.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/MethodInvocationException.h"

namespace joynr
{

MessageSender::MessageSender(std::shared_ptr<IMessageRouter> messageRouter,
                             std::shared_ptr<IKeychain> keyChain,
                             std::uint64_t ttlUpliftMs)
        : _dispatcher(),
          _messageRouter(std::move(messageRouter)),
          _messageFactory(ttlUpliftMs, std::move(keyChain)),
          _replyToAddress()
{
}

void MessageSender::setReplyToAddress(const std::string& replyToAddress)
{
    this->_replyToAddress = replyToAddress;
}

void MessageSender::registerDispatcher(std::weak_ptr<IDispatcher> dispatcher)
{
    this->_dispatcher = std::move(dispatcher);
}

void MessageSender::sendRequest(const std::string& senderParticipantId,
                                const std::string& receiverParticipantId,
                                const MessagingQos& qos,
                                const Request& request,
                                std::shared_ptr<IReplyCaller> callback,
                                bool isLocalMessage)
{
    auto dispatcherSharedPtr = _dispatcher.lock();
    if (dispatcherSharedPtr == nullptr) {
        JOYNR_LOG_ERROR(logger(),
                        "Sending a request failed. Dispatcher is null. Probably a proxy "
                        "was used after the runtime was deleted.");
        return;
    }

    MutableMessage message = _messageFactory.createRequest(
            senderParticipantId, receiverParticipantId, qos, request, isLocalMessage);
    dispatcherSharedPtr->addReplyCaller(request.getRequestReplyId(), std::move(callback), qos);

    if (!message.isLocalMessage()) {
        message.setReplyTo(_replyToAddress);
    }

    JOYNR_LOG_DEBUG(logger(),
                    "Send Request: method: {}, requestReplyId: {}, messageId: {}, "
                    "proxy participantId: {}, provider participantId: {}",
                    request.getMethodName(),
                    request.getRequestReplyId(),
                    message.getId(),
                    senderParticipantId,
                    receiverParticipantId);
    assert(_messageRouter);
    _messageRouter->route(message.getImmutableMessage());
}

void MessageSender::sendOneWayRequest(const std::string& senderParticipantId,
                                      const std::string& receiverParticipantId,
                                      const MessagingQos& qos,
                                      const OneWayRequest& request,
                                      bool isLocalMessage)
{
    try {
        MutableMessage message = _messageFactory.createOneWayRequest(
                senderParticipantId, receiverParticipantId, qos, request, isLocalMessage);
        JOYNR_LOG_DEBUG(logger(),
                        "Send OneWayRequest: method: {}, messageId: {}, proxy participantId: {}, "
                        "provider participantId: {}",
                        request.getMethodName(),
                        message.getId(),
                        senderParticipantId,
                        receiverParticipantId);
        assert(_messageRouter);
        _messageRouter->route(message.getImmutableMessage());
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    }
}

void MessageSender::sendReply(const std::string& senderParticipantId,
                              const std::string& receiverParticipantId,
                              const MessagingQos& qos,
                              std::unordered_map<std::string, std::string> prefixedCustomHeaders,
                              const Reply& reply)
{
    try {
        MutableMessage message = _messageFactory.createReply(senderParticipantId,
                                                             receiverParticipantId,
                                                             qos,
                                                             std::move(prefixedCustomHeaders),
                                                             reply);
        assert(_messageRouter);
        _messageRouter->route(message.getImmutableMessage());
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    } catch (const exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_ERROR(logger(),
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
        MutableMessage message = _messageFactory.createSubscriptionRequest(senderParticipantId,
                                                                           receiverParticipantId,
                                                                           qos,
                                                                           subscriptionRequest,
                                                                           isLocalMessage);
        if (!message.isLocalMessage()) {
            message.setReplyTo(_replyToAddress);
        }
        JOYNR_LOG_DEBUG(logger(),
                        "Send AttributeSubscriptionRequest: subscriptionId: {}, messageId: {}, "
                        "proxy participantId: {}, provider participantId: {}",
                        subscriptionRequest.getSubscriptionId(),
                        message.getId(),
                        senderParticipantId,
                        receiverParticipantId);
        assert(_messageRouter);
        _messageRouter->route(message.getImmutableMessage());
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
        MutableMessage message =
                _messageFactory.createBroadcastSubscriptionRequest(senderParticipantId,
                                                                   receiverParticipantId,
                                                                   qos,
                                                                   subscriptionRequest,
                                                                   isLocalMessage);
        if (!message.isLocalMessage()) {
            message.setReplyTo(_replyToAddress);
        }
        JOYNR_LOG_DEBUG(logger(),
                        "Send BroadcastSubscriptionRequest: subscriptionId: {}, messageId: {}, "
                        "proxy participantId: {}, provider participantId: {}",
                        subscriptionRequest.getSubscriptionId(),
                        message.getId(),
                        senderParticipantId,
                        receiverParticipantId);
        assert(_messageRouter);
        _messageRouter->route(message.getImmutableMessage());
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
    std::ignore = isLocalMessage;

    try {
        // MulticastSubscriptionRequest is no longer transmitted, instead
        // the SubscriptionReply formerly sent by provider is simulated and
        // routed back to invoke regular reply handling as before.
        JOYNR_LOG_DEBUG(logger(),
                        "MulticastSubscription: subscriptionId: {}, "
                        "proxy participantId: {}, provider participantId: {}",
                        subscriptionRequest.getSubscriptionId(),
                        senderParticipantId,
                        receiverParticipantId);
        SubscriptionReply subscriptionReply;
        subscriptionReply.setSubscriptionId(subscriptionRequest.getSubscriptionId());
        sendSubscriptionReply(receiverParticipantId, senderParticipantId, qos, subscriptionReply);
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
        MutableMessage message = _messageFactory.createSubscriptionReply(
                senderParticipantId, receiverParticipantId, qos, subscriptionReply);
        assert(_messageRouter);
        _messageRouter->route(message.getImmutableMessage());
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    } catch (const exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_ERROR(
                logger(),
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
        MutableMessage message = _messageFactory.createSubscriptionStop(
                senderParticipantId, receiverParticipantId, qos, subscriptionStop);
        JOYNR_LOG_DEBUG(logger(),
                        "UNREGISTER SUBSCRIPTION call proxy: subscriptionId: {}, messageId: {}, "
                        "proxy participantId: {}, provider participantId: {}",
                        subscriptionStop.getSubscriptionId(),
                        message.getId(),
                        senderParticipantId,
                        receiverParticipantId);
        assert(_messageRouter);
        _messageRouter->route(message.getImmutableMessage());
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
        MutableMessage message = _messageFactory.createSubscriptionPublication(
                senderParticipantId, receiverParticipantId, qos, subscriptionPublication);
        assert(_messageRouter);
        _messageRouter->route(message.getImmutableMessage());
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    } catch (const exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_ERROR(
                logger(),
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
        MutableMessage message = _messageFactory.createMulticastPublication(
                fromParticipantId, messagingQos, multicastPublication);
        assert(_messageRouter);
        _messageRouter->route(message.getImmutableMessage());
    } catch (const std::invalid_argument& exception) {
        throw joynr::exceptions::MethodInvocationException(exception.what());
    } catch (const exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_ERROR(logger(),
                        "MulticastPublication with multicastId {} could not be sent. Error: {}",
                        multicastPublication.getMulticastId(),
                        e.getMessage());
    }
}

void MessageSender::removeRoutingEntry(const std::string& participantId)
{
    try {
        _messageRouter->removeNextHop(participantId);
    } catch (const exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_ERROR(logger(),
                        "removeNextHop for participantId {} could not be sent. Error: {}",
                        participantId,
                        e.getMessage());
    }
}
} // namespace joynr
