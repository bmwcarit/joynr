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
#include "joynr/DiscoveryQos.h"
#include "joynr/types/CustomParameter.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/system/DiscoveryEntry.h"
#include "joynr/RequestStatus.h"

#include <cassert>

namespace joynr {

KeywordArbitrator::KeywordArbitrator(
        const QString& domain,
        const QString& interfaceName,
        joynr::system::IDiscoverySync& discoveryProxy,
        const DiscoveryQos &discoveryQos
) :
    ProviderArbitrator(domain, interfaceName, discoveryProxy, discoveryQos),
    keyword(discoveryQos.getCustomParameter(DiscoveryQos::KEYWORD_PARAMETER()).getValue()),
    logger(joynr_logging::Logging::getInstance()->getLogger("KArb", "KeywordArbitrator"))
{
}


void KeywordArbitrator::attemptArbitration() {
    joynr::RequestStatus status;
    QList<joynr::system::DiscoveryEntry> result;
    discoveryProxy.lookup(
                status,
                result,
                domain,
                interfaceName,
                systemDiscoveryQos
    );
    if(status.successful()) {
        receiveCapabilitiesLookupResults(result);
    } else {
        LOG_ERROR(
                    logger,
                    QString("Unable to lookup provider (domain: %1, interface: %2) "
                            "from discovery. Status code: %3."
                    )
                    .arg(domain)
                    .arg(interfaceName)
                    .arg(status.getCode().toString())
        );
    }
}

void KeywordArbitrator::receiveCapabilitiesLookupResults(
        const QList<joynr::system::DiscoveryEntry>& discoveryEntries
) {
    // Check for an empty list of results
    if (discoveryEntries.size() == 0) {
        return;
    }

    // Loop through the result list
    QListIterator<joynr::system::DiscoveryEntry> discoveryEntriesIterator(discoveryEntries);
    while (discoveryEntriesIterator.hasNext()) {
        joynr::system::DiscoveryEntry discoveryEntry = discoveryEntriesIterator.next();
        types::ProviderQos providerQos = discoveryEntry.getQos();
        LOG_TRACE(logger,"Looping over capabilitiesEntry: " + discoveryEntry.toString());

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
                QString res = discoveryEntry.getParticipantId();
                LOG_TRACE(logger,"setting res to " + res);
                joynr::system::CommunicationMiddleware::Enum preferredConnection(
                        selectPreferredCommunicationMiddleware(discoveryEntry.getConnections())
                );
                updateArbitrationStatusParticipantIdAndAddress(
                            ArbitrationStatus::ArbitrationSuccessful,
                            res,
                            preferredConnection
                );
                return;
            }
        }
    }

    // If this point is reached, no provider with the keyword was found
}


} // namespace joynr
