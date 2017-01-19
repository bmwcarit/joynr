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
#ifndef CAPABILITIESCLIENT_H
#define CAPABILITIESCLIENT_H

#include <string>
#include <memory>
#include <vector>

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/types/GlobalDiscoveryEntry.h"
#include "joynr/types/DiscoveryQos.h"
#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/Logger.h"

/*
*   Client for the global capabilities directory. Registration and lookup
*   requests are sent in serialized JsonFunctionCalls. The capabilities directory
*   executes the function call and responds with a JsonFunctionResponse.
*/

namespace joynr
{

class JOYNRCLUSTERCONTROLLER_EXPORT CapabilitiesClient : public ICapabilitiesClient
{

public:
    /*
       Default constructor for the capabilities client.
       This will create a CapabilitiesClient that is not capable of doing actual lookups.
       To upgrade to a complete CapabilitiesClient the setProxyBuilder method must be called, and a
       ProxyBuilder must be provided. The Class will take ownership
       of the ProxyBuilder and will make sure it is deleted.
    */
    CapabilitiesClient();

    ~CapabilitiesClient() override = default;

    /*
       Add a capabilities record to the directory
      */
    void add(const types::GlobalDiscoveryEntry& entry,
             std::function<void()> onSuccess,
             std::function<void(const exceptions::JoynrRuntimeException& error)> onError) override;

    /*
      Remove previously created capabilities directory entries.
      */
    void remove(std::vector<std::string> participantIds) override;

    /*
      Remove previously created capability directroy entry
     */
    void remove(const std::string& participantId) override;

    /*
      Synchronous lookup of capabilities for domain and interface.
      */
    std::vector<types::GlobalDiscoveryEntry> lookup(const std::vector<std::string>& domains,
                                                    const std::string& interfaceName,
                                                    const std::int64_t messagingTtl) override;

    /*
      Asynchronous lookup of capabilities for domain and interface.
      */
    void lookup(const std::vector<std::string>& domains,
                const std::string& interfaceName,
                const std::int64_t messagingTtl,
                std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& result)>
                        onSuccess,
                std::function<void(const exceptions::JoynrRuntimeException& error)> onError =
                        nullptr) override;

    void lookup(const std::string& participantId,
                std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& result)>
                        onSuccess,
                std::function<void(const exceptions::JoynrRuntimeException& error)> onError =
                        nullptr) override;

    void touch(const std::string& clusterControllerId,
               std::function<void()> onSuccess = nullptr,
               std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError =
                       nullptr) override;

    void setProxyBuilder(
            std::unique_ptr<IProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>>
                    capabilitiesProxyBuilder) override;

private:
    std::unique_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy>
    getGlobalCapabilitiesDirectoryProxy(std::int64_t messagingTtl);

    DISALLOW_COPY_AND_ASSIGN(CapabilitiesClient);

    std::unique_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> defaultCapabilitiesProxy;
    std::unique_ptr<IProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>>
            capabilitiesProxyBuilder;
    ADD_LOGGER(CapabilitiesClient);
};

} // namespace joynr
#endif // CAPABILITIESCLIENT_H
