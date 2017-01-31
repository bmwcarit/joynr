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

#include "MosquittoConnection.h"
#include "MqttSettings.h"
#include "joynr/MessagingSettings.h"

namespace joynr
{

INIT_LOGGER(MosquittoConnection);

MosquittoConnection::MosquittoConnection(const MessagingSettings& settings)
        : mqttSettings(), brokerUrl(settings.getBrokerUrl())
{
    const Url url = brokerUrl.getBrokerChannelsBaseUrl();

    std::string host = url.getHost();
    uint16_t port = url.getPort();

    mqttSettings.host = std::string(host);
    mqttSettings.port = port;
    mqttSettings.keepAliveTime = settings.getMqttKeepAliveTime();

    JOYNR_LOG_DEBUG(logger, "Try to connect to tcp://{}:{}", mqttSettings.host, mqttSettings.port);

    connect(host.c_str(), port, mqttSettings.keepAliveTime.count());
}

void MosquittoConnection::on_disconnect(int rc)
{
    if (rc == 0) {
        JOYNR_LOG_DEBUG(
                logger, "Disconnected from tcp://{}:{}", mqttSettings.host, mqttSettings.port);
    } else {
        JOYNR_LOG_ERROR(logger,
                        "Unexpectedly disconnected from tcp://{}:{}, error: {}",
                        mqttSettings.host,
                        mqttSettings.port,
                        rc);
    }
}

void MosquittoConnection::on_log(int level, const char* str)
{
    if (level == MOSQ_LOG_ERR) {
        JOYNR_LOG_ERROR(logger, "Mosquitto Log: {}", str);
    } else if (level == MOSQ_LOG_WARNING) {
        JOYNR_LOG_WARN(logger, "Mosquitto Log: {}", str);
    } else if (level == MOSQ_LOG_INFO) {
        JOYNR_LOG_INFO(logger, "Mosquitto Log: {}", str);
    } else {
        // MOSQ_LOG_NOTICE || MOSQ_LOG_DEBUG || any other log level
        JOYNR_LOG_DEBUG(logger, "Mosquitto Log: {}", str);
    }
}

void MosquittoConnection::on_error()
{
    JOYNR_LOG_WARN(logger, "Mosquitto Error");
}

uint16_t MosquittoConnection::getMqttQos() const
{
    return mqttSettings.qos;
}

std::string MosquittoConnection::getMqttPrio() const
{
    return mqttSettings.prio;
}

bool MosquittoConnection::isMqttRetain() const
{
    return mqttSettings.retain;
}

} // namespace joynr
