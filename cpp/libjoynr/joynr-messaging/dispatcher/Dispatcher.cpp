/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include <cstdint>
#include <chrono>

#include "muesli/archives/json/JsonInputArchive.h"

#include "joynr/serializer/Serializer.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/MessagingQos.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/MessagingQos.h"
#include "joynr/JsonSerializer.h"
#include "joynr/IRequestInterpreter.h"
#include "libjoynr/joynr-messaging/dispatcher/ReceivedMessageRunnable.h"
#include "joynr/PublicationInterpreter.h"
#include "joynr/PublicationManager.h"
#include "joynr/ISubscriptionManager.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/MetaTypeRegistrar.h"
#include "joynr/Request.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"
#include "joynr/TypeUtil.h"

namespace joynr
{

INIT_LOGGER(Dispatcher);

Dispatcher::Dispatcher(JoynrMessageSender* messageSender, int maxThreads)
        : messageSender(messageSender),
          requestCallerDirectory("Dispatcher-RequestCallerDirectory"),
          replyCallerDirectory("Dispatcher-ReplyCallerDirectory"),
          publicationManager(nullptr),
          subscriptionManager(nullptr),
          handleReceivedMessageThreadPool("Dispatcher", maxThreads),
          subscriptionHandlingMutex()
{
}

Dispatcher::~Dispatcher()
{
    JOYNR_LOG_DEBUG(logger, "Destructing Dispatcher");
    handleReceivedMessageThreadPool.shutdown();
    delete publicationManager;
    delete subscriptionManager;
    publicationManager = nullptr;
    subscriptionManager = nullptr;
    JOYNR_LOG_DEBUG(logger, "Destructing finished");
}

void Dispatcher::addRequestCaller(const std::string& participantId,
                                  std::shared_ptr<RequestCaller> requestCaller)
{
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    JOYNR_LOG_DEBUG(logger, "addRequestCaller id= {}", participantId);
    requestCallerDirectory.add(participantId, requestCaller);

    if (publicationManager != nullptr) {
        // publication manager queues received subscription requests, that are
        // received before the corresponding request caller is added
        publicationManager->restore(participantId, requestCaller, messageSender);
    } else {
        JOYNR_LOG_DEBUG(logger, "No publication manager available!");
    }
}

void Dispatcher::removeRequestCaller(const std::string& participantId)
{
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    JOYNR_LOG_DEBUG(logger, "removeRequestCaller id= {}", participantId);
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
    JOYNR_LOG_DEBUG(logger, "addReplyCaller id= {}", requestReplyId);
    // add the callback to the registry that is responsible for reply messages
    replyCallerDirectory.add(requestReplyId, replyCaller, qosSettings.getTtl());
}

void Dispatcher::removeReplyCaller(const std::string& requestReplyId)
{
    JOYNR_LOG_DEBUG(logger, "removeReplyCaller id= {}", requestReplyId);
    replyCallerDirectory.remove(requestReplyId);
}

void Dispatcher::receive(const JoynrMessage& message)
{
    JOYNR_LOG_DEBUG(logger, "receive(message). Message payload: {}", message.getPayload());
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
    try {
        Request request = deserializePayload<Request>(message);
        const std::string& requestReplyId = request.getRequestReplyId();
        JoynrTimePoint requestExpiryDate = message.getHeaderExpiryDate();

        auto onSuccess =
                [requestReplyId, requestExpiryDate, this, senderId, receiverId](Reply&& reply) {
            reply.setRequestReplyId(requestReplyId);
            // reply.setResponseVariant(std::move(returnValueVar));
            // send reply back to the original sender (ie. sender and receiver ids are reversed
            // on
            // purpose)
            JOYNR_LOG_DEBUG(logger,
                            "Got reply from RequestInterpreter for requestReplyId {}",
                            requestReplyId);
            JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now());
            std::int64_t ttl = std::chrono::duration_cast<std::chrono::milliseconds>(
                                       requestExpiryDate - now).count();
            messageSender->sendReply(receiverId, // receiver of the request is sender of reply
                                     senderId,   // sender of request is receiver of reply
                                     MessagingQos(ttl),
                                     std::move(reply));
        };

        auto onError = [requestReplyId, requestExpiryDate, this, senderId, receiverId](
                const exceptions::JoynrException& exception) {
            Reply reply;
            reply.setRequestReplyId(requestReplyId);
            reply.setErrorVariant(joynr::exceptions::JoynrExceptionUtil::createVariant(exception));
            JOYNR_LOG_DEBUG(logger,
                            "Got error reply from RequestInterpreter for requestReplyId {}",
                            requestReplyId);
            JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now());
            std::int64_t ttl = std::chrono::duration_cast<std::chrono::milliseconds>(
                                       requestExpiryDate - now).count();
            messageSender->sendReply(receiverId, // receiver of the request is sender of reply
                                     senderId,   // sender of request is receiver of reply
                                     MessagingQos(ttl),
                                     std::move(reply));
        };
        // execute request
        requestInterpreter->execute(caller, request, onSuccess, onError);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize request object from: {} - error: {}",
                        message.getPayload(),
                        e.what());
        return;
    }
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
    try {
        OneWayRequest request = deserializePayload<OneWayRequest>(message);
        // execute request
        requestInterpreter->execute(caller, request);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize request object from: {} - error: {}",
                        message.getPayload(),
                        e.what());
        return;
    }
}

