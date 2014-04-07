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
#include "libjoynr/capabilities/CapabilitiesLookupRunnable.h"
#include "joynr/ICapabilities.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "libjoynr/capabilities/InProcessEndpointAddressFactory.h"

namespace joynr {

CapabilitiesLookupRunnable::CapabilitiesLookupRunnable(const DiscoveryQos& discoveryQos,
                                                       QSharedPointer<ILocalCapabilitiesCallback> callback,
                                                       ICapabilities *capabilitiesStub,
                                                       IRequestCallerDirectory *requestCallerDirectory)
    : discoveryQos(discoveryQos),
      callback(callback),
      capabilitiesStub(capabilitiesStub),
      requestCallerDirectory(requestCallerDirectory)
{

}

CapabilitiesLookupByInterfaceRunnable::CapabilitiesLookupByInterfaceRunnable(const QString &domain,
                                                               const QString &interfaceName,
                                                               const DiscoveryQos& discoveryQos,
                                                               QSharedPointer<ILocalCapabilitiesCallback> callback,
                                                               ICapabilities *capabilitiesStub,
                                                               IRequestCallerDirectory *requestCallerDirectory)
    : CapabilitiesLookupRunnable(discoveryQos, callback, capabilitiesStub, requestCallerDirectory),
      domain(domain),
      interfaceName(interfaceName)
{

}

void CapabilitiesLookupByInterfaceRunnable::run(){
    QList<CapabilityEntry> results = capabilitiesStub->lookup(domain,
                                                              interfaceName,
                                                              discoveryQos);
    InProcessEndpointAddressFactory::create(results, requestCallerDirectory);
    callback->capabilitiesReceived(results);
}

CapabilitiesLookupByIdRunnable::CapabilitiesLookupByIdRunnable(const QString& participantId,
                                                               const DiscoveryQos& discoveryQos,
                                                               QSharedPointer<ILocalCapabilitiesCallback> callback,
                                                               ICapabilities* capabilitiesStub,
                                                               IRequestCallerDirectory* requestCallerDirectory)
    : CapabilitiesLookupRunnable(discoveryQos, callback, capabilitiesStub, requestCallerDirectory),
      participantId(participantId)
{

}

void CapabilitiesLookupByIdRunnable::run(){
    QList<CapabilityEntry> results = capabilitiesStub->lookup(participantId,
                                                              discoveryQos);
    InProcessEndpointAddressFactory::create(results, requestCallerDirectory);
    callback->capabilitiesReceived(results);
}

} // namespace joynr
