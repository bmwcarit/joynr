/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrClusterControllerExport.h"
#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/Logger.h"

/*
*   Client for the capabilities directory. Registration and lookup
*   requests are sent in serialized JsonFunctionCalls. The capabilities directory
*   executes the function call and responds with a JsonFunctionResponse.
*
*/

#include <string>
#include <memory>
#include <vector>
#include <QObject>
namespace joynr
{

class Reply;
class MessagingQos;
class IReplyCaller;

class JOYNRCLUSTERCONTROLLER_EXPORT CapabilitiesClient : public ICapabilitiesClient
{

public:
    /*
       Default constructor for the capabilities client.
       This will create a CapabilitiesClient that is not capable of doing actual lookups.
       To upgrade to a complete CapabilitiesClient the init method must be called, and a
       ProxyBuilder
        must be provided. No lookups may be performed before the proxyBuilder is passed in.
       To create the GlobalCapabilitiesDirectoryProxy the provisioned data in the
       LocalCapabilitiesDirectory
        has to be used.
       Todo: Ownership of libjoynr is not transferred, should not be a pointer.
    */
    explicit CapabilitiesClient(const std::string& localChannelId);

    /*
      * The init method has to be caleld before any calls to the CapabilitiesClient are made.
      */
    void init(std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy);

    /*
       Add a capabilities record to the directory containing a list of capabilities and the
       channelId of the provider(the client's channelId)
      */
    void add(std::vector<types::CapabilityInformation> capabilitiesInformationList) override;

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
    std::vector<types::CapabilityInformation> lookup(const std::string& domain,
                                                     const std::string& interfaceName) override;

    /*
      Asynchronous lookup of capabilities for domain and interface.
      */
    void lookup(const std::string& domain,
                const std::string& interfaceName,
                std::function<void(const std::vector<joynr::types::CapabilityInformation>& result)>
                        onSuccess,
                std::function<void(const exceptions::JoynrRuntimeException& error)> onError =
                        nullptr) override;

    void lookup(const std::string& participantId,
                std::function<void(const std::vector<joynr::types::CapabilityInformation>& result)>
                        onSuccess,
                std::function<void(const exceptions::JoynrRuntimeException& error)> onError =
                        nullptr) override;

    ~CapabilitiesClient() override = default;

    std::string getLocalChannelId() override;

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesClient);

    std::string localChannelId;

    // capabilitiesProxy is a shared_ptr, because ownership is shared between CapabilitiesClient and
    // Joynr
    std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy;

    ADD_LOGGER(CapabilitiesClient);
};

} // namespace joynr
#endif // CAPABILITIESCLIENT_H
