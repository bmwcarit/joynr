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
#include "joynr/Dispatcher.h"

#include <cassert>
#include <chrono>
#include <cstdint>

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/IMessageSender.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/IRequestInterpreter.h"
#include "joynr/ISubscriptionManager.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/MessagingQos.h"
#include "joynr/MulticastPublication.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/PublicationManager.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"
#include "joynr/serializer/Serializer.h"
#include "libjoynr/joynr-messaging/dispatcher/ReceivedMessageRunnable.h"

namespace joynr
{

Dispatcher::Dispatcher(std::shared_ptr<IMessageSender> messageSender,
                       boost::asio::io_service& ioService,
                       int maxThreads)
        : std::enable_shared_from_this<Dispatcher>(),
          IDispatcher(),
          messageSender(std::move(messageSender)),
          requestCallerDirectory("Dispatcher-RequestCallerDirectory", ioService),
          replyCallerDirectory("Dispatcher-ReplyCallerDirectory", ioService),
          publicationManager(),
          subscriptionManager(nullptr),
          handleReceivedMessageThreadPool(std::make_shared<ThreadPool>("Dispatcher", maxThreads)),
          subscriptionHandlingMutex()
{
    handleReceivedMessageThreadPool->init();
}

Dispatcher::~Dispatcher()
{
    JOYNR_LOG_TRACE(logger(), "Destructing Dispatcher");
    handleReceivedMessageThreadPool->shutdown();
    JOYNR_LOG_TRACE(logger(), "Destructing finished");
}

void Dispatcher::addRequestCaller(const std::string& participantId,
                                  std::shared_ptr<RequestCaller> requestCaller)
{
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    JOYNR_LOG_TRACE(logger(), "addRequestCaller id= {}", participantId);
    requestCallerDirectory.add(participantId, requestCaller);

    if (auto publicationManagerSharedPtr = publicationManager.lock()) {
        // publication manager queues received subscription requests, that are
        // received before the corresponding request caller is added
        publicationManagerSharedPtr->restore(
                participantId, std::move(requestCaller), messageSender);
    } else {
        JOYNR_LOG_WARN(logger(), "No publication manager available!");
    }
}

void Dispatcher::removeRequestCaller(const std::string& participantId)
{
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    JOYNR_LOG_TRACE(logger(), "removeRequestCaller id= {}", participantId);
    // TODO if a provider is removed, all publication runnables are stopped
    // the subscription request is deleted,
    // Q: Should it be restored once the provider is registered again?
    if (auto publicationManagerSharedPtr = publicationManager.lock()) {
        publicationManagerSharedPtr->removeAllSubscriptions(participantId);
    }
    requestCallerDirectory.remove(participantId);
}

void Dispatcher::addReplyCaller(const std::string& requestReplyId,
                                std::shared_ptr<IReplyCaller> replyCaller,
                                const MessagingQos& qosSettings)
{
    JOYNR_LOG_TRACE(logger(), "addReplyCaller id= {}", requestReplyId);
    // add the callback to the registry that is responsible for reply messages
    replyCallerDirectory.add(requestReplyId, std::move(replyCaller), qosSettings.getTtl());
}

void Dispatcher::receive(std::shared_ptr<ImmutableMessage> message)
{
    JOYNR_LOG_TRACE(logger(), "received message: {}", message->toLogMessage());
    // we only support non-encrypted messages for now
    assert(!message->isEncrypted());
    std::shared_ptr<ReceivedMessageRunnable> receivedMessageRunnable =
            std::make_shared<ReceivedMessageRunnable>(std::move(message), shared_from_this());
    handleReceivedMessageThreadPool->execute(receivedMessageRunnable);
}

void Dispatcher::handleRequestReceived(std::shared_ptr<ImmutableMessage> message)
{
    std::string senderId = message->getSender();
    std::string receiverId = message->getRecipient();

    // lookup necessary data
    std::shared_ptr<RequestCaller> caller = requestCallerDirectory.lookup(receiverId);
    if (!caller) {
        JOYNR_LOG_ERROR(
                logger(),
                "caller not found in the RequestCallerDirectory for receiverId {}, ignoring",
                receiverId);
        return;
    }

    const std::string& interfaceName = caller->getInterfaceName();

    // Get the request interpreter that has been registered with this interface name
    std::shared_ptr<IRequestInterpreter> requestInterpreter =
            InterfaceRegistrar::instance().getRequestInterpreter(interfaceName);

    if (!requestInterpreter) {
        JOYNR_LOG_ERROR(logger(), "requestInterpreter not found for interface {}", interfaceName);
        return;
    }

    // deserialize Request
    Request request;
    try {
        joynr::serializer::deserializeFromJson(request, message->getUnencryptedBody());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to deserialize request object from: {} - error: {}",
                        message->toLogMessage(),
                        e.what());
        return;
    }

