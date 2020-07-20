/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include "UdsLibJoynrMessagingSkeleton.h"

#include <stdexcept>
#include <string>
#include <utility>

#include <smrf/ByteVector.h>
#include <smrf/exceptions.h>

#include "joynr/IMessageRouter.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Message.h"
#include "joynr/Logger.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

UdsLibJoynrMessagingSkeleton::UdsLibJoynrMessagingSkeleton(
        std::weak_ptr<IMessageRouter> messageRouter)
        : IMessagingSkeleton(), _messageRouter(std::move(messageRouter))
{
}

void UdsLibJoynrMessagingSkeleton::transmit(
        std::shared_ptr<ImmutableMessage> message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    if (message->getType() == Message::VALUE_MESSAGE_TYPE_MULTICAST()) {
        message->setReceivedFromGlobal(true);
    }

    try {
        if (auto ptr = _messageRouter.lock()) {
            ptr->route(std::move(message));
        } else {
            std::string errorMessage(
                    "unable to transmit message because messageRouter unavailable: " +
                    message->toLogMessage());
            onFailure(exceptions::JoynrMessageNotSentException(std::move(errorMessage)));
        }
    } catch (const exceptions::JoynrRuntimeException& e) {
        onFailure(e);
    }
}

void UdsLibJoynrMessagingSkeleton::onMessageReceived(smrf::ByteVector&& message,
                                                     const std::string& creator)
{
    // deserialize message and transmit
    std::shared_ptr<ImmutableMessage> immutableMessage;
    try {
        immutableMessage = std::make_shared<ImmutableMessage>(std::move(message));
        immutableMessage->setCreator(creator);
    } catch (const smrf::EncodingException& e) {
        JOYNR_LOG_ERROR(logger(), "Unable to deserialize message - error: {}", e.what());
        return;
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger(), "deserialized message is not valid - error: {}", e.what());
        return;
    }

    if (logger().getLogLevel() == LogLevel::Debug) {
        JOYNR_LOG_DEBUG(logger(), "<<< INCOMING <<< {}", immutableMessage->getTrackingInfo());
    } else {
        JOYNR_LOG_TRACE(logger(), "<<< INCOMING <<< {}", immutableMessage->toLogMessage());
    }

    auto onFailure = [immutableMessage](const exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_ERROR(logger(),
                        "Incoming Message {} could not be sent! reason: {}",
                        immutableMessage->getTrackingInfo(),
                        e.getMessage());
    };
    transmit(std::move(immutableMessage), std::move(onFailure));
}

} // namespace joynr
