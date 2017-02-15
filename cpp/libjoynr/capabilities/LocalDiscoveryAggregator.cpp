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
#include "joynr/LocalDiscoveryAggregator.h"

#include <chrono>
#include <limits>
#include <utility>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

LocalDiscoveryAggregator::LocalDiscoveryAggregator(
        std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo> provisionedDiscoveryEntries)
        : discoveryProxy(), provisionedDiscoveryEntries(std::move(provisionedDiscoveryEntries))
{
}

void LocalDiscoveryAggregator::setDiscoveryProxy(
        std::unique_ptr<joynr::system::IDiscoverySync> discoveryProxy)
{
    this->discoveryProxy = std::move(discoveryProxy);
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::add(const joynr::types::DiscoveryEntry& discoveryEntry)
{
    if (!discoveryProxy) {
        throw exceptions::JoynrRuntimeException(
                "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                "local capabilitites directory.");
    }

    discoveryProxy->add(discoveryEntry);
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::lookup(std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result,
                                      const std::vector<std::string>& domains,
                                      const std::string& interfaceName,
                                      const joynr::types::DiscoveryQos& discoveryQos)
{
    if (!discoveryProxy) {
        throw exceptions::JoynrRuntimeException(
                "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                "local capabilitites directory.");
    }
    discoveryProxy->lookup(result, domains, interfaceName, discoveryQos);
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::lookup(joynr::types::DiscoveryEntryWithMetaInfo& result,
                                      const std::string& participantId)
{
    auto entry = provisionedDiscoveryEntries.find(participantId);
    if (entry != provisionedDiscoveryEntries.end()) {
        result = entry->second;
    } else {
        if (!discoveryProxy) {
            throw exceptions::JoynrRuntimeException(
                    "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                    "local capabilitites directory.");
        }
        discoveryProxy->lookup(result, participantId);
    }
}

// inherited from joynr::system::IDiscoverySync
void LocalDiscoveryAggregator::remove(const std::string& participantId)
{
    if (!discoveryProxy) {
        throw exceptions::JoynrRuntimeException(
                "LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                "local capabilitites directory.");
    }
    discoveryProxy->remove(participantId);
}

} // namespace joynr
