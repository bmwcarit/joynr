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
#include "libjoynr/joynr-messaging/dispatcher/ReceivedMessageRunnable.h"

#include <cassert>

#include "joynr/CallContextStorage.h"
#include "joynr/Dispatcher.h"
#include "joynr/Message.h"
#include "joynr/ImmutableMessage.h"

namespace joynr
{

INIT_LOGGER(ReceivedMessageRunnable);

ReceivedMessageRunnable::ReceivedMessageRunnable(std::shared_ptr<ImmutableMessage> message,
                                                 Dispatcher& dispatcher)
        : Runnable(true),
          ObjectWithDecayTime(message->getExpiryDate()),
          message(std::move(message)),
          dispatcher(dispatcher)
{
    JOYNR_LOG_TRACE(logger,
                    "Creating ReceivedMessageRunnable for message: {}",
                    this->message->toLogMessage());
}

void ReceivedMessageRunnable::shutdown()
{
}

void ReceivedMessageRunnable::run()
{
    const std::string& messageType = message->getType();

    JOYNR_LOG_TRACE(logger,
                    "Running ReceivedMessageRunnable for message type: {}, msg ID: {}",
                    messageType,
                    message->getId());
    if (isExpired()) {
        JOYNR_LOG_DEBUG(
                logger, "Dropping ReceivedMessageRunnable message, because it is expired: ");
        return;
    }

    CallContext callContext;
    callContext.setPrincipal(message->getCreator());

    CallContextStorage::set(std::move(callContext));

    if (messageType == Message::VALUE_MESSAGE_TYPE_REQUEST()) {
        dispatcher.handleRequestReceived(*message);
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_REPLY()) {
        dispatcher.handleReplyReceived(*message);
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_ONE_WAY()) {
        dispatcher.handleOneWayRequestReceived(*message);
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST()) {
        dispatcher.handleSubscriptionRequestReceived(*message);
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST()) {
        dispatcher.handleBroadcastSubscriptionRequestReceived(*message);
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST()) {
        dispatcher.handleMulticastSubscriptionRequestReceived(*message);
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY()) {
        dispatcher.handleSubscriptionReplyReceived(*message);
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_MULTICAST()) {
        dispatcher.handleMulticastReceived(*message);
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_PUBLICATION()) {
        dispatcher.handlePublicationReceived(*message);
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP()) {
        dispatcher.handleSubscriptionStopReceived(*message);
    } else {
        JOYNR_LOG_ERROR(logger, "unknown message type: {}", messageType);
    }

    CallContextStorage::invalidate();
}

} // namespace joynr
