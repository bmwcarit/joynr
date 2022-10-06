/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
#include "joynr/AbstractGlobalMessagingSkeleton.h"

#include <stdexcept>

#include "joynr/IMessageRouter.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Message.h"
#include "joynr/TimePoint.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

namespace joynr
{

void AbstractGlobalMessagingSkeleton::registerGlobalRoutingEntryIfRequired(
        const ImmutableMessage& message,
        std::shared_ptr<IMessageRouter> messageRouter,
        const std::string& gbid)
{
    const std::string& messageType = message.getType();

    if (messageType == Message::VALUE_MESSAGE_TYPE_REQUEST() ||
        messageType == Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST() ||
        messageType == Message::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST() ||
        messageType == Message::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST()) {

        boost::optional<std::string> optionalReplyTo = message.getReplyTo();

        if (!optionalReplyTo) {
            std::string errorMessage("message " + message.getTrackingInfo() +
                                     " did not contain replyTo header, discarding");
            JOYNR_LOG_ERROR(logger(), errorMessage);
            throw exceptions::JoynrMessageNotSentException(errorMessage);
        }
        const std::string& replyTo = *optionalReplyTo;
        try {
            // because the message is received via global transport (isGloballyVisible=true),
            // isGloballyVisible must be true
            const bool isGloballyVisible = true;
            const TimePoint expiryDate = TimePoint::max();

            const bool isSticky = false;
            using system::RoutingTypes::Address;
            std::shared_ptr<const Address> address;
            joynr::serializer::deserializeFromJson(address, replyTo);
            if (auto mqttAddress = dynamic_cast<const joynr::system::RoutingTypes::MqttAddress*>(
                        address.get())) {
                address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
                        gbid, mqttAddress->getTopic());
            }
            messageRouter->addNextHop(message.getSender(),
                                      address,
                                      isGloballyVisible,
                                      expiryDate.toMilliseconds(),
                                      isSticky);
        } catch (const std::invalid_argument& e) {
            std::string errorMessage("could not deserialize Address from " + replyTo +
                                     " - error: " + e.what());
            JOYNR_LOG_FATAL(logger(), errorMessage);
            // do not try to route the message if address is not valid
            throw exceptions::JoynrMessageNotSentException(errorMessage);
        }
    }
}
} // namespace joynr
