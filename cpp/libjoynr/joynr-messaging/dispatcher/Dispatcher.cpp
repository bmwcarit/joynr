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
#include "joynr/IMessageSender.h"
#include "joynr/IReplyCaller.h"
#include "joynr/IRequestInterpreter.h"
#include "joynr/ISubscriptionCallback.h"
#include "joynr/ISubscriptionManager.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/MessagingQos.h"
#include "joynr/MessagingQosEffort.h"
#include "joynr/MulticastPublication.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/OneWayRequest.h"
#include "joynr/PublicationManager.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/RequestCaller.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/ThreadPool.h"
#include "joynr/TimePoint.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/types/Version.h"

#include "ReceivedMessageRunnable.h"

namespace joynr
{

Dispatcher::Dispatcher(std::shared_ptr<IMessageSender> messageSender,
                       boost::asio::io_service& ioService)
        : std::enable_shared_from_this<Dispatcher>(),
          IDispatcher(),
          _messageSender(std::move(messageSender)),
          _requestCallerDirectory("Dispatcher-RequestCallerDirectory", ioService),
          _replyCallerDirectory("Dispatcher-ReplyCallerDirectory", ioService),
          _publicationManager(),
          _subscriptionManager(nullptr),
          _handleReceivedMessageThreadPool(std::make_shared<ThreadPool>("Dispatcher", 1)),
          _subscriptionHandlingMutex(),
          _isShuttingDown(false),
          _isShuttingDownLock()
{
    _handleReceivedMessageThreadPool->init();
}

Dispatcher::~Dispatcher()
{
    JOYNR_LOG_TRACE(logger(), "Destructing Dispatcher");
    assert(_isShuttingDown);
    JOYNR_LOG_TRACE(logger(), "Destructing finished");
}

void Dispatcher::addRequestCaller(const std::string& participantId,
                                  std::shared_ptr<RequestCaller> requestCaller)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(
                logger(), "addRequestCaller id= {} cancelled, shutting down", participantId);
        return;
    }

    std::lock_guard<std::mutex> lock(_subscriptionHandlingMutex);
    JOYNR_LOG_TRACE(logger(), "addRequestCaller id= {}", participantId);

    _requestCallerDirectory.add(participantId, requestCaller);
    locker.unlock();

    if (auto publicationManagerSharedPtr = _publicationManager.lock()) {
        // publication manager queues received subscription requests, that are
        // received before the corresponding request caller is added
        publicationManagerSharedPtr->restore(
                participantId, std::move(requestCaller), _messageSender);
    } else {
        JOYNR_LOG_WARN(logger(), "No publication manager available!");
    }
}

void Dispatcher::removeRequestCaller(const std::string& participantId)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(
                logger(), "removeRequestCaller id= {} cancelled, shutting down", participantId);
        return;
    }
    std::lock_guard<std::mutex> lock(_subscriptionHandlingMutex);
    JOYNR_LOG_TRACE(logger(), "removeRequestCaller id= {}", participantId);
    locker.unlock();

    // TODO if a provider is removed, all publication runnables are stopped
    // the subscription request is deleted,
    // Q: Should it be restored once the provider is registered again?
    if (auto publicationManagerSharedPtr = _publicationManager.lock()) {
        publicationManagerSharedPtr->removeAllSubscriptions(participantId);
    }

    locker.lock();
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(
                logger(), "removeRequestCaller id= {} cancelled, shutting down", participantId);
        return;
    }
    _requestCallerDirectory.remove(participantId);
}

void Dispatcher::addReplyCaller(const std::string& requestReplyId,
                                std::shared_ptr<IReplyCaller> replyCaller,
                                const MessagingQos& qosSettings)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(), "addReplyCaller id= {} cancelled, shutting down", requestReplyId);
        return;
    }
    JOYNR_LOG_TRACE(logger(), "addReplyCaller id= {}", requestReplyId);
    // add the callback to the registry that is responsible for reply messages
    _replyCallerDirectory.add(requestReplyId,
                              std::move(replyCaller),
                              static_cast<std::int64_t>(qosSettings.getTtl()));
}

void Dispatcher::receive(std::shared_ptr<ImmutableMessage> message)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(),
                        "received message: {}, operation cancelled, shutting down",
                        message->toLogMessage());
        return;
    }
    JOYNR_LOG_TRACE(logger(), "received message: {}", message->toLogMessage());
    // we only support non-encrypted messages for now
    assert(!message->isEncrypted());
    std::shared_ptr<ReceivedMessageRunnable> receivedMessageRunnable =
            std::make_shared<ReceivedMessageRunnable>(std::move(message), shared_from_this());
    _handleReceivedMessageThreadPool->execute(receivedMessageRunnable);
}

