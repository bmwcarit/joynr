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
#ifndef CLUSTER_CONTROLLER_MQTT_MOSQUITTOCONNECTION_H_
#define CLUSTER_CONTROLLER_MQTT_MOSQUITTOCONNECTION_H_

#include "mosquittopp.h"

#include "MqttSettings.h"

#include "joynr/BrokerUrl.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class MessagingSettings;

class MosquittoConnection : public mosqpp::mosquittopp
{

public:
    explicit MosquittoConnection(const MessagingSettings& brokerUrl);

    ~MosquittoConnection() override = default;

    uint16_t getMqttQos() const;
    std::string getMqttPrio() const;
    bool isMqttRetain() const;

private:
    DISALLOW_COPY_AND_ASSIGN(MosquittoConnection);

    MqttSettings mqttSettings;

    const BrokerUrl brokerUrl;

    ADD_LOGGER(MosquittoConnection);

    void on_disconnect(int rc) override;
    void on_log(int level, const char* str) override;
    void on_error() override;
};

} // namespace joynr

#endif // CLUSTER_CONTROLLER_MQTT_MOSQUITTOCONNECTION_H_