    const std::string& requestReplyId = request.getRequestReplyId();
    JoynrTimePoint requestExpiryDate = message->getExpiryDate();

    auto onSuccess = [
        requestReplyId,
        requestExpiryDate,
        thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
        senderId,
        receiverId,
        message
    ](Reply && reply) mutable
    {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            JOYNR_LOG_TRACE(logger(),
                            "Got reply from RequestInterpreter for requestReplyId {}",
                            requestReplyId);
            reply.setRequestReplyId(std::move(requestReplyId));
            // send reply back to the original sender (ie. sender and receiver ids are reversed
            // on purpose)
            JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now());
            std::int64_t ttl = std::chrono::duration_cast<std::chrono::milliseconds>(
                                       requestExpiryDate - now).count();
            thisSharedPtr->messageSender->sendReply(
                    receiverId, // receiver of the request is sender of reply
                    senderId,   // sender of request is receiver of reply
                    MessagingQos(ttl),
                    message->getPrefixedCustomHeaders(),
                    std::move(reply));
        }
    };

    auto onError = [
        requestReplyId,
        requestExpiryDate,
        thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
        senderId,
        receiverId,
        message
    ](const std::shared_ptr<exceptions::JoynrException>& exception) mutable
    {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            JOYNR_LOG_WARN(logger(),
                           "Got error reply from RequestInterpreter for requestReplyId {}",
                           requestReplyId);
            Reply reply;
            reply.setRequestReplyId(std::move(requestReplyId));
            reply.setError(exception);
            JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now());
            std::int64_t ttl = std::chrono::duration_cast<std::chrono::milliseconds>(
                                       requestExpiryDate - now).count();
            thisSharedPtr->messageSender->sendReply(
                    receiverId, // receiver of the request is sender of reply
                    senderId,   // sender of request is receiver of reply
                    MessagingQos(ttl),
                    message->getPrefixedCustomHeaders(),
                    std::move(reply));
        }
    };
    // execute request
    requestInterpreter->execute(
            std::move(caller), request, std::move(onSuccess), std::move(onError));
}

void Dispatcher::handleOneWayRequestReceived(std::shared_ptr<ImmutableMessage> message)
{
    const std::string& receiverId = message->getRecipient();

    // json request
    // lookup necessary data
    std::shared_ptr<RequestCaller> caller = requestCallerDirectory.lookup(receiverId);
    if (!caller) {
        JOYNR_LOG_ERROR(
                logger(),
                "caller not found in the RequestCallerDirectory for receiverId {}, ignoring",
                receiverId);
        return;
    }

    const std::string& interfaceName = caller->getInterfaceName();

    // Get the request interpreter that has been registered with this interface name
    std::shared_ptr<IRequestInterpreter> requestInterpreter =
            InterfaceRegistrar::instance().getRequestInterpreter(interfaceName);

    if (!requestInterpreter) {
        JOYNR_LOG_ERROR(logger(),
                        "requestInterpreter not found for receiverId {}, ignoring",
                        interfaceName);
        return;
    }

    // deserialize json
    OneWayRequest request;
    try {
        joynr::serializer::deserializeFromJson(request, message->getUnencryptedBody());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to deserialize request object from: {} - error: {}",
                        message->toLogMessage(),
                        e.what());
        return;
    }

    // execute request
    requestInterpreter->execute(std::move(caller), request);
}

void Dispatcher::handleReplyReceived(std::shared_ptr<ImmutableMessage> message)
{
    // deserialize the Reply
    Reply reply;
    try {
        joynr::serializer::deserializeFromJson(reply, message->getUnencryptedBody());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to deserialize reply object from: {} - error {}",
                        message->toLogMessage(),
                        e.what());
        return;
    }

    std::string requestReplyId = reply.getRequestReplyId();
    std::shared_ptr<IReplyCaller> caller = replyCallerDirectory.take(requestReplyId);
    if (!caller) {
        // This used to be a fatal error, but it is possible that the replyCallerDirectory
        // removed
        // the caller
        // because its lifetime exceeded TTL
        JOYNR_LOG_WARN(logger(),
                       "caller not found in the ReplyCallerDirectory for requestid {}, ignoring",
                       requestReplyId);
        return;
    }

    caller->execute(std::move(reply));
}

