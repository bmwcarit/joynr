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

namespace joynr {

using namespace joynr_logging;
Logger* ProxyBase::logger = Logging::getInstance()->getLogger("ProxyBase", "ProxyBase");

ProxyBase::ProxyBase(
        ConnectorFactory* connectorFactory,
        IClientCache *cache,
        const QString &domain,
        const QString &interfaceName,
        const ProxyQos &proxyQos,
        const MessagingQos &qosSettings,
        bool cached
) :
        connectorFactory(connectorFactory),
        cache(cache),
        domain(domain),
        interfaceName(interfaceName),
        proxyQos(proxyQos),
        qosSettings(qosSettings),
        cached(cached),
        providerParticipantId(""),
        proxyParticipantId(""),
        providerAddress(NULL),
        destinationChannelId("DummyChannelIdForRefactoring")
{
    proxyParticipantId = QUuid::createUuid().toString();
    proxyParticipantId = proxyParticipantId.mid(1,proxyParticipantId.length()-2);
}

ProxyBase::~ProxyBase(){
}

void ProxyBase::handleArbitrationFinished(const QString &participantId, QSharedPointer<joynr::system::Address> providerAddress){
    providerParticipantId = participantId;
    this->providerAddress = providerAddress;
}

QString ProxyBase::getProxyParticipantId() {
    return this->proxyParticipantId;
}


} // namespace joynr
