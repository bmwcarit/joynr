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

#ifndef ROUTINGTABLE_H
#define ROUTINGTABLE_H

#include <string>
#include <memory>
#include <unordered_map>

#include <boost/type_index.hpp>

#include "joynr/Directory.h"
#include "joynr/JsonSerializer.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/InProcessAddress.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

namespace joynr
{

class RoutingTable : public Directory<std::string, const joynr::system::RoutingTypes::Address>
{
public:
    using Directory::Directory;
    ~RoutingTable() override = default;

    std::string serializeToJson() const;
    void deserializeFromJson(const std::string& jsonString);

private:
    DISALLOW_COPY_AND_ASSIGN(RoutingTable);

    /*
     * Bypass polymorphism issue of RoutingTypes::Address
     *
     * The current json serializer does not handle class polymorphism. It is not possible to
     * automatically serialize a map whose
     * values are child classes (e.g. WebSocketAddress) of the class used for the map (Address).
     *
     * The current solution, converts the inputMap to a map of variants which is then serialized.
     * keys are mantained.
     *
     * Warning: convertDictionaryToVariantMap relies on the inner implementation of Dictionary class
     * (i.e. it knows a map container is being used)
     *
     */
    template <typename InputMap>
    std::map<std::string, Variant> convertDictionaryToVariantMap(const InputMap& inputMap) const
    {
        std::map<std::string, Variant> variantMap;
        for (const auto& element : inputMap) {
            if (boost::typeindex::type_id_runtime(*element.second) ==
                boost::typeindex::type_id<joynr::system::RoutingTypes::WebSocketAddress>()) {
                variantMap.emplace(
                        element.first,
                        Variant::make<joynr::system::RoutingTypes::WebSocketAddress>(
                                static_cast<const joynr::system::RoutingTypes::WebSocketAddress&>(
                                        *(element.second))));
            } else if (boost::typeindex::type_id_runtime(*element.second) ==
                       boost::typeindex::type_id<joynr::system::RoutingTypes::ChannelAddress>()) {
                variantMap.emplace(
                        element.first,
                        Variant::make<joynr::system::RoutingTypes::ChannelAddress>(
                                static_cast<const joynr::system::RoutingTypes::ChannelAddress&>(
                                        *(element.second))));
            } else if (boost::typeindex::type_id_runtime(*element.second) ==
                       boost::typeindex::type_id<joynr::system::RoutingTypes::MqttAddress>()) {
                variantMap.emplace(
                        element.first,
                        Variant::make<joynr::system::RoutingTypes::MqttAddress>(
                                static_cast<const joynr::system::RoutingTypes::MqttAddress&>(
                                        *(element.second))));
            } else if (boost::typeindex::type_id_runtime(*element.second) ==
                       boost::typeindex::type_id<
                               joynr::system::RoutingTypes::CommonApiDbusAddress>()) {
                variantMap.emplace(
                        element.first,
                        Variant::make<joynr::system::RoutingTypes::CommonApiDbusAddress>(
                                static_cast<
                                        const joynr::system::RoutingTypes::CommonApiDbusAddress&>(
                                        *(element.second))));
            } else if (boost::typeindex::type_id_runtime(*element.second) ==
                       boost::typeindex::type_id<joynr::system::RoutingTypes::BrowserAddress>()) {
                variantMap.emplace(
                        element.first,
                        Variant::make<joynr::system::RoutingTypes::BrowserAddress>(
                                static_cast<const joynr::system::RoutingTypes::BrowserAddress&>(
                                        *(element.second))));
            } else if (boost::typeindex::type_id_runtime(*element.second) ==
                       boost::typeindex::type_id<
                               joynr::system::RoutingTypes::WebSocketClientAddress>()) {
                variantMap.emplace(
                        element.first,
                        Variant::make<joynr::system::RoutingTypes::WebSocketClientAddress>(
                                static_cast<
                                        const joynr::system::RoutingTypes::WebSocketClientAddress&>(
                                        *(element.second))));
            } else if (boost::typeindex::type_id_runtime(*element.second) ==
                       boost::typeindex::type_id<joynr::InProcessAddress>()) {
                continue;
            } else if (boost::typeindex::type_id_runtime(*element.second) ==
                       boost::typeindex::type_id<joynr::InProcessMessagingAddress>()) {
                continue;
            } else {
                JOYNR_LOG_ERROR(logger,
                                "Cannot make Variant out of {}. Address not handled.",
                                boost::typeindex::type_id_runtime(*element.second));
            }
        }
        return variantMap;
    }
};
} // namespace joynr
#endif // ROUTINGTABLE_H
