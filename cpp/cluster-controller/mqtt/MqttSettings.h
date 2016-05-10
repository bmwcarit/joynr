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
#ifndef CLUSTER_CONTROLLER_MQTT_MQTTSETTINGS_H_
#define CLUSTER_CONTROLLER_MQTT_MQTTSETTINGS_H_

#include <chrono>
#include <cstdint>
#include <string>

/**
 * Structure with values for mqtt configuration
 */
struct MqttSettings
{
    std::string host = "localhost";
    uint16_t port = 1883;
    std::chrono::seconds keepAliveTime = std::chrono::seconds(60);
    uint16_t qos = 1;
    std::string prio = "low";
    bool retain = false;
    std::chrono::milliseconds reconnectSleepTimeMs = std::chrono::milliseconds(1000);
};

#endif // CLUSTER_CONTROLLER_MQTT_MQTTSETTINGS_H_
