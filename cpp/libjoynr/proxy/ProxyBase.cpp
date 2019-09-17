/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "joynr/ProxyBase.h"
#include <tuple>
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/Util.h"

namespace joynr
{

ProxyBase::ProxyBase(std::weak_ptr<JoynrRuntimeImpl> runtime,
                     std::shared_ptr<JoynrMessagingConnectorFactory> connectorFactory,
                     const std::string& domain,
                     const MessagingQos& qosSettings)
        : _runtime(std::move(runtime)),
          _connectorFactory(std::move(connectorFactory)),
          _domain(domain),
          _qosSettings(qosSettings),
          _proxyParticipantId(""),
          _providerDiscoveryEntry()
{
    _proxyParticipantId = util::createUuid();
}

void ProxyBase::handleArbitrationFinished(
        const types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry)
{
    this->_providerDiscoveryEntry = providerDiscoveryEntry;
}

const std::string& ProxyBase::getProxyParticipantId() const
{
    return this->_proxyParticipantId;
}

} // namespace joynr
