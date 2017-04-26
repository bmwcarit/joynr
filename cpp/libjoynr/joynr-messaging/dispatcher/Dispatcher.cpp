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

INIT_LOGGER(Dispatcher);

Dispatcher::Dispatcher(std::shared_ptr<IMessageSender> messageSender,
                       boost::asio::io_service& ioService,
                       int maxThreads)
        : messageSender(messageSender),
          requestCallerDirectory("Dispatcher-RequestCallerDirectory", ioService),
          replyCallerDirectory("Dispatcher-ReplyCallerDirectory", ioService),
          publicationManager(nullptr),
          subscriptionManager(nullptr),
          handleReceivedMessageThreadPool("Dispatcher", maxThreads),
          subscriptionHandlingMutex()
{
}

Dispatcher::~Dispatcher()
{
    JOYNR_LOG_TRACE(logger, "Destructing Dispatcher");
    handleReceivedMessageThreadPool.shutdown();
    delete publicationManager;
    publicationManager = nullptr;
    JOYNR_LOG_TRACE(logger, "Destructing finished");
}

void Dispatcher::addRequestCaller(const std::string& participantId,
                                  std::shared_ptr<RequestCaller> requestCaller)
{
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    JOYNR_LOG_TRACE(logger, "addRequestCaller id= {}", participantId);
    requestCallerDirectory.add(participantId, requestCaller);

    if (publicationManager != nullptr) {
        // publication manager queues received subscription requests, that are
        // received before the corresponding request caller is added
        publicationManager->restore(participantId, requestCaller, messageSender.get());
    } else {
        JOYNR_LOG_WARN(logger, "No publication manager available!");
    }
}

void Dispatcher::removeRequestCaller(const std::string& participantId)
{
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    JOYNR_LOG_TRACE(logger, "removeRequestCaller id= {}", participantId);
    // TODO if a provider is removed, all publication runnables are stopped
    // the subscription request is deleted,
    // Q: Should it be restored once the provider is registered again?
    publicationManager->removeAllSubscriptions(participantId);
    requestCallerDirectory.remove(participantId);
}

void Dispatcher::addReplyCaller(const std::string& requestReplyId,
                                std::shared_ptr<IReplyCaller> replyCaller,
                                const MessagingQos& qosSettings)
{
    JOYNR_LOG_TRACE(logger, "addReplyCaller id= {}", requestReplyId);
    // add the callback to the registry that is responsible for reply messages
    replyCallerDirectory.add(requestReplyId, replyCaller, qosSettings.getTtl());
}

void Dispatcher::removeReplyCaller(const std::string& requestReplyId)
{
    JOYNR_LOG_TRACE(logger, "removeReplyCaller id= {}", requestReplyId);
    replyCallerDirectory.remove(requestReplyId);
}

void Dispatcher::receive(const JoynrMessage& message)
{
    JOYNR_LOG_TRACE(logger, "receive(message). Message payload: {}", message.getPayload());
    ReceivedMessageRunnable* receivedMessageRunnable = new ReceivedMessageRunnable(message, *this);
    handleReceivedMessageThreadPool.execute(receivedMessageRunnable);
}

void Dispatcher::handleRequestReceived(const JoynrMessage& message)
{
    std::string senderId = message.getHeaderFrom();
    std::string receiverId = message.getHeaderTo();

    // lookup necessary data
    std::shared_ptr<RequestCaller> caller = requestCallerDirectory.lookup(receiverId);
    if (caller == nullptr) {
        JOYNR_LOG_ERROR(
                logger,
                "caller not found in the RequestCallerDirectory for receiverId {}, ignoring",
                receiverId);
        return;
    }
    std::string interfaceName = caller->getInterfaceName();

    // Get the request interpreter that has been registered with this interface name
    std::shared_ptr<IRequestInterpreter> requestInterpreter =
            InterfaceRegistrar::instance().getRequestInterpreter(interfaceName);

    // deserialize Request
    Request request;
    try {
        joynr::serializer::deserializeFromJson(request, message.getPayload());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize request object from: {} - error: {}",
                        message.getPayload(),
                        e.what());
        return;
    }

    const std::string& requestReplyId = request.getRequestReplyId();
    JoynrTimePoint requestExpiryDate = message.getHeaderExpiryDate();

    auto onSuccess =
            [requestReplyId, requestExpiryDate, this, senderId, receiverId](Reply&& reply) {
        reply.setRequestReplyId(requestReplyId);
        // send reply back to the original sender (ie. sender and receiver ids are reversed
        // on purpose)
        JOYNR_LOG_TRACE(
                logger, "Got reply from RequestInterpreter for requestReplyId {}", requestReplyId);
        JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now());
        std::int64_t ttl = std::chrono::duration_cast<std::chrono::milliseconds>(requestExpiryDate -
                                                                                 now).count();
        messageSender->sendReply(receiverId, // receiver of the request is sender of reply
                                 senderId,   // sender of request is receiver of reply
                                 MessagingQos(ttl),
                                 std::move(reply));
    };

    auto onError = [requestReplyId, requestExpiryDate, this, senderId, receiverId](
            const std::shared_ptr<exceptions::JoynrException>& exception) {
        Reply reply;
        reply.setRequestReplyId(requestReplyId);
        reply.setError(exception);
        JOYNR_LOG_WARN(logger,
                       "Got error reply from RequestInterpreter for requestReplyId {}",
                       requestReplyId);
        JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now());
        std::int64_t ttl = std::chrono::duration_cast<std::chrono::milliseconds>(requestExpiryDate -
                                                                                 now).count();
        messageSender->sendReply(receiverId, // receiver of the request is sender of reply
                                 senderId,   // sender of request is receiver of reply
                                 MessagingQos(ttl),
                                 std::move(reply));
    };
    // execute request
    requestInterpreter->execute(caller, request, std::move(onSuccess), std::move(onError));
}

