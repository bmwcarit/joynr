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
#include "joynr/CapabilitiesAggregator.h"
#include "libjoynr/capabilities/CapabilitiesLookupRunnable.h"
#include "libjoynr/capabilities/InProcessEndpointAddressFactory.h"

namespace joynr {

CapabilitiesAggregator::CapabilitiesAggregator(ICapabilities *capabilitiesStub,
                                               IRequestCallerDirectory *requestCallerDirectory)
    : capabilitiesStub(capabilitiesStub),
      requestCallerDirectory(requestCallerDirectory),
      threadPool()
{

}

QList<CapabilityEntry> CapabilitiesAggregator::lookup(const QString &domain,
                                                      const QString &interfaceName,
                                                      const DiscoveryQos& discoveryQos){
    QList<CapabilityEntry> results;
    results = capabilitiesStub->lookup(domain,
                                       interfaceName,
                                       discoveryQos);
    return checkForInprocessParticiants(results);
}

QList<CapabilityEntry> CapabilitiesAggregator::lookup(const QString& participantId,
                                                      const DiscoveryQos& discoveryQos){
    QList<CapabilityEntry> results;
    results = capabilitiesStub->lookup(
                participantId,
                discoveryQos);
    return checkForInprocessParticiants(results);
}

void CapabilitiesAggregator::lookup(const QString& domain,
                                    const QString& interfaceName,
                                    const DiscoveryQos& discoveryQos,
                                    QSharedPointer<ILocalCapabilitiesCallback> callback)
{
    threadPool.start(new CapabilitiesLookupByInterfaceRunnable(domain,
                                                               interfaceName,
                                                               discoveryQos,
                                                               callback,
                                                               capabilitiesStub,
                                                               requestCallerDirectory));
}

void CapabilitiesAggregator::lookup(const QString& participantId,
                                    const DiscoveryQos& discoveryQos,
                                    QSharedPointer<ILocalCapabilitiesCallback> callback)
{
    threadPool.start(new CapabilitiesLookupByIdRunnable(participantId,
                                                        discoveryQos,
                                                        callback,
                                                        capabilitiesStub,
                                                        requestCallerDirectory));
}

QList<CapabilityEntry> CapabilitiesAggregator::checkForInprocessParticiants(QList<CapabilityEntry> &entryList){
    return InProcessEndpointAddressFactory::create(entryList, requestCallerDirectory);
}



void CapabilitiesAggregator::add(const QString &domain,
                                 const QString &interfaceName,
                                 const QString &participantId,
                                 const types::ProviderQos &qos,
                                 QList<QSharedPointer<joynr::system::Address> > endpointAddressList,
                                 QSharedPointer<joynr::system::Address> messagingStubAddress,
                                 const qint64& timeout_ms){
    capabilitiesStub->add(domain,
                          interfaceName,
                          participantId,
                          qos,
                          endpointAddressList,
                          messagingStubAddress,
                          timeout_ms);
}

void CapabilitiesAggregator::addEndpoint(const QString &participantId,
                                         QSharedPointer<joynr::system::Address> messagingStubAddress,
                                         const qint64& timeout_ms){
    capabilitiesStub->addEndpoint(participantId, messagingStubAddress, timeout_ms);
}

void CapabilitiesAggregator::remove(const QString &participantId,
                                    const qint64& timeout_ms){
    capabilitiesStub->remove(participantId,
                             timeout_ms);
}

} // namespace joynr
