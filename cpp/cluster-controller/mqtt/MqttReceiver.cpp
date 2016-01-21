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
#include "MqttReceiver.h"

#include "cluster-controller/messaging/MessagingPropertiesPersistence.h"

namespace joynr
{

INIT_LOGGER(MqttReceiver);

MqttReceiver::MqttReceiver(const MessagingSettings& settings)
        : channelCreatedSemaphore(new joynr::Semaphore(0)),
          channelId(),
          receiverId(),
          settings(settings),
          channelUrlDirectory(),
          mosquittoSubscriber(settings.getBrokerUrl(), channelId, channelCreatedSemaphore),
          mqttSettings()
{
    MessagingPropertiesPersistence persist(settings.getMessagingPropertiesPersistenceFilename());

    channelId = mqttSettings.mqttChannelIdPrefix + persist.getChannelId();
    receiverId = persist.getReceiverId();

    init();
}

MqttReceiver::~MqttReceiver()
{
    mosquittoSubscriber.stop();
}

void MqttReceiver::init()
{
    mosquittoSubscriber.registerChannelId(channelId);
}

void MqttReceiver::init(std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory)
{
    (void)channelUrlDirectory;
}

void MqttReceiver::updateSettings()
{
}

void MqttReceiver::startReceiveQueue()
{
    JOYNR_LOG_DEBUG(logger, "startReceiveQueue");

    mosquittoSubscriber.start();
}

void MqttReceiver::waitForReceiveQueueStarted()
{
    JOYNR_LOG_TRACE(logger, "waiting for ReceiveQueue to be started.");
    channelCreatedSemaphore->wait();
}

void MqttReceiver::stopReceiveQueue()
{
    JOYNR_LOG_DEBUG(logger, "stopReceiveQueue");

    mosquittoSubscriber.stop();
}

const std::string& MqttReceiver::getReceiveChannelId() const
{
    return channelId;
}

bool MqttReceiver::tryToDeleteChannel()
{
    return true;
}

void MqttReceiver::registerReceiveCallback(
        std::function<void(const std::string&)> onTextMessageReceived)
{
    mosquittoSubscriber.registerReceiveCallback(onTextMessageReceived);
}

} // namespace joynr
