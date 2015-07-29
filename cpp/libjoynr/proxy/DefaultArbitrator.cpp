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
#include "joynr/DefaultArbitrator.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/types/QtDiscoveryEntry.h"
#include "joynr/system/QtChannelAddress.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/RequestStatus.h"
#include <vector>

namespace joynr
{

joynr_logging::Logger* DefaultArbitrator::logger =
        joynr_logging::Logging::getInstance()->getLogger("DIS", "DefaultArbitrator");

DefaultArbitrator::DefaultArbitrator(const std::string& domain,
                                     const std::string& interfaceName,
                                     joynr::system::IDiscoverySync& discoveryProxy,
                                     const DiscoveryQos& discoveryQos)
        : ProviderArbitrator(domain, interfaceName, discoveryProxy, discoveryQos)
{
}

void DefaultArbitrator::attemptArbitration()
{
    joynr::RequestStatus status;
    std::vector<joynr::types::DiscoveryEntry> result;
    discoveryProxy.lookup(status, result, domain, interfaceName, systemDiscoveryQos);
    if (status.successful()) {
        receiveCapabilitiesLookupResults(result);
    } else {
        LOG_ERROR(logger,
                  QString("Unable to lookup provider (domain: %1, interface: %2) "
                          "from discovery. Status code: %3.")
                          .arg(QString::fromStdString(domain))
                          .arg(QString::fromStdString(interfaceName))
                          .arg(QString::fromStdString(status.getCode().toString())));
    }
}

void DefaultArbitrator::receiveCapabilitiesLookupResults(
        const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries)
{
    // Check for empty results
    if (discoveryEntries.size() == 0)
        return;

    // default arbitrator picks first entry
    joynr::types::DiscoveryEntry discoveredProvider = discoveryEntries.front();
    joynr::types::CommunicationMiddleware::Enum preferredConnection(
            selectPreferredCommunicationMiddleware(discoveredProvider.getConnections()));
    updateArbitrationStatusParticipantIdAndAddress(ArbitrationStatus::ArbitrationSuccessful,
                                                   discoveredProvider.getParticipantId(),
                                                   preferredConnection);
}

} // namespace joynr