void Dispatcher::handleReplyReceived(const JoynrMessage& message)
{
    // deserialize the Reply
    try {
        Reply reply = deserializePayload<Reply>(message);
        std::string requestReplyId = reply.getRequestReplyId();
        std::shared_ptr<IReplyCaller> caller = replyCallerDirectory.lookup(requestReplyId);
        if (caller == nullptr) {
            // This used to be a fatal error, but it is possible that the replyCallerDirectory
            // removed
            // the caller
            // because its lifetime exceeded TTL
            JOYNR_LOG_INFO(
                    logger,
                    "caller not found in the ReplyCallerDirectory for requestid {}, ignoring",
                    requestReplyId);
            return;
        }

        caller->execute(std::move(reply));

        // Clean up
        removeReplyCaller(requestReplyId);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize reply object from: {} - error {}",
                        message.getPayload(),
                        e.what());
    }
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

    try {
        // PublicationManager is responsible for deleting SubscriptionRequests
        SubscriptionRequest subscriptionRequest =
                JsonSerializer::deserialize<SubscriptionRequest>(jsonSubscriptionRequest);

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
                                    messageSender);
        }
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize subscription request object from: {} - error: {}",
                        jsonSubscriptionRequest,
                        e.what());
        return;
    }
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
    try {
        BroadcastSubscriptionRequest subscriptionRequest =
                JsonSerializer::deserialize<BroadcastSubscriptionRequest>(jsonSubscriptionRequest);

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
                                    messageSender);
        }
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(
                logger,
                "Unable to deserialize broadcast subscription request object from: {} - error: {}",
                jsonSubscriptionRequest,
                e.what());
    }
}

void Dispatcher::handleSubscriptionStopReceived(const JoynrMessage& message)
{
    JOYNR_LOG_DEBUG(logger, "handleSubscriptionStopReceived");
    std::string jsonSubscriptionStop = message.getPayload();

    std::string subscriptionId;
    try {
        SubscriptionStop subscriptionStop =
                JsonSerializer::deserialize<SubscriptionStop>(jsonSubscriptionStop);

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

void Dispatcher::handlePublicationReceived(const JoynrMessage& message)
{
    std::string jsonSubscriptionPublication = message.getPayload();

    try {
        SubscriptionPublication subscriptionPublication =
                JsonSerializer::deserialize<SubscriptionPublication>(jsonSubscriptionPublication);

        std::string subscriptionId = subscriptionPublication.getSubscriptionId();

        assert(subscriptionManager != nullptr);

        std::shared_ptr<ISubscriptionCallback> callback =
                subscriptionManager->getSubscriptionCallback(subscriptionId);
        if (!callback) {
            JOYNR_LOG_ERROR(logger,
                            "Dropping reply for non/no more existing subscription with id = {}",
                            subscriptionId);
            return;
        }

        subscriptionManager->touchSubscriptionState(subscriptionId);

        int typeId = callback->getTypeId();

        // Get the publication interpreter - this has to be a reference to support
        // PublicationInterpreter polymorphism
        IPublicationInterpreter& interpreter =
                MetaTypeRegistrar::instance().getPublicationInterpreter(typeId);
        interpreter.execute(callback, subscriptionPublication);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(
                logger,
                "Unable to deserialize subscription publication object from: {} - error: {}",
                jsonSubscriptionPublication,
                e.what());
    }
}

void Dispatcher::registerSubscriptionManager(ISubscriptionManager* subscriptionManager)
{
    this->subscriptionManager = subscriptionManager;
}

void Dispatcher::registerPublicationManager(PublicationManager* publicationManager)
{
    this->publicationManager = publicationManager;
}

template <typename T>
T Dispatcher::deserializePayload(const JoynrMessage& message)
{
    using Stream = muesli::StringIStream;
    T payload;
    Stream stream(message.getPayload());
    auto archive = std::make_shared<muesli::JsonInputArchive<Stream>>(stream);
    (*archive)(payload);

    return payload;
}

} // namespace joynr
