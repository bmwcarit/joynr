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
#include "libjoynr/joynr-messaging/dispatcher/ReceivedMessageRunnable.h"
#include "joynr/Dispatcher.h"

namespace joynr
{

using namespace joynr_logging;

Logger* ReceivedMessageRunnable::logger =
        Logging::getInstance()->getLogger("MSG", "ReceivedMessageRunnable");

ReceivedMessageRunnable::ReceivedMessageRunnable(const JoynrMessage& message,
                                                 Dispatcher& dispatcher)
        : ObjectWithDecayTime(message.getHeaderExpiryDate()),
          message(message),
          dispatcher(dispatcher)
{
    LOG_DEBUG(logger, "Creating ReceivedMessageRunnable for message type: " + message.getType());
}

void ReceivedMessageRunnable::run()
{
    LOG_DEBUG(logger,
              QString("Running ReceivedMessageRunnable for message type: %1, msg ID: %2 and "
                      "payload: %3")
                      .arg(message.getType())
                      .arg(message.getHeaderMessageId())
                      .arg(QString(message.getPayload())));
    if (isExpired()) {
        LOG_DEBUG(logger, "Dropping ReceivedMessageRunnable message, because it is expired: ");
        return;
    }

    if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST) {
        dispatcher.handleRequestReceived(message);
    } else if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REPLY) {
        dispatcher.handleReplyReceived(message);
    } else if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST) {
        dispatcher.handleSubscriptionRequestReceived(message);
    } else if (message.getType() ==
               JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST) {
        dispatcher.handleBroadcastSubscriptionRequestReceived(message);
    } else if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY) {
        LOG_FATAL(logger, "subscription reply not yet implemented");
        assert(false);
    } else if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_PUBLICATION) {
        dispatcher.handlePublicationReceived(message);
    } else if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP) {
        dispatcher.handleSubscriptionStopReceived(message);
    } else {
        LOG_FATAL(logger, "unknown message type: " + message.getType());
        assert(false);
    }
}

} // namespace joynr
