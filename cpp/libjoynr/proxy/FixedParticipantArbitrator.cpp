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
#include "joynr/FixedParticipantArbitrator.h"
#include "joynr/ArbitrationStatus.h"
#include "joynr/ICapabilities.h"
#include "joynr/DiscoveryQos.h"

#include <cassert>

namespace joynr {

FixedParticipantArbitrator::FixedParticipantArbitrator(const QString& domain,const QString& interfaceName, QSharedPointer<ICapabilities> capabilitiesStub,const DiscoveryQos &discoveryQos) :
    ProviderArbitrator(domain, interfaceName, capabilitiesStub, discoveryQos),
    participantId(discoveryQos.getCustomParameter("fixedParticipantId").getValue()),
    reqCacheDataFreshness(discoveryQos.getCacheMaxAge())
{
}

void FixedParticipantArbitrator::attemptArbitration()
{
    QList<CapabilityEntry> result = capabilitiesStub->lookup(participantId, discoveryQos);

    if (result.isEmpty()) return;
    assert(result.first().getEndpointAddresses().size() > 0);

    updateArbitrationStatusParticipantIdAndAddress(ArbitrationStatus::ArbitrationSuccessful,
                                                   participantId,
                                                   result.first().getEndpointAddresses().first());
}


} // namespace joynr
