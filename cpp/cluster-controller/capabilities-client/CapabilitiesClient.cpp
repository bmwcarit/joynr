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
#include "cluster-controller/capabilities-client/GlobalCapabilityInformationCallback.h"
#include "joynr/Future.h"

#include <QString>
#include <cassert>

namespace joynr
{

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

CapabilitiesClient::~CapabilitiesClient()
{
}

QString CapabilitiesClient::getLocalChannelId()
{
    return localChannelId;
}

void CapabilitiesClient::add(QList<types::CapabilityInformation> capabilitiesInformationList)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
                                         // the capabilitiesProxy has been set via the init method
    if (localChannelId.isEmpty()) {
        assert(false); // "Assertion in CapabilitiesClient: Local channelId is empty. Tried to
                       // register capabilities before messaging was started(no queueing implemented
                       // yet;
    } else {
        for (int i = 0; i < capabilitiesInformationList.size(); i++) {
            capabilitiesInformationList[i].setChannelId(localChannelId);
        }
        RequestStatus rs;
        // TM switching from sync to async
        // capabilitiesProxy->add(rs, capabilitiesInformationList);
        QSharedPointer<Future<void>> future(new Future<void>());
        capabilitiesProxy->add(future, capabilitiesInformationList);

        // check requestStatus?
    }
}

void CapabilitiesClient::remove(const QString& participantId)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
                                         // the capabilitiesProxy has been set via the init method
    RequestStatus status;
    capabilitiesProxy->remove(status, participantId);
}

void CapabilitiesClient::remove(QList<QString> participantIdList)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
                                         // the capabilitiesProxy has been set via the init method
    RequestStatus status;
    capabilitiesProxy->remove(status, participantIdList);
}

QList<types::CapabilityInformation> CapabilitiesClient::lookup(const QString& domain,
                                                               const QString& interfaceName)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
                                         // the capabilitiesProxy has been set via the init method

    RequestStatus status;
    QList<types::CapabilityInformation> result;
    capabilitiesProxy->lookup(status, result, domain, interfaceName);
    return result;
}

void CapabilitiesClient::lookup(const QString& domain,
                                const QString& interfaceName,
                                QSharedPointer<IGlobalCapabilitiesCallback> callback)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
                                         // the capabilitiesProxy has been set via the init method

    // This callback in a callback is a workaround, see GlobalCapabilitiesInformationCallback.h for
    // details
    QSharedPointer<ICallback<QList<types::CapabilityInformation>>> capabilitiesCallback(
            new GlobalCapabilitiesInformationCallback(callback));
    capabilitiesProxy->lookup(
            capabilitiesCallback, domain, interfaceName); // QList QString is needed, because
                                                          // capabilititiesInterface on java expects
                                                          // a map<string, string>
}

void CapabilitiesClient::lookup(const QString& participantId,
                                QSharedPointer<IGlobalCapabilitiesCallback> callback)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
                                         // the capabilitiesProxy has been set via the init method
    // This callback in a callback is a workaround, see GlobalCapabilityInformationCallback.h for
    // details
    QSharedPointer<ICallback<types::CapabilityInformation>> capabilitiesCallback(
            new GlobalCapabilityInformationCallback(callback));
    capabilitiesProxy->lookup(capabilitiesCallback, participantId);
}

void CapabilitiesClient::init(
        QSharedPointer<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy)
{
    this->capabilitiesProxy = capabilitiesProxy;
}

} // namespace joynr
