/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef IGLOBALCAPABILITIESDIRECTORYCLIENT_H
#define IGLOBALCAPABILITIESDIRECTORYCLIENT_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/GlobalDiscoveryEntry.h"

namespace joynr
{

namespace infrastructure
{
class GlobalCapabilitiesDirectoryProxy;
} // namespace infrastructure

class IGlobalCapabilitiesDirectoryClient
{
public:
    virtual ~IGlobalCapabilitiesDirectoryClient() = default;

    virtual void add(
            const types::GlobalDiscoveryEntry& entry,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr) = 0;

    virtual void remove(
            const std::string& participantId,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError) = 0;

    virtual void lookup(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const std::vector<std::string>& gbids,
            std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& result)> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError) = 0;

    virtual void lookup(
            const std::string& participantId,
            const std::vector<std::string>& gbids,
            std::int64_t messagingTtl,
            std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError) = 0;

    virtual void removeStale(
            const std::string& clusterControllerId,
            std::int64_t maxLastSeenDateMs,
            const std::string gbid,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError) = 0;

    virtual void touch(
            const std::string& clusterControllerId,
            const std::vector<std::string>& participantIds,
            const std::string& gbid,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError) = 0;

    virtual void reAdd(
            std::shared_ptr<LocalCapabilitiesDirectoryStore> localCapabilitiesDirectoryStore,
            const std::string& localAddress) = 0;
};

} // namespace joynr
#endif // IGLOBALCAPABILITIESDIRECTORYCLIENT_H
