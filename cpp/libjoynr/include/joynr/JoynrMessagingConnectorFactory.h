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
#ifndef JOYNRMESSAGINGCONNECTORFACTORY_H
#define JOYNRMESSAGINGCONNECTORFACTORY_H

#include "joynr/JoynrExport.h"
#include "joynr/system/CommunicationMiddleware.h"

#include <QString>
#include <QSharedPointer>

namespace joynr {

class IJoynrMessageSender;
class SubscriptionManager;
class MessagingQos;
class IClientCache;
namespace system { class Address; }

// Default implementation of a JoynrMessagingConnectorFactoryHelper
// Template specializations are found in the generated *JoynrMessagingConnector.h files
template <class T>
class JoynrMessagingConnectorFactoryHelper {
public:
    T* create(
            IJoynrMessageSender* messageSender,
            SubscriptionManager* subscriptionManager,
            const QString &domain,
            const QString &interfaceName,
            const QString proxyParticipantId,
            const QString& providerParticipantId,
            const MessagingQos &qosSettings,
            IClientCache *cache,
            bool cached,
            qint64 reqCacheDataFreshness_ms
    ){
        Q_UNUSED(messageSender);
        Q_UNUSED(subscriptionManager);
        Q_UNUSED(domain);
        Q_UNUSED(interfaceName);
        Q_UNUSED(proxyParticipantId);
        Q_UNUSED(providerParticipantId);
        Q_UNUSED(qosSettings);
        Q_UNUSED(cache);
        Q_UNUSED(cached);
        Q_UNUSED(reqCacheDataFreshness_ms);
        notImplemented();
        return 0;
    }
    void notImplemented();
};

// Create a JoynrMessagingConnector for a generated interface
class JOYNR_EXPORT JoynrMessagingConnectorFactory {
public:
    JoynrMessagingConnectorFactory(IJoynrMessageSender* messageSender, SubscriptionManager* subscriptionManager);

    bool canBeCreated(const joynr::system::CommunicationMiddleware::Enum& connection);

    template <class T>
    T* create(
    		const QString &domain,
            const QString proxyParticipantId,
            const QString& providerParticipantId,
            const MessagingQos &qosSettings,
            IClientCache *cache,
            bool cached,
            qint64 reqCacheDataFreshness_ms
    ) {
        return JoynrMessagingConnectorFactoryHelper<T>().create(
        		messageSender,
        		subscriptionManager,
        		domain,
        		proxyParticipantId,
        		providerParticipantId,
        		qosSettings,
        		cache,
        		cached,
        		reqCacheDataFreshness_ms
        );
    }
private:
    IJoynrMessageSender* messageSender;
    SubscriptionManager* subscriptionManager;
};


} // namespace joynr
#endif //JOYNRMESSAGINGCONNECTORFACTORY_H
