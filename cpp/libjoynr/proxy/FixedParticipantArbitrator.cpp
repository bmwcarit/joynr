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
#include "joynr/system/IDiscovery.h"
#include "joynr/types/QtDiscoveryEntry.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/system/QtChannelAddress.h"
#include "joynr/RequestStatus.h"
#include "joynr/DiscoveryQos.h"

#include "joynr/TypeUtil.h"

#include <cassert>

namespace joynr
{

joynr_logging::Logger* FixedParticipantArbitrator::logger =
        joynr_logging::Logging::getInstance()->getLogger("Arb", "FixedParticipantArbitrator");

FixedParticipantArbitrator::FixedParticipantArbitrator(
        const std::string& domain,
        const std::string& interfaceName,
        joynr::system::IDiscoverySync& discoveryProxy,
        const DiscoveryQos& discoveryQos)
        : ProviderArbitrator(domain, interfaceName, discoveryProxy, discoveryQos),
          participantId(discoveryQos.getCustomParameter("fixedParticipantId").getValue()),
          reqCacheDataFreshness(discoveryQos.getCacheMaxAge())
{
}

void FixedParticipantArbitrator::attemptArbitration()
{
    joynr::RequestStatus status;
    joynr::types::DiscoveryEntry result;
    discoveryProxy.lookup(status, result, participantId);
    if (status.successful()) {
        joynr::types::CommunicationMiddleware::Enum preferredConnection(
                selectPreferredCommunicationMiddleware(result.getConnections()));
        updateArbitrationStatusParticipantIdAndAddress(
                ArbitrationStatus::ArbitrationSuccessful, participantId, preferredConnection);
    } else {
        LOG_ERROR(logger,
                  QString("Unable to lookup provider (domain: %1, interface: %2) "
                          "from discovery. Status code: %3.")
                          .arg(QString::fromStdString(domain))
                          .arg(QString::fromStdString(interfaceName))
                          .arg(status.getCode().toString()));
    }
}

} // namespace joynr
