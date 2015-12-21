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
#include <tuple>

#include "MosquittoPublisher.h"

namespace joynr
{

INIT_LOGGER(MosquittoPublisher);

MosquittoPublisher::MosquittoPublisher(const BrokerUrl& brokerUrl)
        : joynr::Thread("MosquittoPublisher"),
          MosquittoConnection(brokerUrl),
          mqttSettings(),
          brokerUrl(brokerUrl),
          isRunning(true)
{
}

void MosquittoPublisher::interrupt()
{
    isRunning = false;
}

bool MosquittoPublisher::isInterrupted()
{
    return !isRunning;
}

void MosquittoPublisher::stop()
{
    interrupt();
    joynr::Thread::stop();
}

void MosquittoPublisher::run()
{
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

int MosquittoPublisher::publishMessage(const std::string& channelId,
                                       const std::string& participantId,
                                       uint32_t payloadlen = 0,
                                       const void* payload = nullptr)
{
    std::string topic = channelId + "/" + getMqttPrio() + "/" + participantId;

    // std::string* data = static_cast<std::string*>(const_cast<void *>(payload));

    JOYNR_LOG_DEBUG(logger, "Publish to {}", topic);

    return publish(nullptr, topic.c_str(), payloadlen, payload, getMqttQos(), isMqttRetain());
}

void MosquittoPublisher::on_connect(int rc)
{
    if (rc > 0) {
        JOYNR_LOG_DEBUG(logger, "Mosquitto Connection Error {}", rc);
    } else {
        JOYNR_LOG_DEBUG(logger, "Mosquitto Connection established");
    }
}

void MosquittoPublisher::on_publish(int rc)
{
    // TODO: check rc
    std::ignore = rc;
}

} // namespace joynr
