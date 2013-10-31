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
#ifndef CONNECTORFACTORY_H
#define CONNECTORFACTORY_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"
#include "joynr/joynrlogging.h"
#include "joynr/IClientCache.h"
#include "joynr/MessagingQos.h"
#include "joynr/InProcessConnectorFactory.h"
#include "joynr/JoynrMessagingConnectorFactory.h"

#include <QString>
#include <QSharedPointer>

namespace joynr {

class EndpointAddressBase;
class InProcessDispatcher;

class JOYNR_EXPORT ConnectorFactory {
public:
    ConnectorFactory(InProcessConnectorFactory* inProcessConnectorFactory, JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory);
    ~ConnectorFactory();
    template <class T>
    T* create(const QString &domain,
              const QString proxyParticipantId,
              const QString& providerParticipantId,
              const MessagingQos &qosSettings,
              IClientCache *cache,
              bool cached,
              qint64 reqCacheDataFreshness_ms,
              QSharedPointer<EndpointAddressBase> endpointAddress){

        if (inProcessConnectorFactory->canBeCreated(endpointAddress)){
            return inProcessConnectorFactory->create<T>(proxyParticipantId, providerParticipantId, endpointAddress.dynamicCast<InProcessEndpointAddress>());
        }

        if (joynrMessagingConnectorFactory->canBeCreated(endpointAddress)){
            return joynrMessagingConnectorFactory->create<T>(domain, proxyParticipantId, providerParticipantId, qosSettings, cache, cached, reqCacheDataFreshness_ms);
        }

        //else if (endpointClassName == Lipci/SomeIp...)
        LOG_ERROR(logger, "Can not create Connector: Unknown endpointAddress type.");
        return NULL;
    }
private:
    DISALLOW_COPY_AND_ASSIGN(ConnectorFactory);
    InProcessConnectorFactory* inProcessConnectorFactory;
    JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory;
    static joynr_logging::Logger* logger;
};


} // namespace joynr
#endif //CONNECTORFACTORY_H
