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
#ifndef PROXYBASE_H
#define PROXYBASE_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"
#include "joynr/joynrlogging.h"
#include "joynr/MessagingQos.h"
#include "joynr/system/Address.h"
#include "joynr/types/CommunicationMiddleware.h"
#include <string>

namespace joynr
{

class IClientCache;
class ConnectorFactory;

class JOYNR_EXPORT ProxyBase
{

public:
    ProxyBase(ConnectorFactory* connectorFactory,
              IClientCache* cache,
              const std::string& domain,
              const std::string& interfaceName,
              const MessagingQos& qosSettings,
              bool cached);
    virtual ~ProxyBase();

    /**
     * Returns the participantId of the proxy object.
     * Not shure if this should be part of the public api, but is needed in proxy builder to
     * register the next hop on message router.
     */
    std::string getProxyParticipantId();

protected:
    DISALLOW_COPY_AND_ASSIGN(ProxyBase);

    /*
     *  handleArbitrationFinished has to be implemented by the concrete provider proxy.
     *  It is called as soon as the arbitration result is available.
     */
    virtual void handleArbitrationFinished(
            const std::string& participantId,
            const joynr::types::CommunicationMiddleware::Enum& connection);

    ConnectorFactory* connectorFactory;
    IClientCache* cache;
    std::string domain;
    std::string interfaceName;
    MessagingQos qosSettings;
    bool cached;
    std::string providerParticipantId;
    std::string proxyParticipantId;
    joynr::types::CommunicationMiddleware::Enum* connection;
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // PROXYBASE_H
