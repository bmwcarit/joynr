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
#include "UdsMessagingStub.h"

#include <smrf/ByteArrayView.h>

#include "joynr/ImmutableMessage.h"
#include "joynr/IUdsSender.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

UdsMessagingStub::UdsMessagingStub(std::shared_ptr<IUdsSender> udsSender)
        : _udsSender(std::move(udsSender))
{
}

void UdsMessagingStub::transmit(
        std::shared_ptr<ImmutableMessage> message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    if (logger().getLogLevel() == LogLevel::Debug) {
        JOYNR_LOG_DEBUG(logger(), ">>> OUTGOING >>> {}", message->getTrackingInfo());
    } else {
        JOYNR_LOG_TRACE(logger(), ">>> OUTGOING >>> {}", message->toLogMessage());
    }
    const smrf::ByteArrayView serializedMessageView(message->getSerializedMessage());
    _udsSender->send(std::move(serializedMessageView), std::move(onFailure));
}

} // namespace joynr
