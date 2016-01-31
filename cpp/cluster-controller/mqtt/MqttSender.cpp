/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "MqttSender.h"

#include "joynr/JsonSerializer.h"
#include "joynr/Util.h"

namespace joynr
{

INIT_LOGGER(MqttSender);

MqttSender::MqttSender(const BrokerUrl& brokerUrl)
        : mosquittoPublisher(brokerUrl), brokerUrl(brokerUrl), waitForReceiveQueueStarted(nullptr)
{
    mosquittoPublisher.start();
}

MqttSender::~MqttSender()
{
    mosquittoPublisher.stop();
}

void MqttSender::init(std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory,
                      const MessagingSettings& settings)
{
    std::ignore = channelUrlDirectory;
    std::ignore = settings;
}

void MqttSender::sendMessage(const std::string& channelId, const JoynrMessage& message)
{
    JOYNR_LOG_DEBUG(logger, "sendMessage: ...");

    waitForReceiveQueueStarted();

    std::string serializedMessage = JsonSerializer::serialize(message);

    const int payloadLength = serializedMessage.length();
    const void* payload = serializedMessage.c_str();

    Util::logSerializedMessage(logger, "Sending Message: ", serializedMessage);

    mosquittoPublisher.publishMessage(channelId, message.getHeaderTo(), payloadLength, payload);
}

void MqttSender::registerReceiveQueueStartedCallback(
        std::function<void(void)> waitForReceiveQueueStarted)
{
    this->waitForReceiveQueueStarted = waitForReceiveQueueStarted;
}

} // namespace joynr
