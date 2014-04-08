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
#include "joynr/KeywordArbitrator.h"
#include "joynr/ICapabilities.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/types/CustomParameter.h"

#include <cassert>

namespace joynr {

KeywordArbitrator::KeywordArbitrator(const QString& domain, const QString& interfaceName, QSharedPointer<ICapabilities> capabilitiesStub, const DiscoveryQos &discoveryQos)
    :     ProviderArbitrator(domain, interfaceName, capabilitiesStub, discoveryQos),
      keyword(discoveryQos.getCustomParameter(DiscoveryQos::KEYWORD_PARAMETER()).getValue()),
    logger(joynr_logging::Logging::getInstance()->getLogger("KArb", "KeywordArbitrator"))
{
}


void KeywordArbitrator::attemptArbitration()
{
    receiveCapabilitiesLookupResults(
                capabilitiesStub->lookup(domain,
                                         interfaceName,
                                         discoveryQos));

}

void KeywordArbitrator::receiveCapabilitiesLookupResults(const QList<CapabilityEntry> capabilityEntries){
    // Check for an empty list of results
    if (capabilityEntries.size() == 0) {
        return;
    }

    // Loop through the result list
    QListIterator<CapabilityEntry> capabilitiesIterator(capabilityEntries);
    while (capabilitiesIterator.hasNext()) {
        CapabilityEntry capEntry = capabilitiesIterator.next();
        types::ProviderQos providerQos = capEntry.getQos();
        LOG_TRACE(logger,"Looping over capabilitiesEntry: " + capEntry.toString());

        // Check that the provider supports onChange subscriptions if this was requested
        if ( discoveryQos.getProviderMustSupportOnChange() &&  !providerQos.getSupportsOnChangeSubscriptions()) {
            continue;
        }

        // Search the QosParameters for the keyword field
        QList<types::CustomParameter> qosParameters = providerQos.getCustomParameters();
        QListIterator<types::CustomParameter> parameterIterator(qosParameters);
        while (parameterIterator.hasNext()) {
            types::CustomParameter parameter = parameterIterator.next();
            QString name = parameter.getName();
            if (name == DiscoveryQos::KEYWORD_PARAMETER() && keyword == parameter.getValue()) {
                QString res = capEntry.getParticipantId();
                LOG_TRACE(logger,"setting res to " + res);
                //TODO decide which endpointAddress to choose
                assert(capEntry.getEndpointAddresses().size()>0);
                QSharedPointer<joynr::system::Address> endpointAddressResult = capEntry.getEndpointAddresses().first();
                updateArbitrationStatusParticipantIdAndAddress(ArbitrationStatus::ArbitrationSuccessful, res, endpointAddressResult);
                return;
            }
        }
    }

    // If this point is reached, no provider with the keyword was found
}


} // namespace joynr
