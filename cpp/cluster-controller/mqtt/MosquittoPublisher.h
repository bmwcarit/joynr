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
#ifndef CLUSTER_CONTROLLER_MQTT_MOSQUITTOPUBLISHER_H_
#define CLUSTER_CONTROLLER_MQTT_MOSQUITTOPUBLISHER_H_

#include <atomic>

#include "mosquittopp.h"

#include "MosquittoConnection.h"
#include "MqttSettings.h"

#include "joynr/exceptions/JoynrException.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Thread.h"

namespace joynr
{

class MessagingSettings;

class MosquittoPublisher : public Thread, private MosquittoConnection
{
public:
    explicit MosquittoPublisher(const MessagingSettings& settings);

    ~MosquittoPublisher() override = default;

    void stop() override;
    void run() override;
    void interrupt();
    bool isInterrupted();
    uint16_t getMqttQos() const override;
    std::string getMqttPrio() const;

    void publishMessage(
            const std::string& topic,
            const int qosLevel,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure,
            uint32_t payloadlen,
            const void* payload);

private:
    DISALLOW_COPY_AND_ASSIGN(MosquittoPublisher);

    MqttSettings mqttSettings;

    std::atomic<bool> isRunning;

    ADD_LOGGER(MosquittoPublisher);

    void on_connect(int rc) override;
    void on_publish(int mid) override;
};

} // namespace joynr

#endif // CLUSTER_CONTROLLER_MQTT_MOSQUITTOPUBLISHER_H_
