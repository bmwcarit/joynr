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
#include "MqttTransportStatus.h"

#include <joynr/system/RoutingTypes/MqttAddress.h>

#include "MosquittoConnection.h"

namespace joynr
{

MqttTransportStatus::MqttTransportStatus(std::shared_ptr<MosquittoConnection> mosquittoConnection,
                                         const std::string& gbid)
        : _mosquittoConnection(std::move(mosquittoConnection)),
          _gbid(gbid),
          _availabilityChangedCallback()
{
    this->_mosquittoConnection->registerReadyToSendChangedCallback([this](bool readyToSend) {
        if (_availabilityChangedCallback) {
            _availabilityChangedCallback(readyToSend);
        }
    });
}

MqttTransportStatus::~MqttTransportStatus()
{
    _mosquittoConnection->registerReadyToSendChangedCallback(nullptr);
}

bool MqttTransportStatus::isReponsibleFor(
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address)
{
    auto mqttAddress = dynamic_cast<const joynr::system::RoutingTypes::MqttAddress*>(address.get());
    return (mqttAddress && (mqttAddress->getBrokerUri() == _gbid));
}

bool MqttTransportStatus::isAvailable()
{
    return _mosquittoConnection->isReadyToSend();
}

void MqttTransportStatus::setAvailabilityChangedCallback(
        std::function<void(bool)> availabilityChangedCallback)
{
    this->_availabilityChangedCallback = std::move(availabilityChangedCallback);
}

} // namespace joynr
