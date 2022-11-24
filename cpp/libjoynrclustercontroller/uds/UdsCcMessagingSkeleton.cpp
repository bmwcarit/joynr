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

#include "UdsCcMessagingSkeleton.h"

#include <memory>
#include <stdexcept>
#include <utility>

#include <smrf/ByteVector.h>
#include <smrf/exceptions.h>

#include "joynr/IMessageRouter.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Logger.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{
UdsCcMessagingSkeleton::UdsCcMessagingSkeleton(std::shared_ptr<IMessageRouter> messageRouter)
        : std::enable_shared_from_this<UdsCcMessagingSkeleton>(),
          _messageRouter(std::move(messageRouter))
{
}

void UdsCcMessagingSkeleton::transmit(
        std::shared_ptr<ImmutableMessage> message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    try {
        _messageRouter->route(std::move(message));
    } catch (const exceptions::JoynrMessageExpiredException& e) {
        JOYNR_LOG_WARN(logger(), "Could not transmit incoming message. Error: {}", e.getMessage());
    } catch (exceptions::JoynrRuntimeException& e) {
        onFailure(e);
    }
}

void UdsCcMessagingSkeleton::onMessageReceived(smrf::ByteVector&& message,
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
