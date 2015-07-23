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
#include "joynr/DeclareMetatypeUtil.h"
#include "joynr/ProxyBase.h"

#include <QUuid>

namespace joynr
{

using namespace joynr_logging;
Logger* ProxyBase::logger = Logging::getInstance()->getLogger("ProxyBase", "ProxyBase");

ProxyBase::ProxyBase(ConnectorFactory* connectorFactory,
                     IClientCache* cache,
                     const std::string& domain,
                     const std::string& interfaceName,
                     const MessagingQos& qosSettings,
                     bool cached)
        : connectorFactory(connectorFactory),
          cache(cache),
          domain(domain),
          interfaceName(interfaceName),
          qosSettings(qosSettings),
          cached(cached),
          providerParticipantId(""),
          proxyParticipantId(""),
          connection(NULL)
{
    QString internalId = QUuid::createUuid().toString();
    proxyParticipantId = internalId.mid(1, internalId.length() - 2).toStdString();
}

ProxyBase::~ProxyBase()
{
    delete connection;
}

void ProxyBase::handleArbitrationFinished(
        const std::string& participantId,
        const joynr::types::StdCommunicationMiddleware::Enum& connection)
{
    providerParticipantId = participantId;
    this->connection = new joynr::types::StdCommunicationMiddleware::Enum(connection);
}

std::string ProxyBase::getProxyParticipantId()
{
    return this->proxyParticipantId;
}

} // namespace joynr
