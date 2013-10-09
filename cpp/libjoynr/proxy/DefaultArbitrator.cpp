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
#include "joynr/DefaultArbitrator.h"
#include "joynr/ICapabilities.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/types/ProviderQosRequirements.h"

namespace joynr {

DefaultArbitrator::DefaultArbitrator(const QString& domain,const QString& interfaceName, QSharedPointer<ICapabilities> capabilitiesStub,const DiscoveryQos &discoveryQos)
    : ProviderArbitrator(domain, interfaceName, capabilitiesStub, discoveryQos)
{

}


void DefaultArbitrator::attemptArbitration(){
    receiveCapabilitiesLookupResults(
                capabilitiesStub->lookup(domain,
                                         interfaceName,
                                         types::ProviderQosRequirements(),
                                         discoveryQos));
}

void DefaultArbitrator::receiveCapabilitiesLookupResults(const QList<CapabilityEntry> capabilityEntries){

    // Check for empty results
    if (capabilityEntries.size() == 0) return;

    updateArbitrationStatusParticipantIdAndAddress(ArbitrationStatus::ArbitrationSuccessful,
                                                   capabilityEntries.first().getParticipantId(),
                                                   capabilityEntries.first().getEndpointAddresses().first());
}

} // namespace joynr
