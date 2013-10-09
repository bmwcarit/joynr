/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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

/*
*   Client for the capabilities directory. It uses a joynr proxy provided by
*   an instance of libjoynr running on the clusterController.
*
*/

#include "cluster-controller/capabilities-client/CapabilitiesClient.h"
#include "joynr/exceptions.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "cluster-controller/capabilities-client/GlobalCapabilitiesInformationCallback.h"
#include "joynr/types/ProviderQosRequirements.h"
#include "joynr/Future.h"

#include <QString>
#include <cassert>

namespace joynr {

CapabilitiesClient::CapabilitiesClient(const QString& localChannelId)
    : defaultRequestTTL(30000),
      defaultRequestRoundtripTTL(40000),
      capabilitiesClientParticipantId(),
      localChannelId(localChannelId),
      capabilitiesProxy(NULL)
{
    // We will be deserializing CapabilityInformation - register the metatypes
    qRegisterMetaType<joynr::types::CapabilityInformation>("joynr::types::CapabilityInformation");
    qRegisterMetaType<joynr__types__CapabilityInformation>("joynr__types__CapabilityInformation");
}

CapabilitiesClient::~CapabilitiesClient(){
}


QString CapabilitiesClient::getLocalChannelId(){
    return localChannelId;
}


void CapabilitiesClient::registerCapabilities(QList<types::CapabilityInformation> capabilitiesInformationList){
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once the capabilitiesProxy has been set via the init method
    if (localChannelId.isEmpty()){
        assert(false);// "Assertion in CapabilitiesClient: Local channelId is empty. Tried to register capabilities before messaging was started(no queueing implemented yet;
    } else {
        for(int i=0; i < capabilitiesInformationList.size(); i++){
            capabilitiesInformationList[i].setChannelId(localChannelId);
        }
        RequestStatus rs;
        //TM switching from sync to async
        //capabilitiesProxy->registerCapabilities(rs, capabilitiesInformationList);
        QSharedPointer<Future<void > > future(new Future<void>() );
        capabilitiesProxy->registerCapabilities(future, capabilitiesInformationList);

        //check requestStatus?

    }
}

void CapabilitiesClient::removeCapabilities(QList<types::CapabilityInformation> capabilitiesInformationList){
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once the capabilitiesProxy has been set via the init method
    Q_UNUSED(capabilitiesInformationList);
    RequestStatus status;
    capabilitiesProxy->unregisterCapabilities(status, capabilitiesInformationList);
}

QList<types::CapabilityInformation>  CapabilitiesClient::getCapabilitiesForInterfaceAddress(const QString& domain, const QString& interfaceName){
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once the capabilitiesProxy has been set via the init method
    Q_UNUSED(domain);
    Q_UNUSED(interfaceName);
    //capabilitiesProxy->getChannelsForInterfaceAddress(domain, interfaceName);
    assert(false); //not yet implemented

    return QList<types::CapabilityInformation>();
}

void CapabilitiesClient::getCapabilitiesForInterfaceAddress(const QString& domain, const QString& interfaceName, QSharedPointer<IGlobalCapabilitiesCallback> callback) {
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once the capabilitiesProxy has been set via the init method
    Q_UNUSED(domain);
    Q_UNUSED(interfaceName);
    Q_UNUSED(callback);

    //This callback in a callback is a workaround, see GlobalCapabilitiesInformationCallback.h for details
    QSharedPointer<ICallback<QList<types::CapabilityInformation> > > icallback(
                QSharedPointer<ICallback<QList<types::CapabilityInformation> > >(
                    new GlobalCapabilitiesInformationCallback(callback)
                    )
                );
    capabilitiesProxy->lookupCapabilities(icallback, domain, interfaceName, types::ProviderQosRequirements()); //QList QString is needed, because capabilititiesInterface on java expects a map<string, string>
}


QList<types::CapabilityInformation> CapabilitiesClient::getCapabilitiesForChannelId(const QString& channelId){
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once the capabilitiesProxy has been set via the init method
    Q_UNUSED(channelId);
    assert(false); //not yet implemented
    //capabilitiesProxy->getCapabilitiesForChannelId(channelId);
    return QList<types::CapabilityInformation>();
}

void CapabilitiesClient::getCapabilitiesForChannelId(const QString& channelId, QSharedPointer<IGlobalCapabilitiesCallback> callback) {
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once the capabilitiesProxy has been set via the init method
    Q_UNUSED(channelId);
    Q_UNUSED(callback);
    assert(false); //not yet implemented

    //capabilitiesProxy->getCapabilitiesForChannelId(channelId,callback);
}

void CapabilitiesClient::getCapabilitiesForParticipantId(const QString& participantId, QSharedPointer<IGlobalCapabilitiesCallback> callback){
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once the capabilitiesProxy has been set via the init method
    Q_UNUSED(participantId);
    Q_UNUSED(callback);
    assert(false); //not yet implemented
    //capabilitiesProxy->getCapabilitiesForParticipantId(participantId, callback);
}

void CapabilitiesClient::init(QSharedPointer<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy)
{
    this->capabilitiesProxy = capabilitiesProxy;
}


} // namespace joynr