void Dispatcher::handleSubscriptionRequestReceived(std::shared_ptr<ImmutableMessage> message)
{
    JOYNR_LOG_TRACE(logger(), "Starting handleSubscriptionReceived");
    // Make sure that noone is registering a Caller at the moment, because a racing condition could
    // occour.
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    auto publicationManagerSharedPtr = publicationManager.lock();
    if (!publicationManagerSharedPtr) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to handle subscription request object from: {} - no publication "
                        "manager available",
                        message->toLogMessage());
        return;
    }

    const std::string& receiverId = message->getRecipient();
    std::shared_ptr<RequestCaller> caller = requestCallerDirectory.lookup(receiverId);

    // PublicationManager is responsible for deleting SubscriptionRequests
    SubscriptionRequest subscriptionRequest;
    try {
        joynr::serializer::deserializeFromJson(subscriptionRequest, message->getUnencryptedBody());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to deserialize subscription request object from: {} - error: {}",
                        message->toLogMessage(),
                        e.what());
        return;
    }

    if (!caller) {
        // Provider not registered yet
        // Dispatcher will call publicationManger->restore when a new provider is added to
        // activate
        // subscriptions for that provider
        publicationManagerSharedPtr->add(
                message->getSender(), message->getRecipient(), subscriptionRequest);
    } else {
        publicationManagerSharedPtr->add(message->getSender(),
                                         message->getRecipient(),
                                         std::move(caller),
                                         subscriptionRequest,
                                         messageSender);
    }
}

void Dispatcher::handleMulticastSubscriptionRequestReceived(
        std::shared_ptr<ImmutableMessage> message)
{
    JOYNR_LOG_TRACE(logger(), "Starting handleMulticastSubscriptionRequestReceived");
    auto publicationManagerSharedPtr = publicationManager.lock();
    if (!publicationManagerSharedPtr) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to handle multicast subscription request object from: {} - no "
                        "publication manager available",
                        message->toLogMessage());
        return;
    }

    // PublicationManager is responsible for deleting SubscriptionRequests
    MulticastSubscriptionRequest subscriptionRequest;
    try {
        joynr::serializer::deserializeFromJson(subscriptionRequest, message->getUnencryptedBody());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(
                logger(),
                "Unable to deserialize broadcast subscription request object from: {} - error: {}",
                message->toLogMessage(),
                e.what());
        return;
    }

    publicationManagerSharedPtr->add(
            message->getSender(), message->getRecipient(), subscriptionRequest, messageSender);
}

void Dispatcher::handleBroadcastSubscriptionRequestReceived(
        std::shared_ptr<ImmutableMessage> message)
{
    JOYNR_LOG_TRACE(logger(), "Starting handleBroadcastSubscriptionRequestReceived");
    // Make sure that noone is registering a Caller at the moment, because a racing condition could
    // occour.
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    auto publicationManagerSharedPtr = publicationManager.lock();
    if (!publicationManagerSharedPtr) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to handle broadcast subscription request object from: {} - no "
                        "publication manager available",
                        message->toLogMessage());
        return;
    }

    const std::string& receiverId = message->getRecipient();
    std::shared_ptr<RequestCaller> caller = requestCallerDirectory.lookup(receiverId);

    // PublicationManager is responsible for deleting SubscriptionRequests
    BroadcastSubscriptionRequest subscriptionRequest;
    try {
        joynr::serializer::deserializeFromJson(subscriptionRequest, message->getUnencryptedBody());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(
                logger(),
                "Unable to deserialize broadcast subscription request object from: {} - error: {}",
                message->toLogMessage(),
                e.what());
        return;
    }

    if (!caller) {
        // Provider not registered yet
        // Dispatcher will call publicationManger->restore when a new provider is added to
        // activate
        // subscriptions for that provider
        publicationManagerSharedPtr->add(
                message->getSender(), message->getRecipient(), subscriptionRequest);
    } else {
        publicationManagerSharedPtr->add(message->getSender(),
                                         message->getRecipient(),
                                         std::move(caller),
                                         subscriptionRequest,
                                         messageSender);
    }
}

