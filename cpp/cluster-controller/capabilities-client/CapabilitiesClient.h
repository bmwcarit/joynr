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


#include <QString>
#include <QSharedPointer>
#include <QList>

namespace joynr {

class MessageRouter;
class Request;
class Reply;
class CapabilitiesInformationConverter;
class MessagingQos;
class IReplyCaller;



class JOYNRCLUSTERCONTROLLER_EXPORT CapabilitiesClient : public ICapabilitiesClient{

public:
    /*
       Default constructor for the capabilities client.
       This will create a CapabilitiesClient that is not capable of doing actual lookups.
       To upgrade to a complete CapabilitiesClient the init method must be called, and a ProxyBuilder
        must be provided. No lookups may be performed before the proxyBuilder is passed in.
       To create the GlobalCapabilitiesDirectoryProxy the provisioned data in the LocalCapabilitiesDirectory
        has to be used.
       Todo: Ownership of libjoynr is not transferred, should not be a pointer.
    */
    CapabilitiesClient(const QString& localChannelId);

    /*
      * The init method has to be caleld before any calls to the CapabilitiesClient are made.
      */
    void init(QSharedPointer<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy);

    /*
       Add a capabilities record to the directory containing a list of capabilities and the
       channelId of the provider(the client's channelId)
      */
    virtual void add(QList<types::CapabilityInformation> capabilitiesInformationList);

    /*
      Remove previously created capabilities directory entries.
      */
    virtual void remove(QList<QString> participantIds);

    /*
      Remove previously created capability directroy entry
     */
    virtual void remove(const QString& participantId);

    /*
      Channel id lookup for a known interfaceAddress.
      */
    virtual QList<types::CapabilityInformation> lookup(const QString& domain, const QString& interfaceName);

    /*
      Asynchronous channel id lookup for a known interfaceAddress.
      */
    virtual void lookup(const QString& domain, const QString& interfaceName, QSharedPointer<IGlobalCapabilitiesCallback> callback);

    virtual void lookup(const QString& participantId, QSharedPointer<IGlobalCapabilitiesCallback> callback);

    virtual ~CapabilitiesClient();

    virtual QString getLocalChannelId();

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesClient);
    void sendOneWayFunctionCall(QSharedPointer<QObject> jsonFunctionCallSharedPtr, MessagingQos qosSettings);
    Reply sendSynchronizedRequestFunctionCall(QSharedPointer<QObject> jsonFunctionCallSharedPtr, MessagingQos qosSettings);
    void sendRequest(QSharedPointer<QObject> jsonFunctionCallSharedPtr, MessagingQos qosSettings, QSharedPointer<IReplyCaller> callBack);


    qint64 defaultRequestTTL;
    qint64 defaultRequestRoundtripTTL;

    QString capabilitiesClientParticipantId;
    QString localChannelId;

    //capabilitiesProxy is a QSP, because ownership is shared between CapabilitiesClient and Joynr
    QSharedPointer<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy;
};



} // namespace joynr
#endif //CAPABILITIESCLIENT_H
