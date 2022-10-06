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

#include "joynr/MulticastMessagingSkeletonDirectory.h"

#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

namespace joynr
{

std::shared_ptr<IMessagingMulticastSubscriber> MulticastMessagingSkeletonDirectory::getSkeleton(
        std::shared_ptr<const system::RoutingTypes::Address> address)
{
    const system::RoutingTypes::Address& addressRef = *address;
    std::type_index typeIndex(typeid(addressRef));
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    std::string localGbid = "";
    if (contains(address)) {
        auto mqttAddress = dynamic_cast<const system::RoutingTypes::MqttAddress*>(address.get());
        if (mqttAddress) {
            localGbid = mqttAddress->getBrokerUri();
        }
        JOYNR_LOG_TRACE(logger(),
                        "get messaging skeleton for address {}, gbid: >{}<",
                        address->toString(),
                        localGbid);
        return _multicastSkeletons[std::make_pair(typeIndex, localGbid)];
    }
    JOYNR_LOG_WARN(
            logger(), "No messaging skeleton registered for address {}", address->toString());
    return std::shared_ptr<IMessagingMulticastSubscriber>();
}

bool MulticastMessagingSkeletonDirectory::contains(
        std::shared_ptr<const system::RoutingTypes::Address> address)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const system::RoutingTypes::Address& addressRef = *address;
    std::type_index typeIndex(typeid(addressRef));
    auto mqttAddress = dynamic_cast<const system::RoutingTypes::MqttAddress*>(address.get());
    std::string localGbid = "";
    if (mqttAddress) {
        localGbid = mqttAddress->getBrokerUri();
    }
    return _multicastSkeletons.find(std::make_pair(typeIndex, localGbid)) !=
           _multicastSkeletons.cend();
}

} // namespace joynr