void Dispatcher::handleRequestReceived(std::shared_ptr<ImmutableMessage> message)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(), "handleRequestReceived cancelled, shutting down");
        return;
    }
    std::string senderId = message->getSender();
    std::string receiverId = message->getRecipient();

    // lookup necessary data
    std::shared_ptr<RequestCaller> caller = _requestCallerDirectory.lookup(receiverId);
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
            InterfaceRegistrar::instance().getRequestInterpreter(
                    interfaceName + std::to_string(caller->getProviderVersion().getMajorVersion()));
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
    TimePoint requestExpiryDate = message->getExpiryDate();

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
            const std::chrono::milliseconds ttl = requestExpiryDate.relativeFromNow();
            MessagingQos messagingQos(static_cast<std::uint64_t>(ttl.count()));
            messagingQos.setCompress(message->isCompressed());
            const boost::optional<std::string> effort = message->getEffort();
            if (effort) {
                try {
                    messagingQos.setEffort(MessagingQosEffort::getEnum(*effort));
                } catch (const std::invalid_argument& e) {
                    JOYNR_LOG_ERROR(logger(),
                                    "Request message (id: {}) uses invalid effort: {}. Using "
                                    "default effort for reply message.",
                                    message->getId(),
                                    *effort);
                }
            }
            thisSharedPtr->_messageSender->sendReply(
                    receiverId, // receiver of the request is sender of reply
                    senderId,   // sender of request is receiver of reply
                    messagingQos,
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
        assert(exception);
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            JOYNR_LOG_WARN(logger(),
                           "Got error '{}' from RequestInterpreter for requestReplyId {}",
                           exception->getMessage(),
                           requestReplyId);
            Reply reply;
            reply.setRequestReplyId(std::move(requestReplyId));
            reply.setError(exception);
            const std::chrono::milliseconds ttl = requestExpiryDate.relativeFromNow();
            MessagingQos messagingQos(static_cast<std::uint64_t>(ttl.count()));
            messagingQos.setCompress(message->isCompressed());
            thisSharedPtr->_messageSender->sendReply(
                    receiverId, // receiver of the request is sender of reply
                    senderId,   // sender of request is receiver of reply
                    messagingQos,
                    message->getPrefixedCustomHeaders(),
                    std::move(reply));
        }
    };
    locker.unlock();

    // execute request
    requestInterpreter->execute(
            std::move(caller), request, std::move(onSuccess), std::move(onError));
}

void Dispatcher::handleOneWayRequestReceived(std::shared_ptr<ImmutableMessage> message)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(), "handleOneWayRequestReceived cancelled, shutting down");
        return;
    }
    const std::string& receiverId = message->getRecipient();

    // json request
    // lookup necessary data
    std::shared_ptr<RequestCaller> caller = _requestCallerDirectory.lookup(receiverId);
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
            InterfaceRegistrar::instance().getRequestInterpreter(
                    interfaceName + std::to_string(caller->getProviderVersion().getMajorVersion()));

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
    locker.unlock();

    // execute request
    requestInterpreter->execute(std::move(caller), request);
}

void Dispatcher::handleReplyReceived(std::shared_ptr<ImmutableMessage> message)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(), "handleReplyReceived cancelled, shutting down");
        return;
    }
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
    std::shared_ptr<IReplyCaller> caller = _replyCallerDirectory.take(requestReplyId);
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
    locker.unlock();

    caller->execute(std::move(reply));
}

void Dispatcher::handleSubscriptionRequestReceived(std::shared_ptr<ImmutableMessage> message)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(), "handleSubscriptionRequestReceived cancelled, shutting down");
        return;
    }
    JOYNR_LOG_TRACE(logger(), "Starting handleSubscriptionReceived");
    // Make sure that noone is registering a Caller at the moment, because a racing condition could
    // occour.
    std::lock_guard<std::mutex> lock(_subscriptionHandlingMutex);
    auto publicationManagerSharedPtr = _publicationManager.lock();
    if (!publicationManagerSharedPtr) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to handle subscription request object from: {} - no publication "
                        "manager available",
                        message->toLogMessage());
        return;
    }

    const std::string& receiverId = message->getRecipient();
    std::shared_ptr<RequestCaller> caller = _requestCallerDirectory.lookup(receiverId);

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
    locker.unlock();

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
                                         _messageSender);
    }
}

void Dispatcher::handleMulticastSubscriptionRequestReceived(
        std::shared_ptr<ImmutableMessage> message)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(
                logger(), "handleMulticastSubscriptionRequestReceived cancelled, shutting down");
        return;
    }
    JOYNR_LOG_TRACE(logger(), "Starting handleMulticastSubscriptionRequestReceived");
    auto publicationManagerSharedPtr = _publicationManager.lock();
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
    locker.unlock();

    publicationManagerSharedPtr->add(
            message->getSender(), message->getRecipient(), subscriptionRequest, _messageSender);
}

