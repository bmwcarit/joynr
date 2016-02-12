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
#include <cstring>

#include "MosquittoSubscriber.h"

#include "joynr/JoynrMessage.h"
#include "joynr/JsonSerializer.h"
#include "joynr/MessageRouter.h"

namespace joynr
{

INIT_LOGGER(MosquittoSubscriber);

MosquittoSubscriber::MosquittoSubscriber(const BrokerUrl& brokerUrl,
                                         const std::string& channelId,
                                         joynr::Semaphore* channelCreatedSemaphore)
        : joynr::Thread("MosquittoSubscriber"),
          MosquittoConnection(brokerUrl),
          mqttSettings(),
          brokerUrl(brokerUrl),
          channelId(channelId),
          channelCreatedSemaphore(channelCreatedSemaphore),
          isRunning(false),
          isChannelAvailable(false),
          onTextMessageReceived(nullptr)
{
}

void MosquittoSubscriber::interrupt()
{
    isRunning = false;
}

bool MosquittoSubscriber::isInterrupted()
{
    return !isRunning;
}

void MosquittoSubscriber::stop()
{
    interrupt();
    joynr::Thread::stop();
}

void MosquittoSubscriber::run()
{
    checkServerTime();

    isRunning = true;

    while (!isInterrupted()) {
        int rc = loop();

        if (rc) {
            std::this_thread::sleep_for(mqttSettings.reconnectSleepTimeMs);
            reconnect();
        }
    }

    JOYNR_LOG_DEBUG(logger, "Try to disconnect Mosquitto Connection");
    disconnect();
    JOYNR_LOG_DEBUG(logger, "Mosquitto Connection disconnected");

    mosqpp::lib_cleanup();
}

void MosquittoSubscriber::registerChannelId(const std::string& channelId)
{
    this->channelId = channelId;
    isChannelAvailable = true;
}

void MosquittoSubscriber::registerReceiveCallback(
        std::function<void(const std::string&)> onTextMessageReceived)
{
    this->onTextMessageReceived = onTextMessageReceived;
}

void MosquittoSubscriber::on_connect(int rc)
{
    if (rc > 0) {
        JOYNR_LOG_DEBUG(logger, "Mosquitto Connection Error {}", rc);
    } else {
        JOYNR_LOG_DEBUG(logger, "Mosquitto Connection established");

        std::string topic = channelId + "/" + getMqttPrio() + "/" + "#";

        while (!isChannelAvailable) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }

        // TODO: Check mid in callback on_subscribe instead of generated mid (NULL)
        subscribe(nullptr, topic.c_str(), getMqttQos());
        JOYNR_LOG_DEBUG(logger, "Subscribed to {}", topic);
    }
}

void MosquittoSubscriber::on_subscribe(int mid, int qos_count, const int* granted_qos)
{
    JOYNR_LOG_DEBUG(logger, "Subscribed (mid: {} with granted QOS {}", mid, granted_qos[0]);

    for (int i = 1; i < qos_count; i++) {
        JOYNR_LOG_DEBUG(logger, "QOS: {} granted {}", i, granted_qos[i]);
    }

    channelCreatedSemaphore->notify();
}

void MosquittoSubscriber::on_message(const struct mosquitto_message* message)
{
    JOYNR_LOG_DEBUG(logger, "Received raw message len: {}", message->payloadlen);

    std::string jsonObject(static_cast<char*>(message->payload), message->payloadlen);

    JOYNR_LOG_DEBUG(logger, "Received raw message: {}", jsonObject);

    if (onTextMessageReceived) {
        onTextMessageReceived(jsonObject);
    } else {
        JOYNR_LOG_ERROR(
                logger,
                "Discarding received message, since onTextMessageReceived callback is empty.");
    }
}

void MosquittoSubscriber::checkServerTime()
{
}

} // namespace joynr
