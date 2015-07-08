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
#include <QSharedPointer>
#include <QList>
#include <QSettings>

namespace joynr
{

class MessageRouter;
class Request;
class Reply;
class CapabilitiesInformationConverter;
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
    virtual void add(QList<types::CapabilityInformation> capabilitiesInformationList);

    /*
      Remove previously created capabilities directory entries.
      */
    virtual void remove(QList<std::string> participantIds);

    /*
      Channel id lookup for a known interfaceAddress.
      */
    virtual QList<types::CapabilityInformation> lookup(const std::string& domain,
                                                       const std::string& interfaceName);

    /*
      Asynchronous channel id lookup for a known interfaceAddress.
      */
    virtual void lookup(const std::string& domain,
                        const std::string& interfaceName,
                        QSharedPointer<IGlobalCapabilitiesCallback> callback);

    virtual void lookup(const std::string& participantId,
                        QSharedPointer<IGlobalCapabilitiesCallback> callback);

    virtual std::string getLocalChannelId();

private:
    DISALLOW_COPY_AND_ASSIGN(FakeCapabilitiesClient);
    void sendOneWayFunctionCall(QSharedPointer<QObject> jsonFunctionCallSharedPtr,
                                MessagingQos qosSettings);
    Reply sendSynchronizedRequestFunctionCall(QSharedPointer<QObject> jsonFunctionCallSharedPtr,
                                              MessagingQos qosSettings);
    void sendRequest(QSharedPointer<QObject> jsonFunctionCallSharedPtr,
                     MessagingQos qosSettings,
                     QSharedPointer<IReplyCaller> callBack);

    QList<types::CapabilityInformation> createFakedCapInfoList(const QString& domain,
                                                               const QString& interfaceName);
    QList<types::CapabilityInformation> createFakedCapInfoListForChannelId(
            const QString& channelId);
    QList<types::CapabilityInformation> createFakedCapInfoListForParticipantId(
            const QString& participantId);
    QList<types::CapabilityInformation> createFakedCapInfoList();

    qint64 defaultRequestTTL;
    qint64 defaultRequestRoundtripTTL;

    std::string capabilitiesClientParticipantId;
    std::string localChannelId;

    QSettings configuration;

    QString preconfiguredDomain;
    QString preconfiguredInterfaceName;
    QString preconfiguredChannelId;
    QString preconfiguredParticipantId;
};

} // namespace joynr
#endif // FAKECAPABILITIESCLIENT_H
