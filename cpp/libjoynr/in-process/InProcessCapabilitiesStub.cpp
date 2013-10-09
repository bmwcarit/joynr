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
#include "libjoynr/in-process/InProcessCapabilitiesStub.h"
#include <cassert>

namespace joynr {

InProcessCapabilitiesStub::InProcessCapabilitiesStub(ICapabilities *skeleton)
    : skeleton(skeleton)
{

}

QList<CapabilityEntry> InProcessCapabilitiesStub::lookup(const QString &domain, const QString &interfaceName, const types::ProviderQosRequirements &qos, const DiscoveryQos& discoveryQos){
    assert(skeleton!=NULL);
    return skeleton->lookup(domain, interfaceName, qos, discoveryQos);
}

QList<CapabilityEntry> InProcessCapabilitiesStub::lookup(const QString &participantId, const DiscoveryQos& discoveryQos){
    assert(skeleton!=NULL);
    return skeleton->lookup(participantId, discoveryQos);
}

void InProcessCapabilitiesStub::addEndpoint(const QString &participantId, QSharedPointer<EndpointAddressBase> messagingStubAddress, const qint64& timeout_ms){
    assert(skeleton!=NULL);
    skeleton->addEndpoint(participantId, messagingStubAddress, timeout_ms);
}

void InProcessCapabilitiesStub::add(const QString &domain, const QString &interfaceName, const QString &participantId, const types::ProviderQos &qos, QList<QSharedPointer<EndpointAddressBase> > endpointAddressList, QSharedPointer<EndpointAddressBase> messagingStubAddress, const qint64& timeout_ms){
    assert(skeleton!=NULL);
    skeleton->add(domain, interfaceName, participantId, qos, endpointAddressList, messagingStubAddress, timeout_ms);
}

void InProcessCapabilitiesStub::remove(const QString& participantId, const qint64& timeout_ms){
    assert(skeleton!=NULL);
    skeleton->remove(participantId, timeout_ms);
}

} // namespace joynr
