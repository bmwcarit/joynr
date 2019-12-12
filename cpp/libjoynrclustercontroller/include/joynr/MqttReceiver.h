/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#ifndef MQTTRECEIVER_H
#define MQTTRECEIVER_H

#include <functional>
#include <memory>
#include <string>

#include "joynr/PrivateCopyAssign.h"

#include "joynr/ITransportMessageReceiver.h"
#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/Logger.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

namespace joynr
{

class MosquittoConnection;
class MessagingSettings;

class JOYNRCLUSTERCONTROLLER_EXPORT MqttReceiver : public ITransportMessageReceiver
{
public:
    explicit MqttReceiver(std::shared_ptr<MosquittoConnection> mosquittoConnection,
                          const MessagingSettings& /*settings*/,
                          const std::string& channelIdForMqttTopic,
                          const std::string& gbid,
                          const std::string& unicastTopicPrefix);

    ~MqttReceiver() override = default;

    /**
      * Gets the serialized address of the receive channel for incoming messages.
      */
    const std::string getSerializedGlobalClusterControllerAddress() const override;

    /**
      * Gets the address of the receive channel for incoming messages.
      */
    const system::RoutingTypes::MqttAddress& getGlobalClusterControllerAddress() const override;

    /**
      * Checks the MessageSettings and updates the configuration.
      * Can be called at any time to read settings.
      */
    void updateSettings() override;

    /**
      * Deletes the channel on the broker. Will only try once
      */
    bool tryToDeleteChannel() override;

    bool isConnected() override;

    void startReceiveQueue() override;

    /**
      * stops the receiveQue. This might ungracefully terminate the thread of the
     * LongPollingMessageReceiver.
      */
    void stopReceiveQueue() override;

    void registerReceiveCallback(
            std::function<void(smrf::ByteVector&&)> onMessageReceived) override;

    virtual void subscribeToTopic(const std::string& topic);

    virtual void unsubscribeFromTopic(const std::string& topic);

private:
    DISALLOW_COPY_AND_ASSIGN(MqttReceiver);

    system::RoutingTypes::MqttAddress _globalClusterControllerAddress;

    std::shared_ptr<MosquittoConnection> _mosquittoConnection;

    ADD_LOGGER(MqttReceiver)
};

} // namespace joynr

#endif // MQTTRECEIVER_H
