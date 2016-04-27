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

#include "joynr/RoutingTable.h"

#include "joynr/SharedPtrDeserializer.h"
#include "joynr/MapSerializer.h"

namespace joynr
{

void RoutingTable::deserializeFromJson(const std::string& jsonString)
{
    if (jsonString.empty()) {
        return;
    }

    callbackMap = JsonSerializer::deserialize<
            std::unordered_map<std::string,
                               std::shared_ptr<const joynr::system::RoutingTypes::Address>>>(
            jsonString);
}

std::string RoutingTable::serializeToJson() const
{
    auto&& variantMap = convertDictionaryToVariantMap<
            std::unordered_map<std::string,
                               std::shared_ptr<const joynr::system::RoutingTypes::Address>>>(
            callbackMap);

    std::stringstream ss;
    joynr::MapSerializer::serialize(variantMap, ss);
    return ss.str();
}

} // namespace joynr
