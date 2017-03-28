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
#ifndef IMESSAGESENDER_H
#define IMESSAGESENDER_H

#include <string>
#include <memory>

#include "joynr/MessagingSettings.h"
#include "joynr/JoynrMessage.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

class IMessageReceiver;

class IMessageSender
{
public:
    virtual ~IMessageSender() = default;
    /**
    * @brief Sends the message to the given channel.
    */
    virtual void sendMessage(
            const joynr::system::RoutingTypes::Address& destinationAddress,
            const JoynrMessage& message,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) = 0;

    virtual void registerReceiver(std::shared_ptr<IMessageReceiver>) = 0;
};
} // namespace joynr

#endif // IMESSAGESENDER_H
