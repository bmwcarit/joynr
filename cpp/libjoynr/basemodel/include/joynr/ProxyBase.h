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
#ifndef PROXYBASE_H
#define PROXYBASE_H

#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/MessagingQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

class JoynrMessagingConnectorFactory;
class JoynrRuntimeImpl;

class JOYNR_EXPORT ProxyBase : public std::enable_shared_from_this<ProxyBase>
{

public:
    ProxyBase(std::weak_ptr<JoynrRuntimeImpl> runtime,
              std::shared_ptr<JoynrMessagingConnectorFactory> connectorFactory,
              const std::string& domain,
              const MessagingQos& qosSettings);
    virtual ~ProxyBase() = default;

    /**
     * Returns the participantId of the proxy object.
     * TODO: should this be part of the public API?
     * it is needed in proxy builder to register the next hop on message router.
     */
    const std::string& getProxyParticipantId() const;

protected:
    DISALLOW_COPY_AND_ASSIGN(ProxyBase);

    /*
     *  handleArbitrationFinished has to be implemented by the concrete provider proxy.
     *  It is called as soon as the arbitration result is available.
     */
    virtual void handleArbitrationFinished(
            const types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry);

    std::weak_ptr<JoynrRuntimeImpl> _runtime;
    std::shared_ptr<JoynrMessagingConnectorFactory> _connectorFactory;
    std::string _domain;
    MessagingQos _qosSettings;
    std::string _proxyParticipantId;
    types::DiscoveryEntryWithMetaInfo _providerDiscoveryEntry;
    ADD_LOGGER(ProxyBase)
};

} // namespace joynr
#endif // PROXYBASE_H
