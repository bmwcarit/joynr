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
#ifndef JOYNRMESSAGINGCONNECTORFACTORY_H
#define JOYNRMESSAGINGCONNECTORFACTORY_H

#include "joynr/JoynrExport.h"

#include <memory>
#include <string>

namespace joynr
{

class IMessageSender;
class ISubscriptionManager;
class MessagingQos;

namespace types
{
class DiscoveryEntryWithMetaInfo;
} // namespace types

// traits class which is specialized for every Interface
// this links Interface with the respective Connector
template <typename Interface>
struct JoynrMessagingTraits;

// Create a JoynrMessagingConnector for a generated interface
class JOYNR_EXPORT JoynrMessagingConnectorFactory
{
public:
    JoynrMessagingConnectorFactory(std::shared_ptr<IMessageSender> messageSender,
                                   std::shared_ptr<ISubscriptionManager> subscriptionManager);

    template <class T>
    std::unique_ptr<T> create(const std::string& domain,
                              const std::string proxyParticipantId,
                              const MessagingQos& qosSettings,
                              const types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry)
    {
        using Connector = typename JoynrMessagingTraits<T>::Connector;
        return std::make_unique<Connector>(_messageSender,
                                           _subscriptionManager,
                                           domain,
                                           proxyParticipantId,
                                           qosSettings,
                                           providerDiscoveryEntry);
    }

private:
    std::shared_ptr<IMessageSender> _messageSender;
    std::shared_ptr<ISubscriptionManager> _subscriptionManager;
};

} // namespace joynr
#endif // JOYNRMESSAGINGCONNECTORFACTORY_H
