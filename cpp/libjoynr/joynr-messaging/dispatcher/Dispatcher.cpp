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
/*
 * Dispatcher.cpp
 *
 *  Created on: Aug 12, 2011
 *      Author: grape
 */
#include "joynr/DispatcherUtils.h"
#include "joynr/Dispatcher.h"
#include "joynr/SubscriptionRequest.h"
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
#include "joynr/SubscriptionManager.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/MetaTypeRegistrar.h"
#include "joynr/Request.h"

#include <QUuid>
#include <QDateTime>
#include <cassert>

namespace joynr {

using namespace joynr_logging;

Logger* Dispatcher::logger = Logging::getInstance()->getLogger("MSG", "Dispatcher");

Dispatcher::Dispatcher(JoynrMessageSender* messageSender, int maxThreads)
        : messageSender(messageSender),
          requestCallerDirectory(QString("Dispatcher-RequestCallerDirectory")),
          replyCallerDirectory(QString("Dispatcher-ReplyCallerDirectory")),
          publicationManager(NULL),
          subscriptionManager(NULL),
          handleReceivedMessageThreadPool(),
          subscriptionHandlingMutex()

{
    handleReceivedMessageThreadPool.setMaxThreadCount(maxThreads);

    // Register metatypes
    qRegisterMetaType<Request>();
    qRegisterMetaType<Reply>();
    qRegisterMetaType<SubscriptionRequest>();
}

Dispatcher::~Dispatcher() {
    LOG_DEBUG(logger, "Destructing Dispatcher");
    handleReceivedMessageThreadPool.waitForDone();
    delete publicationManager;
    delete subscriptionManager;
    publicationManager = NULL;
    subscriptionManager = NULL;
    LOG_DEBUG(logger, "Destructing finished");
}

void Dispatcher::addRequestCaller(
        const QString& participantId,
        QSharedPointer<RequestCaller> requestCaller)
{
    QMutexLocker locker(&subscriptionHandlingMutex);
    LOG_DEBUG(logger, "addRequestCaller id= " + participantId );
    requestCallerDirectory.add(participantId, requestCaller);

    if(publicationManager != NULL) {
        // publication manager queues received subscription requests, that are
        // received before the corresponding request caller is added
        publicationManager->restore(participantId, requestCaller, messageSender);
    } else {
        LOG_DEBUG(logger, "No publication manager available!");
    }
}

void Dispatcher::removeRequestCaller(const QString &participantId)
{
    QMutexLocker locker(&subscriptionHandlingMutex);
    LOG_DEBUG(logger, "removeRequestCaller id= " +participantId);
    // TODO if a provider is removed, all publication runnables are stopped
    // the subscription request is deleted,
    // Q: Should it be restored once the provider is registered again?
    publicationManager->removeAllSubscriptions(participantId);
    requestCallerDirectory.remove(participantId);
}


void Dispatcher::addReplyCaller(
        const QString& requestReplyId,
        QSharedPointer<IReplyCaller> replyCaller,
        const MessagingQos& qosSettings)
{
    LOG_DEBUG(logger, "addReplyCaller id= " +requestReplyId);
    // add the callback to the registry that is responsible for reply messages
    replyCallerDirectory.add(requestReplyId,
                             replyCaller,
                             qosSettings.getTtl());

}

void Dispatcher::removeReplyCaller(const QString &requestReplyId)
{
    LOG_DEBUG(logger, "removeReplyCaller id= " +requestReplyId);
    replyCallerDirectory.remove(requestReplyId);
}

void Dispatcher::receive(const JoynrMessage& message, const MessagingQos& qos)
{
    LOG_DEBUG(logger, "receive: entered");
    ReceivedMessageRunnable* receivedMessageRunnable = new ReceivedMessageRunnable(
                DispatcherUtils::convertTtlToAbsoluteTime( qos.getTtl() ),
                message,
                qos,
                *this);
    handleReceivedMessageThreadPool.start(receivedMessageRunnable);
}


void Dispatcher::handleRequestReceived(const JoynrMessage& message, const MessagingQos& qos)
{

    QString senderId = message.getHeaderFrom();
    QString receiverId = message.getHeaderTo();

    // json request
    // lookup necessary data
    QByteArray jsonRequest = message.getPayload();
    QSharedPointer<RequestCaller> caller = requestCallerDirectory.lookup(receiverId);
    if (caller == NULL) {
        LOG_ERROR(logger, "caller not found in the RequestCallerDirectory for receiverId " + receiverId + ", ignoring");
        return;
    }
    QString interfaceName = caller->getInterfaceName();

    // Get the request interpreter that has been registered with this interface name
    QSharedPointer<IRequestInterpreter> requestInterpreter =
            InterfaceRegistrar::instance().getRequestInterpreter(interfaceName);

    // deserialize json
    Request* request = JsonSerializer::deserialize<Request>(jsonRequest);
    QString requestReplyId = request->getRequestReplyId();

    // execute request
    QVariant returnValueQVar = requestInterpreter->execute(
                caller,
                request->getMethodName(),
                request->getParams(),
                request->getParamDatatypes()
    );

    // send reply back to the original sender (ie. sender and receiver ids are reversed on purpose)
    Reply reply;
    reply.setRequestReplyId(requestReplyId);
    reply.setResponse(returnValueQVar);
    messageSender->sendReply(
                receiverId, // receiver of the request is sender of reply
                senderId,   // sender of request is receiver of reply
                qos,
                reply
    );

    delete request;
}


void Dispatcher::handleReplyReceived(const JoynrMessage& message)
{
    // json request
    // lookup necessary data
    QByteArray jsonReply = message.getPayload();

    //deserialize the jsonReply
    Reply* reply = JsonSerializer::deserialize<Reply>(jsonReply);
    if (reply == NULL){
        LOG_FATAL(logger, QString("Could not convert jsonReply %1 into Reply object")
                            .arg(QString::fromUtf8(jsonReply)));
        assert(false);
    }
    QString requestReplyId = reply->getRequestReplyId();

    QSharedPointer<IReplyCaller> caller = replyCallerDirectory.lookup(requestReplyId);
    if (caller == NULL) {
        // This used to be a fatal error, but it is possible that the replyCallerDirectory removed the caller
        // because its lifetime exceeded TTL
        LOG_INFO(logger, "caller not found in the ReplyCallerDirectory for requestid " + requestReplyId + ", ignoring");
        return;
    }

    // Get the reply interpreter - this has to be a reference to support ReplyInterpreter polymorphism
    int typeId = caller->getTypeId();
    IReplyInterpreter& interpreter = MetaTypeRegistrar::instance().getReplyInterpreter(typeId);

    // pass reply
    interpreter.execute(caller, *reply);

    // Clean up
    delete reply;
    removeReplyCaller(requestReplyId);
}


void Dispatcher::handleSubscriptionRequestReceived(const JoynrMessage& message) {
    LOG_TRACE(logger, "Starting handleSubscriptionReceived");
    //Make sure that noone is registering a Caller at the moment, because a racing condition could occour.
    QMutexLocker locker(&subscriptionHandlingMutex);
    assert(publicationManager != NULL);

    QString receiverId = message.getHeaderTo();
    QSharedPointer<RequestCaller> caller = requestCallerDirectory.lookup(receiverId);

    QByteArray jsonSubscriptionRequest = message.getPayload();

    // PublicationManager is responsible for deleting SubscriptionRequests
    SubscriptionRequest* subscriptionRequest = JsonSerializer::deserialize<SubscriptionRequest>(
                jsonSubscriptionRequest
    );

    if(caller.isNull()) {
        // Provider not registered yet
        // Dispatcher will call publicationManger->restore when a new provider is added to activate
        // subscriptions for that provider
        publicationManager->add(message.getHeaderFrom(), message.getHeaderTo(), subscriptionRequest);
    } else {
        publicationManager->add(message.getHeaderFrom(), message.getHeaderTo(), caller, subscriptionRequest, messageSender);
    }
}


void Dispatcher::handleSubscriptionStopReceived(const JoynrMessage& message) {
    LOG_DEBUG(logger, "handleSubscriptionStopReceived");
    QByteArray jsonSubscriptionStop = message.getPayload();

    SubscriptionStop* subscriptionStop = JsonSerializer::deserialize<SubscriptionStop>(
                jsonSubscriptionStop
    );
    QString subscriptionId = subscriptionStop->getSubscriptionId();
    assert(publicationManager != NULL);
    publicationManager->stopPublication(subscriptionId);
}

void Dispatcher::handlePublicationReceived(const JoynrMessage& message)
{
    QByteArray jsonSubscriptionPublication = message.getPayload();

    SubscriptionPublication* subscriptionPublication = JsonSerializer::deserialize<SubscriptionPublication>(
                jsonSubscriptionPublication
    );
    QString subscriptionId = subscriptionPublication->getSubscriptionId();

    assert(subscriptionManager != NULL);

    QSharedPointer<ISubscriptionCallback> callback = subscriptionManager->getSubscriptionCallback(subscriptionId);
    if(callback.isNull()) {
        LOG_ERROR(logger, "Dropping reply for non/no more existing subscription with id=" +subscriptionId);
        delete subscriptionPublication;
        return;
    }

    subscriptionManager->touchSubscriptionState(subscriptionId);

    int typeId = callback->getTypeId();

    // Get the publication interpreter - this has to be a reference to support PublicationInterpreter polymorphism
    IPublicationInterpreter& interpreter = MetaTypeRegistrar::instance().getPublicationInterpreter(typeId);
    interpreter.execute(callback, *subscriptionPublication);

    delete subscriptionPublication;
}

void Dispatcher::registerSubscriptionManager(SubscriptionManager* subscriptionManager) {
    this->subscriptionManager = subscriptionManager;
}

void Dispatcher::registerPublicationManager(PublicationManager* publicationManager) {
    this->publicationManager = publicationManager;
}



} // namespace joynr
