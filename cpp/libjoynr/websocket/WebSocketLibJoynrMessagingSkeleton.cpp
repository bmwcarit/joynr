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
#include "WebSocketLibJoynrMessagingSkeleton.h"

#include <stdexcept>
#include <string>
#include <utility>

#include <smrf/ByteVector.h>
#include <smrf/exceptions.h>

#include "joynr/IMessageRouter.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Message.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

WebSocketLibJoynrMessagingSkeleton::WebSocketLibJoynrMessagingSkeleton(
        std::weak_ptr<IMessageRouter> messageRouter)
        : _messageRouter(std::move(messageRouter))
{
}

void WebSocketLibJoynrMessagingSkeleton::transmit(
        std::shared_ptr<ImmutableMessage> message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    try {
        if (auto ptr = _messageRouter.lock()) {
            ptr->route(message);
        }
    } catch (const exceptions::JoynrRuntimeException& e) {
        onFailure(e);
    }
}

void WebSocketLibJoynrMessagingSkeleton::onMessageReceived(smrf::ByteVector&& message)
{
    // deserialize message and transmit
    std::shared_ptr<ImmutableMessage> immutableMessage;
    try {
        immutableMessage = std::make_shared<ImmutableMessage>(std::move(message));
    } catch (const smrf::EncodingException& e) {
        JOYNR_LOG_ERROR(logger(), "Unable to deserialize message - error: {}", e.what());
        return;
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger(), "deserialized message is not valid - error: {}", e.what());
        return;
    }

    if (immutableMessage->getType() == Message::VALUE_MESSAGE_TYPE_MULTICAST()) {
        immutableMessage->setReceivedFromGlobal(true);
    }

    if (logger().getLogLevel() == LogLevel::Debug) {
        JOYNR_LOG_DEBUG(logger(), "<<< INCOMING <<< {}", immutableMessage->getTrackingInfo());
    } else {
        JOYNR_LOG_TRACE(logger(), "<<< INCOMING <<< {}", immutableMessage->toLogMessage());
    }

    auto onFailure = [messageId = immutableMessage->getId()](
            const exceptions::JoynrRuntimeException& e)
    {
        JOYNR_LOG_ERROR(logger(),
                        "Incoming Message with ID {} could not be sent! reason: {}",
                        messageId,
                        e.getMessage());
    };
    transmit(std::move(immutableMessage), onFailure);
}

} // namespace joynr