void Dispatcher::handleOneWayRequestReceived(const JoynrMessage& message)
{
    std::string receiverId = message.getHeaderTo();

    // json request
    // lookup necessary data
    std::shared_ptr<RequestCaller> caller = requestCallerDirectory.lookup(receiverId);
    if (caller == nullptr) {
        JOYNR_LOG_ERROR(
                logger,
                "caller not found in the RequestCallerDirectory for receiverId {}, ignoring",
                receiverId);
        return;
    }
    std::string interfaceName = caller->getInterfaceName();

    // Get the request interpreter that has been registered with this interface name
    std::shared_ptr<IRequestInterpreter> requestInterpreter =
            InterfaceRegistrar::instance().getRequestInterpreter(interfaceName);

    // deserialize json
    OneWayRequest request;
    try {
        joynr::serializer::deserializeFromJson(request, message.getPayload());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize request object from: {} - error: {}",
                        message.getPayload(),
                        e.what());
        return;
    }

    // execute request
    requestInterpreter->execute(caller, request);
}

void Dispatcher::handleReplyReceived(const JoynrMessage& message)
{
    // deserialize the Reply
    Reply reply;
    try {
        joynr::serializer::deserializeFromJson(reply, message.getPayload());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize reply object from: {} - error {}",
                        message.getPayload(),
                        e.what());
        return;
    }

    std::string requestReplyId = reply.getRequestReplyId();
    std::shared_ptr<IReplyCaller> caller = replyCallerDirectory.lookup(requestReplyId);
    if (caller == nullptr) {
        // This used to be a fatal error, but it is possible that the replyCallerDirectory
        // removed
        // the caller
        // because its lifetime exceeded TTL
        JOYNR_LOG_WARN(logger,
                       "caller not found in the ReplyCallerDirectory for requestid {}, ignoring",
                       requestReplyId);
        return;
    }

    caller->execute(std::move(reply));

    // Clean up
    removeReplyCaller(requestReplyId);
}

void Dispatcher::handleSubscriptionRequestReceived(const JoynrMessage& message)
{
    JOYNR_LOG_TRACE(logger, "Starting handleSubscriptionReceived");
    // Make sure that noone is registering a Caller at the moment, because a racing condition could
    // occour.
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    assert(publicationManager != nullptr);

    std::string receiverId = message.getHeaderTo();
    std::shared_ptr<RequestCaller> caller = requestCallerDirectory.lookup(receiverId);

    std::string jsonSubscriptionRequest = message.getPayload();

    // PublicationManager is responsible for deleting SubscriptionRequests
    SubscriptionRequest subscriptionRequest;
    try {
        joynr::serializer::deserializeFromJson(subscriptionRequest, jsonSubscriptionRequest);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize subscription request object from: {} - error: {}",
                        jsonSubscriptionRequest,
                        e.what());
        return;
    }

    if (!caller) {
        // Provider not registered yet
        // Dispatcher will call publicationManger->restore when a new provider is added to
        // activate
        // subscriptions for that provider
        publicationManager->add(
                message.getHeaderFrom(), message.getHeaderTo(), subscriptionRequest);
    } else {
        publicationManager->add(message.getHeaderFrom(),
                                message.getHeaderTo(),
                                caller,
                                subscriptionRequest,
                                messageSender.get());
    }
}

void Dispatcher::handleMulticastSubscriptionRequestReceived(const JoynrMessage& message)
{
    JOYNR_LOG_TRACE(logger, "Starting handleMulticastSubscriptionRequestReceived");
    assert(publicationManager != nullptr);

    std::string jsonSubscriptionRequest = message.getPayload();

    // PublicationManager is responsible for deleting SubscriptionRequests
    MulticastSubscriptionRequest subscriptionRequest;
    try {
        joynr::serializer::deserializeFromJson(subscriptionRequest, jsonSubscriptionRequest);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(
                logger,
                "Unable to deserialize broadcast subscription request object from: {} - error: {}",
                jsonSubscriptionRequest,
                e.what());
        return;
    }

    publicationManager->add(message.getHeaderFrom(),
                            message.getHeaderTo(),
                            subscriptionRequest,
                            messageSender.get());
}

