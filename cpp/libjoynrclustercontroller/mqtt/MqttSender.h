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
#ifndef MQTTSENDER_H
#define MQTTSENDER_H

#include "joynr/PrivateCopyAssign.h"

#include "joynr/IMessageSender.h"
#include "joynr/Logger.h"

namespace joynr
{

class JoynrMessage;
class MessagingSettings;
class MosquittoConnection;

class MqttSender : public IMessageSender
{

public:
    explicit MqttSender(std::shared_ptr<MosquittoConnection> mosquittoConnection);

    ~MqttSender() override = default;

    /**
    * @brief Sends the message to the given channel.
    */
    void sendMessage(const system::RoutingTypes::Address& destinationAddress,
                     const JoynrMessage& message,
                     const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
            override;

    void registerReceiver(std::shared_ptr<ITransportMessageReceiver> receiver) override;

private:
    DISALLOW_COPY_AND_ASSIGN(MqttSender);

    std::shared_ptr<MosquittoConnection> mosquittoConnection;
    std::shared_ptr<ITransportMessageReceiver> receiver;

    ADD_LOGGER(MqttSender);
};

} // namespace joynr

#endif // MQTTSENDER_H
