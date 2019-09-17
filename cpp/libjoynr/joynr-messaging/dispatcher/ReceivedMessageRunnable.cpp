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

#include "joynr/CallContext.h"
#include "joynr/CallContextStorage.h"
#include "joynr/Dispatcher.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Message.h"

namespace joynr
{

ReceivedMessageRunnable::ReceivedMessageRunnable(std::shared_ptr<ImmutableMessage> message,
                                                 std::weak_ptr<Dispatcher> dispatcher)
        : Runnable(),
          ObjectWithDecayTime(message->getExpiryDate()),
          _message(std::move(message)),
          _dispatcher(dispatcher)
{
    JOYNR_LOG_TRACE(logger(),
                    "Creating ReceivedMessageRunnable for message: {}",
                    this->_message->toLogMessage());
}

void ReceivedMessageRunnable::shutdown()
{
}

void ReceivedMessageRunnable::run()
{
    if (!_message) {
        return;
    }

    const std::string& messageType = _message->getType();

    JOYNR_LOG_TRACE(logger(),
                    "Running ReceivedMessageRunnable for message type: {}, msg ID: {}",
                    messageType,
                    _message->getId());
    if (isExpired()) {
        JOYNR_LOG_DEBUG(
                logger(), "Dropping ReceivedMessageRunnable message, because it is expired");
        return;
    }

    auto dispatcherSharedPtr = _dispatcher.lock();
    if (!dispatcherSharedPtr) {
        JOYNR_LOG_DEBUG(
                logger(),
                "Dropping ReceivedMessageRunnable message, because dispatcher not available");
        return;
    }

    JOYNR_LOG_TRACE(logger(), "Setting callContext principal to: {}", _message->getCreator());
    CallContext callContext;
    callContext.setPrincipal(_message->getCreator());
    CallContextStorage::set(std::move(callContext));

    if (messageType == Message::VALUE_MESSAGE_TYPE_REQUEST()) {
        dispatcherSharedPtr->handleRequestReceived(std::move(_message));
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_REPLY()) {
        dispatcherSharedPtr->handleReplyReceived(std::move(_message));
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_ONE_WAY()) {
        dispatcherSharedPtr->handleOneWayRequestReceived(std::move(_message));
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST()) {
        dispatcherSharedPtr->handleSubscriptionRequestReceived(std::move(_message));
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST()) {
        dispatcherSharedPtr->handleBroadcastSubscriptionRequestReceived(std::move(_message));
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST()) {
        dispatcherSharedPtr->handleMulticastSubscriptionRequestReceived(std::move(_message));
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY()) {
        dispatcherSharedPtr->handleSubscriptionReplyReceived(std::move(_message));
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_MULTICAST()) {
        dispatcherSharedPtr->handleMulticastReceived(std::move(_message));
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_PUBLICATION()) {
        dispatcherSharedPtr->handlePublicationReceived(std::move(_message));
    } else if (messageType == Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP()) {
        dispatcherSharedPtr->handleSubscriptionStopReceived(std::move(_message));
    } else {
        JOYNR_LOG_ERROR(logger(), "unknown message type: {}", messageType);
    }

    CallContextStorage::invalidate();
    JOYNR_LOG_TRACE(logger(), "Invalidating call context.");
}

} // namespace joynr
