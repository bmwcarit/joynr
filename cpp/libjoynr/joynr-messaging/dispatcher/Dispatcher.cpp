/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
/*
 * Dispatcher.cpp
 *
 *  Created on: Aug 12, 2011
 *      Author: grape
 */
#include "joynr/DispatcherUtils.h"
#include "joynr/Dispatcher.h"
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
#include "joynr/IReplyInterpreter.h"
#include "libjoynr/joynr-messaging/dispatcher/ReceivedMessageRunnable.h"
#include "joynr/PublicationInterpreter.h"
#include "joynr/PublicationManager.h"
#include "joynr/ISubscriptionManager.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/MetaTypeRegistrar.h"
#include "joynr/Request.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"

#include <QUuid>
#include <chrono>
#include <stdint.h>
#include <cassert>

namespace joynr
{

using namespace joynr_logging;
using namespace std::chrono;

Logger* Dispatcher::logger = Logging::getInstance()->getLogger("MSG", "Dispatcher");

Dispatcher::Dispatcher(JoynrMessageSender* messageSender, int maxThreads)
        : messageSender(messageSender),
          requestCallerDirectory("Dispatcher-RequestCallerDirectory"),
          replyCallerDirectory("Dispatcher-ReplyCallerDirectory"),
          publicationManager(NULL),
          subscriptionManager(NULL),
          handleReceivedMessageThreadPool("Dispatcher", maxThreads),
          subscriptionHandlingMutex()

{
    // Register metatypes
    qRegisterMetaType<Request>();
    qRegisterMetaType<Reply>();
    qRegisterMetaType<SubscriptionRequest>();
}

Dispatcher::~Dispatcher()
{
    LOG_DEBUG(logger, "Destructing Dispatcher");
    handleReceivedMessageThreadPool.shutdown();
    delete publicationManager;
    delete subscriptionManager;
    publicationManager = NULL;
    subscriptionManager = NULL;
    LOG_DEBUG(logger, "Destructing finished");
}

void Dispatcher::addRequestCaller(const std::string& participantId,
                                  std::shared_ptr<RequestCaller> requestCaller)
{
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    LOG_DEBUG(logger, "addRequestCaller id= " + QString::fromStdString(participantId));
    requestCallerDirectory.add(participantId, requestCaller);

    if (publicationManager != NULL) {
        // publication manager queues received subscription requests, that are
        // received before the corresponding request caller is added
        publicationManager->restore(
                QString::fromStdString(participantId), requestCaller, messageSender);
    } else {
        LOG_DEBUG(logger, "No publication manager available!");
    }
}

void Dispatcher::removeRequestCaller(const std::string& participantId)
{
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    LOG_DEBUG(logger, "removeRequestCaller id= " + QString::fromStdString(participantId));
    // TODO if a provider is removed, all publication runnables are stopped
    // the subscription request is deleted,
    // Q: Should it be restored once the provider is registered again?
    publicationManager->removeAllSubscriptions(QString::fromStdString(participantId));
    requestCallerDirectory.remove(participantId);
}

void Dispatcher::addReplyCaller(const std::string& requestReplyId,
                                std::shared_ptr<IReplyCaller> replyCaller,
                                const MessagingQos& qosSettings)
{
    LOG_DEBUG(logger,
              QString("addReplyCaller id=%1 typeId=%2")
                      .arg(QString::fromStdString(requestReplyId))
                      .arg(replyCaller->getTypeId()));
    // add the callback to the registry that is responsible for reply messages
    replyCallerDirectory.add(requestReplyId, replyCaller, qosSettings.getTtl());
}

void Dispatcher::removeReplyCaller(const std::string& requestReplyId)
{
    LOG_DEBUG(logger, "removeReplyCaller id= " + QString::fromStdString(requestReplyId));
    replyCallerDirectory.remove(requestReplyId);
}

void Dispatcher::receive(const JoynrMessage& message)
{
    LOG_DEBUG(logger,
              QString("receive(message). Message payload: %1")
                      .arg(QString::fromStdString(message.getPayload())));
    ReceivedMessageRunnable* receivedMessageRunnable = new ReceivedMessageRunnable(message, *this);
    handleReceivedMessageThreadPool.execute(receivedMessageRunnable);
}

void Dispatcher::handleRequestReceived(const JoynrMessage& message)
{
    std::string senderId = message.getHeaderFrom();
    std::string receiverId = message.getHeaderTo();

    // json request
    // lookup necessary data
    std::string jsonRequest = message.getPayload();
    std::shared_ptr<RequestCaller> caller = requestCallerDirectory.lookup(receiverId);
    if (caller == NULL) {
        LOG_ERROR(logger,
                  "caller not found in the RequestCallerDirectory for receiverId " +
                          QString::fromStdString(receiverId) + ", ignoring");
        return;
    }
    std::string interfaceName = caller->getInterfaceName();

    // Get the request interpreter that has been registered with this interface name
    std::shared_ptr<IRequestInterpreter> requestInterpreter =
            InterfaceRegistrar::instance().getRequestInterpreter(interfaceName);

    // deserialize json
    Request* request = JsonSerializer::deserialize<Request>(jsonRequest);
    if (request == Q_NULLPTR) {
        LOG_ERROR(logger,
                  QString("Unable to deserialize request object from: %1")
                          .arg(QString::fromStdString(jsonRequest)));
        return;
    }

    std::string requestReplyId = request->getRequestReplyId();
    JoynrTimePoint requestExpiryDate = message.getHeaderExpiryDate();

    std::function<void(std::vector<Variant>)> onSuccess =
            [requestReplyId, requestExpiryDate, this, senderId, receiverId](
                    std::vector<Variant> returnValueVar) {
        Reply reply;
        reply.setRequestReplyId(requestReplyId);
        reply.setResponse(std::move(returnValueVar));
        // send reply back to the original sender (ie. sender and receiver ids are reversed
        // on
        // purpose)
        LOG_DEBUG(logger,
                  QString("Got reply from RequestInterpreter for requestReplyId %1")
                          .arg(TypeUtil::toQt(requestReplyId)));
        JoynrTimePoint now = time_point_cast<milliseconds>(system_clock::now());
        int64_t ttl = duration_cast<milliseconds>(requestExpiryDate - now).count();
        messageSender->sendReply(receiverId, // receiver of the request is sender of reply
                                 senderId,   // sender of request is receiver of reply
                                 MessagingQos(ttl),
                                 reply);
    };

    std::function<void(const exceptions::JoynrException&)> onError =
            [requestReplyId, requestExpiryDate, this, senderId, receiverId](
                    const exceptions::JoynrException& exception) {
        Reply reply;
        reply.setRequestReplyId(requestReplyId);
        reply.setError(joynr::exceptions::JoynrExceptionUtil::createVariant(exception));
        LOG_DEBUG(logger,
                  QString("Got error reply from RequestInterpreter for requestReplyId %1")
                          .arg(TypeUtil::toQt(requestReplyId)));
        JoynrTimePoint now = time_point_cast<milliseconds>(system_clock::now());
        int64_t ttl = duration_cast<milliseconds>(requestExpiryDate - now).count();
        messageSender->sendReply(receiverId, // receiver of the request is sender of reply
                                 senderId,   // sender of request is receiver of reply
                                 MessagingQos(ttl),
                                 reply);
    };
    // execute request
    try {
        requestInterpreter->execute(caller,
                                    request->getMethodName(),
                                    request->getParams(),
                                    request->getParamDatatypes(),
                                    onSuccess,
                                    onError);
        // ApplicationExceptions should not be created by the application itself to ensure
        // serializability. They are treated as JoynrExceptions. They can only be handled correctly
        // if the constructor is used properly (with the appropriate literal of the reported error
        // enumeration).
    } catch (exceptions::ProviderRuntimeException& e) {
        std::string message = "Could not perform an RPC invocation, caught exception: " +
                              e.getTypeName() + ":" + e.getMessage();
        LOG_ERROR(logger, message.c_str());
        onError(e);
    } catch (exceptions::JoynrException& e) {
        std::string message = "Could not perform an RPC invocation, caught exception: " +
                              e.getTypeName() + ":" + e.getMessage();
        LOG_ERROR(logger, message.c_str());
        onError(exceptions::ProviderRuntimeException("caught exception: " + e.getTypeName() + ":" +
                                                     e.getMessage()));
    }

    delete request;
}

void Dispatcher::handleReplyReceived(const JoynrMessage& message)
{
    // json request
    // lookup necessary data
    std::string jsonReply = message.getPayload();

    // deserialize the jsonReply
    Reply* reply = JsonSerializer::deserialize<Reply>(jsonReply);
    if (reply == Q_NULLPTR) {
        LOG_ERROR(logger,
                  QString("Unable to deserialize reply object from: %1")
                          .arg(QString::fromStdString(jsonReply)));
        return;
    }
    std::string requestReplyId = reply->getRequestReplyId();

    std::shared_ptr<IReplyCaller> caller = replyCallerDirectory.lookup(requestReplyId);
    if (caller == NULL) {
        // This used to be a fatal error, but it is possible that the replyCallerDirectory removed
        // the caller
        // because its lifetime exceeded TTL
        LOG_INFO(logger,
                 QString::fromStdString(
                         "caller not found in the ReplyCallerDirectory for requestid " +
                         requestReplyId + ", ignoring"));
        return;
    }

    // Get the reply interpreter - this has to be a reference to support ReplyInterpreter
    // polymorphism
    int typeId = caller->getTypeId();
    IReplyInterpreter& interpreter = MetaTypeRegistrar::instance().getReplyInterpreter(typeId);

    // pass reply
    interpreter.execute(caller, *reply);

    // Clean up
    delete reply;
    removeReplyCaller(requestReplyId);
}

void Dispatcher::handleSubscriptionRequestReceived(const JoynrMessage& message)
{
    LOG_TRACE(logger, "Starting handleSubscriptionReceived");
    // Make sure that noone is registering a Caller at the moment, because a racing condition could
    // occour.
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    assert(publicationManager != NULL);

    QString receiverId = QString::fromStdString(message.getHeaderTo());
    std::shared_ptr<RequestCaller> caller = requestCallerDirectory.lookup(receiverId.toStdString());

    std::string jsonSubscriptionRequest = message.getPayload();

    // PublicationManager is responsible for deleting SubscriptionRequests
    SubscriptionRequest* subscriptionRequest =
            JsonSerializer::deserialize<SubscriptionRequest>(jsonSubscriptionRequest);
    if (subscriptionRequest == Q_NULLPTR) {
        LOG_ERROR(logger,
                  QString("Unable to deserialize subscription request object from: %1")
                          .arg(QString::fromStdString(jsonSubscriptionRequest)));
        return;
    }

    if (!caller) {
        // Provider not registered yet
        // Dispatcher will call publicationManger->restore when a new provider is added to activate
        // subscriptions for that provider
        publicationManager->add(QString::fromStdString(message.getHeaderFrom()),
                                QString::fromStdString(message.getHeaderTo()),
                                *subscriptionRequest);
    } else {
        publicationManager->add(QString::fromStdString(message.getHeaderFrom()),
                                QString::fromStdString(message.getHeaderTo()),
                                caller,
                                *subscriptionRequest,
                                messageSender);
    }
    delete subscriptionRequest;
}

void Dispatcher::handleBroadcastSubscriptionRequestReceived(const JoynrMessage& message)
{
    LOG_TRACE(logger, "Starting handleBroadcastSubscriptionRequestReceived");
    // Make sure that noone is registering a Caller at the moment, because a racing condition could
    // occour.
    std::lock_guard<std::mutex> lock(subscriptionHandlingMutex);
    assert(publicationManager != NULL);

    QString receiverId = QString::fromStdString(message.getHeaderTo());
    std::shared_ptr<RequestCaller> caller = requestCallerDirectory.lookup(receiverId.toStdString());

    std::string jsonSubscriptionRequest = message.getPayload();

    // PublicationManager is responsible for deleting SubscriptionRequests
    BroadcastSubscriptionRequest* subscriptionRequest =
            JsonSerializer::deserialize<BroadcastSubscriptionRequest>(jsonSubscriptionRequest);
    if (subscriptionRequest == Q_NULLPTR) {
        LOG_ERROR(logger,
                  QString("Unable to deserialize broadcast subscription request object from: %1")
                          .arg(QString::fromStdString(jsonSubscriptionRequest)));
        return;
    }

    if (!caller) {
        // Provider not registered yet
        // Dispatcher will call publicationManger->restore when a new provider is added to activate
        // subscriptions for that provider
        publicationManager->add(QString::fromStdString(message.getHeaderFrom()),
                                QString::fromStdString(message.getHeaderTo()),
                                *subscriptionRequest);
    } else {
        publicationManager->add(QString::fromStdString(message.getHeaderFrom()),
                                QString::fromStdString(message.getHeaderTo()),
                                caller,
                                *subscriptionRequest,
                                messageSender);
    }
    delete subscriptionRequest;
}

void Dispatcher::handleSubscriptionStopReceived(const JoynrMessage& message)
{
    LOG_DEBUG(logger, "handleSubscriptionStopReceived");
    std::string jsonSubscriptionStop = message.getPayload();

    SubscriptionStop* subscriptionStop =
            JsonSerializer::deserialize<SubscriptionStop>(jsonSubscriptionStop);
    if (subscriptionStop == Q_NULLPTR) {
        LOG_ERROR(logger,
                  QString("Unable to deserialize subscription stop object from: %1")
                          .arg(QString::fromStdString(jsonSubscriptionStop)));
        return;
    }
    QString subscriptionId = TypeUtil::toQt(subscriptionStop->getSubscriptionId());
    assert(publicationManager != NULL);
    publicationManager->stopPublication(subscriptionId);
}

void Dispatcher::handlePublicationReceived(const JoynrMessage& message)
{
    std::string jsonSubscriptionPublication = message.getPayload();

    SubscriptionPublication* subscriptionPublication =
            JsonSerializer::deserialize<SubscriptionPublication>(jsonSubscriptionPublication);
    if (subscriptionPublication == Q_NULLPTR) {
        LOG_ERROR(logger,
                  QString("Unable to deserialize subscription publication object from: %1")
                          .arg(QString::fromStdString(jsonSubscriptionPublication)));
        return;
    }
    QString subscriptionId = QString::fromStdString(subscriptionPublication->getSubscriptionId());

    assert(subscriptionManager != NULL);

    std::shared_ptr<ISubscriptionCallback> callback =
            subscriptionManager->getSubscriptionCallback(subscriptionId);
    if (!callback) {
        LOG_ERROR(logger,
                  "Dropping reply for non/no more existing subscription with id=" + subscriptionId);
        delete subscriptionPublication;
        return;
    }

    subscriptionManager->touchSubscriptionState(subscriptionId);

    int typeId = callback->getTypeId();

    // Get the publication interpreter - this has to be a reference to support
    // PublicationInterpreter polymorphism
    IPublicationInterpreter& interpreter =
            MetaTypeRegistrar::instance().getPublicationInterpreter(typeId);
    interpreter.execute(callback, *subscriptionPublication);

    delete subscriptionPublication;
}

void Dispatcher::registerSubscriptionManager(ISubscriptionManager* subscriptionManager)
{
    this->subscriptionManager = subscriptionManager;
}

void Dispatcher::registerPublicationManager(PublicationManager* publicationManager)
{
    this->publicationManager = publicationManager;
}

} // namespace joynr
