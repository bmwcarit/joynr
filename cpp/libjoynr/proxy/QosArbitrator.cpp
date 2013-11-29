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
#include "joynr/QosArbitrator.h"
#include "joynr/JoynrMessagingEndpointAddress.h"

#include "joynr/ICapabilities.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/ProviderQosRequirements.h"

#include <cassert>

namespace joynr {

using namespace joynr_logging;

Logger* QosArbitrator::logger = joynr_logging::Logging::getInstance()->getLogger("Arbi", "QosArbitrator");


QosArbitrator::QosArbitrator(const QString& domain, const QString& interfaceName, QSharedPointer<ICapabilities> capabilitiesStub, const DiscoveryQos &discoveryQos)
    : ProviderArbitrator(domain, interfaceName, capabilitiesStub, discoveryQos),
      keyword( discoveryQos.getCustomParameter("keyword").getValue())
{
}


void QosArbitrator::attemptArbitration()
{
   receiveCapabilitiesLookupResults(
        capabilitiesStub->lookup(domain,
                                 interfaceName,
                                 types::ProviderQosRequirements(),
                                 discoveryQos
                                 ));
}


// Returns true if arbitration was successful, false otherwise
void QosArbitrator::receiveCapabilitiesLookupResults(const QList<CapabilityEntry> capabilityEntries){
    QString res = "";
    QSharedPointer<EndpointAddressBase> endpointAddressResult;

    // Check for empty results
    if (capabilityEntries.size() == 0) return;

    qint64 highestPriority = -1;
    QListIterator<CapabilityEntry> capabilitiesIterator(capabilityEntries);
    while (capabilitiesIterator.hasNext()) {
        CapabilityEntry capEntry = capabilitiesIterator.next();
        types::ProviderQos providerQos = capEntry.getQos();
        LOG_TRACE(logger,"Looping over capabilitiesEntry: " + capEntry.toString());
        if ( discoveryQos.getProviderMustSupportOnChange() &&  !providerQos.getSupportsOnChangeSubscriptions()) {
            continue;
        }
        if ( providerQos.getPriority() > highestPriority) {
            res = capEntry.getParticipantId();
            LOG_TRACE(logger,"setting res to " + res);
            //TODO decide which endpointAddress to choose
            assert(capEntry.getEndpointAddresses().size()>0);
            endpointAddressResult = capEntry.getEndpointAddresses().first();
            highestPriority = providerQos.getPriority();
        }
    }
    if (res==""){
        LOG_WARN(logger,"There was more than one entries in capabilitiesEntries, but none had a Priority > 1");
        return;
    }

    updateArbitrationStatusParticipantIdAndAddress(ArbitrationStatus::ArbitrationSuccessful, res, endpointAddressResult);
}


} // namespace joynr