void Dispatcher::handleSubscriptionStopReceived(std::shared_ptr<ImmutableMessage> message)
{
    JOYNR_LOG_TRACE(logger(), "handleSubscriptionStopReceived");

    SubscriptionStop subscriptionStop;
    try {
        joynr::serializer::deserializeFromJson(subscriptionStop, message->getUnencryptedBody());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to deserialize subscription stop object from: {} - error: {}",
                        message->toLogMessage(),
                        e.what());
        return;
    }
    auto publicationManagerSharedPtr = publicationManager.lock();
    if (!publicationManagerSharedPtr) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to handle subscription stop object from: {} - no publication "
                        "manager available",
                        message->toLogMessage());
        return;
    }
    publicationManagerSharedPtr->stopPublication(subscriptionStop.getSubscriptionId());
}

void Dispatcher::handleSubscriptionReplyReceived(std::shared_ptr<ImmutableMessage> message)
{
    SubscriptionReply subscriptionReply;
    try {
        joynr::serializer::deserializeFromJson(subscriptionReply, message->getUnencryptedBody());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to deserialize subscription reply object from: {} - error: {}",
                        message->toLogMessage(),
                        e.what());
        return;
    }

    const std::string& subscriptionId = subscriptionReply.getSubscriptionId();

    assert(subscriptionManager != nullptr);

    std::shared_ptr<ISubscriptionCallback> callback =
            subscriptionManager->getSubscriptionCallback(subscriptionId);
    if (!callback) {
        JOYNR_LOG_ERROR(logger(),
                        "Dropping subscription reply for non/no more existing subscription "
                        "with id = {}",
                        subscriptionId);
        return;
    }

    callback->execute(std::move(subscriptionReply));
}

void Dispatcher::handleMulticastReceived(std::shared_ptr<ImmutableMessage> message)
{
    MulticastPublication multicastPublication;
    try {
        joynr::serializer::deserializeFromJson(multicastPublication, message->getUnencryptedBody());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to deserialize multicast publication object from: {} - error: {}",
                        message->toLogMessage(),
                        e.what());
        return;
    }

    const std::string& multicastId = multicastPublication.getMulticastId();

    assert(subscriptionManager != nullptr);

    std::shared_ptr<ISubscriptionCallback> callback =
            subscriptionManager->getMulticastSubscriptionCallback(multicastId);
    if (callback == nullptr) {
        JOYNR_LOG_ERROR(logger(),
                        "Dropping multicast publication for non/no more existing subscription "
                        "with multicastId = {}",
                        multicastId);
        return;
    }

    // TODO: enable for periodic attribute subscriptions
    // when MulticastPublication is extended by subscriptionId
    // subscriptionManager->touchSubscriptionState(subscriptionId);

    callback->execute(std::move(multicastPublication));
}

void Dispatcher::handlePublicationReceived(std::shared_ptr<ImmutableMessage> message)
{
    SubscriptionPublication subscriptionPublication;
    try {
        joynr::serializer::deserializeFromJson(
                subscriptionPublication, message->getUnencryptedBody());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(
                logger(),
                "Unable to deserialize subscription publication object from: {} - error: {}",
                message->toLogMessage(),
                e.what());
        return;
    }

    const std::string& subscriptionId = subscriptionPublication.getSubscriptionId();

    assert(subscriptionManager != nullptr);

    std::shared_ptr<ISubscriptionCallback> callback =
            subscriptionManager->getSubscriptionCallback(subscriptionId);
    if (!callback) {
        JOYNR_LOG_ERROR(logger(),
                        "Dropping publication for non/no more existing subscription with id = {}",
                        subscriptionId);
        return;
    }

    subscriptionManager->touchSubscriptionState(subscriptionId);

    callback->execute(std::move(subscriptionPublication));
}

void Dispatcher::registerSubscriptionManager(
        std::shared_ptr<ISubscriptionManager> subscriptionManager)
{
    this->subscriptionManager = std::move(subscriptionManager);
}

void Dispatcher::registerPublicationManager(std::weak_ptr<PublicationManager> publicationManager)
{
    this->publicationManager = std::move(publicationManager);
}

void Dispatcher::shutdown()
{
    replyCallerDirectory.shutdown();
    requestCallerDirectory.shutdown();
}

} // namespace joynr