void Dispatcher::handleBroadcastSubscriptionRequestReceived(const JoynrMessage& message)
{
    JOYNR_LOG_TRACE(logger, "Starting handleBroadcastSubscriptionRequestReceived");
    // Make sure that noone is registering a Caller at the moment, because a racing condition could
    // occour.
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    assert(publicationManager != nullptr);

    std::string receiverId = message.getHeaderTo();
    std::shared_ptr<RequestCaller> caller = requestCallerDirectory.lookup(receiverId);

    std::string jsonSubscriptionRequest = message.getPayload();

    // PublicationManager is responsible for deleting SubscriptionRequests
    BroadcastSubscriptionRequest subscriptionRequest;
    try {
        joynr::serializer::deserializeFromJson(subscriptionRequest, jsonSubscriptionRequest);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(
                logger,
                "Unable to deserialize broadcast subscription request object from: {} - error: {}",
                jsonSubscriptionRequest,
                e.what());
        return;
    }

    if (!caller) {
        // Provider not registered yet
        // Dispatcher will call publicationManger->restore when a new provider is added to
        // activate
        // subscriptions for that provider
        publicationManager->add(
                message.getHeaderFrom(), message.getHeaderTo(), subscriptionRequest);
    } else {
        publicationManager->add(message.getHeaderFrom(),
                                message.getHeaderTo(),
                                caller,
                                subscriptionRequest,
                                messageSender.get());
    }
}

void Dispatcher::handleSubscriptionStopReceived(const JoynrMessage& message)
{
    JOYNR_LOG_TRACE(logger, "handleSubscriptionStopReceived");
    std::string jsonSubscriptionStop = message.getPayload();

    std::string subscriptionId;
    try {
        SubscriptionStop subscriptionStop;
        joynr::serializer::deserializeFromJson(subscriptionStop, jsonSubscriptionStop);

        subscriptionId = subscriptionStop.getSubscriptionId();
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize subscription stop object from: {} - error: {}",
                        jsonSubscriptionStop,
                        e.what());
        return;
    }
    assert(publicationManager != nullptr);
    publicationManager->stopPublication(subscriptionId);
}

void Dispatcher::handleSubscriptionReplyReceived(const JoynrMessage& message)
{
    std::string jsonSubscriptionReply = message.getPayload();

    SubscriptionReply subscriptionReply;
    try {
        joynr::serializer::deserializeFromJson(subscriptionReply, jsonSubscriptionReply);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize subscription reply object from: {} - error: {}",
                        jsonSubscriptionReply,
                        e.what());
        return;
    }

    const std::string subscriptionId = subscriptionReply.getSubscriptionId();

    assert(subscriptionManager != nullptr);

    std::shared_ptr<ISubscriptionCallback> callback =
            subscriptionManager->getSubscriptionCallback(subscriptionId);
    if (!callback) {
        JOYNR_LOG_ERROR(logger,
                        "Dropping subscription reply for non/no more existing subscription "
                        "with id = {}",
                        subscriptionId);
        return;
    }

    callback->execute(std::move(subscriptionReply));
}

void Dispatcher::handleMulticastReceived(const JoynrMessage& message)
{
    std::string jsonMulticastPublication = message.getPayload();

    MulticastPublication multicastPublication;
    try {
        joynr::serializer::deserializeFromJson(multicastPublication, jsonMulticastPublication);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize multicast publication object from: {} - error: {}",
                        jsonMulticastPublication,
                        e.what());
        return;
    }

    const std::string multicastId = multicastPublication.getMulticastId();

    assert(subscriptionManager != nullptr);

    std::shared_ptr<ISubscriptionCallback> callback =
            subscriptionManager->getMulticastSubscriptionCallback(multicastId);
    if (callback == nullptr) {
        JOYNR_LOG_ERROR(logger,
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

void Dispatcher::handlePublicationReceived(const JoynrMessage& message)
{
    std::string jsonSubscriptionPublication = message.getPayload();

    SubscriptionPublication subscriptionPublication;
    try {
        joynr::serializer::deserializeFromJson(
                subscriptionPublication, jsonSubscriptionPublication);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(
                logger,
                "Unable to deserialize subscription publication object from: {} - error: {}",
                jsonSubscriptionPublication,
                e.what());
        return;
    }

    const std::string subscriptionId = subscriptionPublication.getSubscriptionId();

    assert(subscriptionManager != nullptr);

    std::shared_ptr<ISubscriptionCallback> callback =
            subscriptionManager->getSubscriptionCallback(subscriptionId);
    if (!callback) {
        JOYNR_LOG_ERROR(logger,
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
    this->subscriptionManager = subscriptionManager;
}

void Dispatcher::registerPublicationManager(PublicationManager* publicationManager)
{
    this->publicationManager = publicationManager;
}

} // namespace joynr
