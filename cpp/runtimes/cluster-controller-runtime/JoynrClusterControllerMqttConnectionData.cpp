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

#include "joynr/JoynrClusterControllerMqttConnectionData.h"

namespace joynr
{

JoynrClusterControllerMqttConnectionData::JoynrClusterControllerMqttConnectionData()
        : mosquittoConnection(nullptr), mqttMessageReceiver(nullptr), mqttMessageSender(nullptr)
{
}

JoynrClusterControllerMqttConnectionData::~JoynrClusterControllerMqttConnectionData() = default;

std::shared_ptr<MosquittoConnection> JoynrClusterControllerMqttConnectionData::
        getMosquittoConnection() const
{
    return mosquittoConnection;
}

void JoynrClusterControllerMqttConnectionData::setMosquittoConnection(
        const std::shared_ptr<MosquittoConnection>& value)
{
    mosquittoConnection = value;
}

std::shared_ptr<ITransportMessageReceiver> JoynrClusterControllerMqttConnectionData::
        getMqttMessageReceiver() const
{
    return mqttMessageReceiver;
}

void JoynrClusterControllerMqttConnectionData::setMqttMessageReceiver(
        const std::shared_ptr<ITransportMessageReceiver>& value)
{
    mqttMessageReceiver = value;
}

std::shared_ptr<ITransportMessageSender> JoynrClusterControllerMqttConnectionData::
        getMqttMessageSender() const
{
    return mqttMessageSender;
}

void JoynrClusterControllerMqttConnectionData::setMqttMessageSender(
        const std::shared_ptr<ITransportMessageSender>& value)
{
    mqttMessageSender = value;
}
}
