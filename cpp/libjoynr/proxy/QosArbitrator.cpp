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

#include "joynr/system/IDiscovery.h"
#include "joynr/types/QtDiscoveryEntry.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/RequestStatus.h"
#include "joynr/types/QtProviderQos.h"

#include "joynr/TypeUtil.h"

#include <cassert>

namespace joynr
{

using namespace joynr_logging;

Logger* QosArbitrator::logger =
        joynr_logging::Logging::getInstance()->getLogger("Arbi", "QosArbitrator");

QosArbitrator::QosArbitrator(const std::string& domain,
                             const std::string& interfaceName,
                             joynr::system::IDiscoverySync& discoveryProxy,
                             const DiscoveryQos& discoveryQos)
        : ProviderArbitrator(domain, interfaceName, discoveryProxy, discoveryQos)
{
}

void QosArbitrator::attemptArbitration()
{
    std::vector<joynr::types::DiscoveryEntry> result;
    joynr::RequestStatus status(
            discoveryProxy.lookup(result, domain, interfaceName, systemDiscoveryQos));
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

// Returns true if arbitration was successful, false otherwise
void QosArbitrator::receiveCapabilitiesLookupResults(
        const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries)
{
    std::string res = "";
    joynr::types::CommunicationMiddleware::Enum preferredConnection(
            joynr::types::CommunicationMiddleware::NONE);

    // Check for empty results
    if (discoveryEntries.size() == 0)
        return;

    qint64 highestPriority = -1;
    for (const joynr::types::DiscoveryEntry discoveryEntry : discoveryEntries) {
        types::ProviderQos providerQos = discoveryEntry.getQos();
        LOG_TRACE(logger,
                  QString("Looping over capabilitiesEntry: %1")
                          .arg(QString::fromStdString(discoveryEntry.toString())));
        if (discoveryQos.getProviderMustSupportOnChange() &&
            !providerQos.getSupportsOnChangeSubscriptions()) {
            continue;
        }
        if (providerQos.getPriority() > highestPriority) {
            res = discoveryEntry.getParticipantId();
            LOG_TRACE(logger, QString("setting res to %1").arg(QString::fromStdString(res)));
            preferredConnection =
                    selectPreferredCommunicationMiddleware(discoveryEntry.getConnections());
            highestPriority = providerQos.getPriority();
        }
    }
    if (res == "") {
        LOG_WARN(logger,
                 "There was more than one entries in capabilitiesEntries, but none had a "
                 "Priority > 1");
        return;
    }

    updateArbitrationStatusParticipantIdAndAddress(
            ArbitrationStatus::ArbitrationSuccessful, res, preferredConnection);
}

} // namespace joynr
