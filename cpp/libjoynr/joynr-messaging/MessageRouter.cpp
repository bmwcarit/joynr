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
#include "joynr/MessageRouter.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/Directory.h"
#include "joynr/JoynrMessagingEndpointAddress.h"
#include "joynr/joynrlogging.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/system/Address.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/RequestStatus.h"
#include "joynr/RequestStatusCode.h"

#include <cassert>

namespace joynr {

using namespace joynr_logging;
Logger* MessageRouter::logger = Logging::getInstance()->getLogger("MSG", "MessageRouter");

MessageRouter::~MessageRouter() {
    threadPool.waitForDone();
    if(parentRouter != NULL) {
        delete parentRouter;
    }
    delete delayedScheduler;
    delete messagingStubFactory;
}

MessageRouter::MessageRouter(
        Directory<QString, joynr::system::Address>* routingTable,
        IMessagingStubFactory* messagingStubFactory,
        int messageSendRetryInterval,
        int maxThreads
) :
        joynr::system::RoutingProvider(joynr::types::ProviderQos(
                QList<joynr::types::CustomParameter>(), // custom provider parameters
                1,                                      // provider version
                1,                                      // provider priority
                joynr::types::ProviderScope::LOCAL,     // provider discovery scope
                false                                   // supports on change subscriptions
        )),
        messagingStubFactory(messagingStubFactory),
        routingTable(routingTable),
        threadPool(),
        delayedScheduler(),
        parentRouter(NULL),
        parentAddress(NULL),
        incomingAddress()
{
    threadPool.setMaxThreadCount(maxThreads);
    delayedScheduler = new ThreadPoolDelayedScheduler(threadPool, QString("MessageRouter-DelayedScheduler"), messageSendRetryInterval);
}

void MessageRouter::addProvisionedNextHop(QString participantId, QSharedPointer<joynr::system::Address> address) {
    routingTable->add(participantId, address);
}

void MessageRouter::setParentRouter(joynr::system::RoutingProxy* parentRouter, QSharedPointer<joynr::system::Address> parentAddress) {
    this->parentRouter = parentRouter;
    this->parentAddress = parentAddress;
}

void MessageRouter::setIncommingAddress(QSharedPointer<joynr::system::Address> incomingAddress) {
    this->incomingAddress = incomingAddress;
}

/**
  * Q (RDZ): What happens if the message cannot be forwarded? Exception? Log file entry?
  * Q (RDZ): When are messagingstubs removed? They are stored indefinitely in the factory
  */
void MessageRouter::route(const JoynrMessage& message, const MessagingQos& qos) {
    assert(messagingStubFactory != NULL);
    // neither JoynrMessage nor MessagingQos give a decaytime, so it doesn't make sense to check for
    // a passed TTL. The TTL itself is only relative, not absolute, so it cannot be used here.
    /*
    if (QDateTime::currentMSecsSinceEpoch() >  qos.getRoundTripTtl_ms()) {
        LOG_DEBUG(logger, "received an expired Message. Dropping the message");
        return;
    }
    */
    LOG_TRACE(logger, "Routing Message.");
    QString destinationPartId = message.getHeaderTo();
    QSharedPointer <joynr::system::Address> destAddress =
            routingTable->lookup(destinationPartId);
    if (destAddress.isNull()) {
        LOG_ERROR(logger, "No endpoint address found for participantId " + destinationPartId + ". Dropping the message");
        return;
    }
    QSharedPointer<IMessaging> messagingStub = messagingStubFactory
                                                ->create(destinationPartId,
                                                         destAddress);
    if (messagingStub.isNull()) {
        LOG_DEBUG(logger, "No send-stub found for endpoint address. Dropping the message");
        return;
    }
    //make runnable (to execute send on the stub) and schedule it
    threadPool.start(new MessageRunnable(
                         message,
                         qos,
                         messagingStub,
                         *delayedScheduler,
                         DispatcherUtils::convertTtlToAbsoluteTime(qos.getTtl()) )
                     );
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        QString participantId,
        joynr::system::ChannelAddress channelAddress
) {
    // TODO check if routing table is thread-safe
    QSharedPointer<joynr::system::ChannelAddress> address(
                new joynr::system::ChannelAddress(channelAddress)
    );
    routingTable->add(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        QString participantId,
        joynr::system::CommonApiDbusAddress commonApiDbusAddress
) {
    // TODO check if routing table is thread-safe
    QSharedPointer<joynr::system::CommonApiDbusAddress> address(
                new joynr::system::CommonApiDbusAddress(commonApiDbusAddress)
    );
    routingTable->add(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        QString participantId,
        joynr::system::BrowserAddress browserAddress
) {
    // TODO check if routing table is thread-safe
    QSharedPointer<joynr::system::BrowserAddress> address(
                new joynr::system::BrowserAddress(browserAddress)
    );
    routingTable->add(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::addNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        QString participantId,
        joynr::system::WebSocketAddress webSocketAddress
) {
    // TODO check if routing table is thread-safe
    QSharedPointer<joynr::system::WebSocketAddress> address(
                new joynr::system::WebSocketAddress(webSocketAddress)
    );
    routingTable->add(participantId, address);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::removeNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        QString participantId
) {
    // TODO check if routing table is thread-safe
    routingTable->remove(participantId);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
}

// inherited from joynr::system::RoutingProvider
void MessageRouter::resolveNextHop(
        joynr::RequestStatus& joynrInternalStatus,
        bool& resolved,
        QString participantId
) {
    // TODO check if routing table is thread-safe
    resolved = routingTable->contains(participantId);
    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
}


/****
  * IMPLEMENTATION OF THE MESSAGE RUNNABLE
  *
  */
Logger* MessageRunnable::logger = Logging::getInstance()->getLogger("MSG", "MessageRunnable");
int MessageRunnable::messageRunnableCounter = 0;

MessageRunnable::MessageRunnable(const JoynrMessage& message,
                                 const MessagingQos& qos,
                                 QSharedPointer<IMessaging> messagingStub,
                                 DelayedScheduler& delayedScheduler,
                                 const QDateTime& decayTime)
    : ObjectWithDecayTime(decayTime),
      message(message),
      qos(qos),
      messagingStub(messagingStub),
      delayedScheduler(delayedScheduler)
{
    messageRunnableCounter++;
}

MessageRunnable::~MessageRunnable(){
    messageRunnableCounter--;
}

void MessageRunnable::run() {
    if(!isExpired()) {
        messagingStub->transmit(message, qos);
    } else {
        LOG_DEBUG(logger, "Message expired. Dropping the message");
    }
}




} // namespace joynr