void Dispatcher::handleBroadcastSubscriptionRequestReceived(
        std::shared_ptr<ImmutableMessage> message)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(
                logger(), "handleBroadcastSubscriptionRequestReceived cancelled, shutting down");
        return;
    }
    JOYNR_LOG_TRACE(logger(), "Starting handleBroadcastSubscriptionRequestReceived");
    // Make sure that noone is registering a Caller at the moment, because a racing condition could
    // occour.
    std::lock_guard<std::mutex> lock(_subscriptionHandlingMutex);
    auto publicationManagerSharedPtr = _publicationManager.lock();
    if (!publicationManagerSharedPtr) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to handle broadcast subscription request object from: {} - no "
                        "publication manager available",
                        message->toLogMessage());
        return;
    }

    const std::string& receiverId = message->getRecipient();
    std::shared_ptr<RequestCaller> caller = _requestCallerDirectory.lookup(receiverId);

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
    locker.unlock();

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
                                         _messageSender);
    }
}

void Dispatcher::handleSubscriptionStopReceived(std::shared_ptr<ImmutableMessage> message)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(), "handleSubscriptionStopReceived cancelled, shutting down");
        return;
    }
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
    auto publicationManagerSharedPtr = _publicationManager.lock();
    if (!publicationManagerSharedPtr) {
        JOYNR_LOG_ERROR(logger(),
                        "Unable to handle subscription stop object from: {} - no publication "
                        "manager available",
                        message->toLogMessage());
        return;
    }
    locker.unlock();

    publicationManagerSharedPtr->stopPublication(subscriptionStop.getSubscriptionId());
}

void Dispatcher::handleSubscriptionReplyReceived(std::shared_ptr<ImmutableMessage> message)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(), "handleSubscriptionReplyReceived cancelled, shutting down");
        return;
    }
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

    assert(_subscriptionManager != nullptr);

    std::shared_ptr<ISubscriptionCallback> callback =
            _subscriptionManager->getSubscriptionCallback(subscriptionId);
    if (!callback) {
        JOYNR_LOG_ERROR(logger(),
                        "Dropping subscription reply for non/no more existing subscription "
                        "with id = {}",
                        subscriptionId);
        return;
    }
    locker.unlock();

    callback->execute(std::move(subscriptionReply));
}

void Dispatcher::handleMulticastReceived(std::shared_ptr<ImmutableMessage> message)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(), "handleMulticastReceived cancelled, shutting down");
        return;
    }
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

    assert(_subscriptionManager != nullptr);

    std::shared_ptr<ISubscriptionCallback> callback =
            _subscriptionManager->getMulticastSubscriptionCallback(multicastId);
    if (callback == nullptr) {
        JOYNR_LOG_ERROR(logger(),
                        "Dropping multicast publication for non/no more existing subscription "
                        "with multicastId = {}",
                        multicastId);
        return;
    }
    locker.unlock();

    // TODO: enable for periodic attribute subscriptions
    // when MulticastPublication is extended by subscriptionId
    // subscriptionManager->touchSubscriptionState(subscriptionId);

    callback->execute(std::move(multicastPublication));
}

void Dispatcher::handlePublicationReceived(std::shared_ptr<ImmutableMessage> message)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(), "handlePublicationReceived cancelled, shutting down");
        return;
    }
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

    assert(_subscriptionManager != nullptr);

    std::shared_ptr<ISubscriptionCallback> callback =
            _subscriptionManager->getSubscriptionCallback(subscriptionId);
    if (!callback) {
        JOYNR_LOG_ERROR(logger(),
                        "Dropping publication for non/no more existing subscription with id = {}",
                        subscriptionId);
        return;
    }

    _subscriptionManager->touchSubscriptionState(subscriptionId);
    locker.unlock();

    callback->execute(std::move(subscriptionPublication));
}

void Dispatcher::registerSubscriptionManager(
        std::shared_ptr<ISubscriptionManager> subscriptionManager)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(), "registerSubscriptionManager cancelled, shutting down");
        return;
    }
    this->_subscriptionManager = std::move(subscriptionManager);
}

void Dispatcher::registerPublicationManager(std::weak_ptr<PublicationManager> publicationManager)
{
    ReadLocker locker(_isShuttingDownLock);
    if (_isShuttingDown) {
        JOYNR_LOG_TRACE(logger(), "registerPublicationManager cancelled, shutting down");
        return;
    }
    this->_publicationManager = std::move(publicationManager);
}

void Dispatcher::shutdown()
{
    {
        WriteLocker locker(_isShuttingDownLock);
        assert(!_isShuttingDown);
        _isShuttingDown = true;
    }
    _handleReceivedMessageThreadPool->shutdown();
    _replyCallerDirectory.shutdown();
    _requestCallerDirectory.shutdown();
}

} // namespace joynr
