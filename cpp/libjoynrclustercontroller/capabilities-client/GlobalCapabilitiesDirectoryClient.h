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
#ifndef GLOBALCAPABILITIESDIRECTORYCLIENT_H
#define GLOBALCAPABILITIESDIRECTORYCLIENT_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/Logger.h"
#include "joynr/MessagingQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/types/DiscoveryError.h"

#include "IGlobalCapabilitiesDirectoryClient.h"

/*
*   Client for the global capabilities directory. Registration and lookup
*   requests are sent in serialized JsonFunctionCalls. The capabilities directory
*   executes the function call and responds with a JsonFunctionResponse.
*/
namespace joynr
{
class ClusterControllerSettings;

namespace exceptions
{
class JoynrRuntimeException;
}

namespace infrastructure
{
class GlobalCapabilitiesDirectoryProxy;
}

namespace types
{
class GlobalDiscoveryEntry;
}

class JOYNRCLUSTERCONTROLLER_EXPORT GlobalCapabilitiesDirectoryClient
        : public IGlobalCapabilitiesDirectoryClient
{

public:
    /*
       Default constructor for the GlobalCapabilitiesDirectory client.
       This will create a GlobalCapabilitiesDirectoryClient that is not capable of doing actual
       lookups.
       To upgrade to a complete GlobalCapabilitiesDirectoryClient the setProxy method must be
       called, and a Proxy must be provided.
     */
    GlobalCapabilitiesDirectoryClient(const ClusterControllerSettings& clusterControllerSettings);

    ~GlobalCapabilitiesDirectoryClient() override = default;

    /*
       Add a capabilities record to the directory
     */
    void add(const types::GlobalDiscoveryEntry& entry,
             const std::vector<std::string>& gbids,
             std::function<void()> onSuccess,
             std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
             std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
            override;

    /*
      Remove previously created capability directroy entry
     */
    void remove(const std::string& participantId,
                const std::vector<std::string>& gbids,
                std::function<void()> onSuccess,
                std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
                std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
            override;

    /*
      Asynchronous lookup of capabilities for domain and interface.
     */
    void lookup(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const std::vector<std::string>& gbids,
            std::int64_t messagingTtl,
            std::function<void(const std::vector<types::GlobalDiscoveryEntry>& result)> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
            std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
            override;

    void lookup(const std::string& participantId,
                const std::vector<std::string>& gbids,
                std::int64_t messagingTtl,
                std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& result)>
                        onSuccess,
                std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
                std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
            override;

    void touch(const std::string& clusterControllerId,
               const std::vector<std::string>& participantIds,
               std::function<void()> onSuccess = nullptr,
               std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError =
                       nullptr) override;

    void setProxy(
            std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy,
            MessagingQos messagingQos);

    void removeStale(const std::string& clusterControllerId,
                     std::int64_t maxLastSeenDateMs,
                     std::function<void()> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                             onRuntimeError) override;

private:
    DISALLOW_COPY_AND_ASSIGN(GlobalCapabilitiesDirectoryClient);
    std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> _capabilitiesProxy;
    MessagingQos _messagingQos;
    const std::uint64_t _touchTtl;
    const std::uint64_t _removeStaleTtl;
    ADD_LOGGER(GlobalCapabilitiesDirectoryClient)
};

} // namespace joynr
#endif // GLOBALCAPABILITIESDIRECTORYCLIENT_H
