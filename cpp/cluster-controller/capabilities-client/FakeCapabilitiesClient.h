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
#ifndef FAKECAPABILITIESCLIENT_H
#define FAKECAPABILITIESCLIENT_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrClusterControllerExport.h"
#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"

/*
*   Client for the capabilities directory. Registration and lookup
*   requests are sent in serialized JsonFunctionCalls. The capabilities directory
*   executes the function call and responds with a JsonFunctionResponse.
*
*/

#include <QString>
#include <string>
#include <QSettings>
#include <vector>
#include <memory>

namespace joynr
{

class Reply;
class MessagingQos;
class IReplyCaller;
class IGlobalCapabilitiesCallback;

class JOYNRCLUSTERCONTROLLER_EXPORT FakeCapabilitiesClient : public ICapabilitiesClient
{

public:
    /*
       Default constructor for the capabilities client.
       dispatcherPrt - pointer to a dispatcher instance created by the dispatcherFactory
      */
    FakeCapabilitiesClient(const std::string& localChannelId, const QString& settingsFileName);

    virtual ~FakeCapabilitiesClient();
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
      Channel id lookup for a known interfaceAddress.
      */
    virtual std::vector<types::CapabilityInformation> lookup(const std::string& domain,
                                                             const std::string& interfaceName);

    /*
      Asynchronous channel id lookup for a known interfaceAddress.
      */
    virtual void lookup(const std::string& domain,
                        const std::string& interfaceName,
                        std::shared_ptr<IGlobalCapabilitiesCallback> callback);

    virtual void lookup(const std::string& participantId,
                        std::shared_ptr<IGlobalCapabilitiesCallback> callback);

    virtual std::string getLocalChannelId();

private:
    DISALLOW_COPY_AND_ASSIGN(FakeCapabilitiesClient);
    void sendOneWayFunctionCall(std::shared_ptr<QObject> jsonFunctionCallSharedPtr,
                                MessagingQos qosSettings);
    Reply sendSynchronizedRequestFunctionCall(std::shared_ptr<QObject> jsonFunctionCallSharedPtr,
                                              MessagingQos qosSettings);
    void sendRequest(std::shared_ptr<QObject> jsonFunctionCallSharedPtr,
                     MessagingQos qosSettings,
                     std::shared_ptr<IReplyCaller> callBack);

    std::vector<types::CapabilityInformation> createFakedCapInfoList(
            const std::string& domain,
            const std::string& interfaceName);
    std::vector<types::CapabilityInformation> createFakedCapInfoListForChannelId(
            const std::string& channelId);
    std::vector<types::CapabilityInformation> createFakedCapInfoListForParticipantId(
            const std::string& participantId);
    std::vector<types::CapabilityInformation> createFakedCapInfoList();

    qint64 defaultRequestTTL;
    qint64 defaultRequestRoundtripTTL;

    std::string capabilitiesClientParticipantId;
    std::string localChannelId;

    QSettings configuration;

    std::string preconfiguredDomain;
    std::string preconfiguredInterfaceName;
    std::string preconfiguredChannelId;
    std::string preconfiguredParticipantId;
};

} // namespace joynr
#endif // FAKECAPABILITIESCLIENT_H
