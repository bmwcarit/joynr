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

/*
*   Client for the capabilities directory. Registration and lookup
*   requests are sent in serialized JsonFunctionCalls. The capabilities directory
*   executes the function call and responds with a JsonFunctionResponse.
*
*/

#include <string>
#include <memory>
#include <vector>

namespace joynr
{

class MessageRouter;
class Request;
class Reply;
class CapabilitiesInformationConverter;
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
    CapabilitiesClient(const std::string& localChannelId);

    /*
      * The init method has to be caleld before any calls to the CapabilitiesClient are made.
      */
    void init(std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy);

    /*
       Add a capabilities record to the directory containing a list of capabilities and the
       channelId of the provider(the client's channelId)
      */
    virtual void add(std::vector<types::CapabilityInformation> capabilitiesInformationList);

    /*
      Remove previously created capabilities directory entries.
      */
    virtual void remove(std::vector<std::string> participantIds);

    /*
      Remove previously created capability directroy entry
     */
    virtual void remove(const std::string& participantId);

    /*
      Synchronous lookup of capabilities for domain and interface.
      */
    virtual std::vector<types::CapabilityInformation> lookup(const std::string& domain,
                                                             const std::string& interfaceName);

    /*
      Asynchronous lookup of capabilities for domain and interface.
      */
    virtual void lookup(
            const std::string& domain,
            const std::string& interfaceName,
            std::function<void(const std::vector<joynr::types::CapabilityInformation>& result)>
                    onSuccess,
            std::function<void(const exceptions::JoynrException& error)> onError = nullptr);

    virtual void lookup(
            const std::string& participantId,
            std::function<void(const std::vector<joynr::types::CapabilityInformation>& result)>
                    onSuccess,
            std::function<void(const exceptions::JoynrException& error)> onError = nullptr);

    virtual ~CapabilitiesClient();

    virtual std::string getLocalChannelId();

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesClient);
    void sendOneWayFunctionCall(std::shared_ptr<QObject> jsonFunctionCallSharedPtr,
                                MessagingQos qosSettings);
    Reply sendSynchronizedRequestFunctionCall(std::shared_ptr<QObject> jsonFunctionCallSharedPtr,
                                              MessagingQos qosSettings);
    void sendRequest(std::shared_ptr<QObject> jsonFunctionCallSharedPtr,
                     MessagingQos qosSettings,
                     std::shared_ptr<IReplyCaller> callBack);

    qint64 defaultRequestTTL;
    qint64 defaultRequestRoundtripTTL;

    std::string capabilitiesClientParticipantId;
    std::string localChannelId;

    // capabilitiesProxy is a QSP, because ownership is shared between CapabilitiesClient and Joynr
    std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy;

    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // CAPABILITIESCLIENT_H
