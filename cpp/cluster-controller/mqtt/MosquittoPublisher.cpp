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

#include "joynr/MessagingSettings.h"

namespace joynr
{

INIT_LOGGER(MosquittoPublisher);

MosquittoPublisher::MosquittoPublisher(const MessagingSettings& settings)
        : joynr::Thread("MosquittoPublisher"),
          MosquittoConnection(settings),
          mqttSettings(),
          isRunning(true)
{
    mqttSettings.reconnectSleepTimeMs = settings.getMqttReconnectSleepTime();
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

std::string MosquittoPublisher::getMqttPrio() const
{
    return joynr::MosquittoConnection::getMqttPrio();
}

uint16_t MosquittoPublisher::getMqttQos() const
{
    return joynr::MosquittoConnection::getMqttQos();
}

void MosquittoPublisher::run()
{
    while (!isInterrupted()) {
        int rc = loop();

        if (rc != MOSQ_ERR_SUCCESS) {
            if (rc == MOSQ_ERR_CONN_LOST) {
                JOYNR_LOG_DEBUG(logger,
                                "error: connection to broker lost ({}), trying to reconnect...",
                                mosqpp::strerror(rc));
            } else if (rc == MOSQ_ERR_NO_CONN) {
                JOYNR_LOG_DEBUG(logger,
                                "error: not connected to a broker ({}), trying to reconnect...",
                                mosqpp::strerror(rc));
            } else {
                // MOSQ_ERR_INVAL || MOSQ_ERR_NOMEM || MOSQ_ERR_PROTOCOL || MOSQ_ERR_ERRNO
                JOYNR_LOG_ERROR(logger,
                                "connection to broker lost, unexpected error: {} ({}), trying to "
                                "reconnect...",
                                std::to_string(rc),
                                mosqpp::strerror(rc));
            }
            std::this_thread::sleep_for(mqttSettings.reconnectSleepTimeMs);
            reconnect();
        }
    }

    JOYNR_LOG_TRACE(logger, "Try to disconnect Mosquitto Connection");
    int rc = disconnect();
    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_DEBUG(logger, "Mosquitto Connection disconnected");
    } else {
        JOYNR_LOG_ERROR(logger,
                        "Mosquitto disconnect failed: error: {} ({})",
                        std::to_string(rc),
                        mosqpp::strerror(rc));
    }

    mosqpp::lib_cleanup();
}

void MosquittoPublisher::publishMessage(
        const std::string& topic,
        const int qosLevel,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure,
        uint32_t payloadlen = 0,
        const void* payload = nullptr)
{
    JOYNR_LOG_DEBUG(logger, "Publish to {}", topic);

    int mid;
    int rc = publish(&mid, topic.c_str(), payloadlen, payload, qosLevel, isMqttRetain());
    if (!(rc == MOSQ_ERR_SUCCESS)) {
        if (rc == MOSQ_ERR_INVAL || rc == MOSQ_ERR_PAYLOAD_SIZE) {
            onFailure(exceptions::JoynrMessageNotSentException(
                    "message could not be sent: mid (mqtt message id): " + std::to_string(mid) +
                    ", error: " + std::to_string(rc) + " (" + mosqpp::strerror(rc) + ")"));
        }
        // MOSQ_ERR_NOMEM || MOSQ_ERR_NO_CONN || MOSQ_ERR_PROTOCOL ||| unexpected errors
        onFailure(exceptions::JoynrDelayMessageException(
                "error sending message: mid (mqtt message id): " + std::to_string(mid) +
                ", error: " + std::to_string(rc) + " (" + mosqpp::strerror(rc) + ")"));
    }
    JOYNR_LOG_TRACE(logger, "published message with mqtt message id {}", std::to_string(mid));
}

void MosquittoPublisher::on_connect(int rc)
{
    if (rc > 0) {
        if (rc == 1) {
            JOYNR_LOG_ERROR(logger,
                            "Mosquitto Connection Error {} ({})",
                            rc,
                            "connection refused (unacceptable protocol version)");
        } else if (rc == 2) {
            JOYNR_LOG_ERROR(logger,
                            "Mosquitto Connection Error {} ({})",
                            rc,
                            "connection refused (identifier rejected)");
        } else if (rc == 3) {
            JOYNR_LOG_DEBUG(logger,
                            "Mosquitto Connection Error {} ({})",
                            rc,
                            "connection refused (broker unavailable)");
        } else {
            JOYNR_LOG_ERROR(logger,
                            "Mosquitto Connection Error {} ({})",
                            rc,
                            "unknown error code (reserved for future use)");
        }
    } else {
        JOYNR_LOG_DEBUG(logger, "Mosquitto Connection established");
    }
}

void MosquittoPublisher::on_publish(int mid)
{
    JOYNR_LOG_TRACE(logger, "published message with mid {}", std::to_string(mid));
}

} // namespace joynr
