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
#include "joynr/AbstractJoynrMessagingConnector.h"

#include "joynr/IMessageSender.h"

namespace joynr
{

AbstractJoynrMessagingConnector::AbstractJoynrMessagingConnector(
        std::weak_ptr<IMessageSender> messageSender,
        std::weak_ptr<ISubscriptionManager> subscriptionManager,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& proxyParticipantId,
        const MessagingQos& qosSettings,
        const types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry)
        : messageSender(messageSender),
          subscriptionManager(subscriptionManager),
          domain(domain),
          interfaceName(interfaceName),
          proxyParticipantId(proxyParticipantId),
          providerParticipantId(providerDiscoveryEntry.getParticipantId()),
          qosSettings(qosSettings),
          providerDiscoveryEntry(providerDiscoveryEntry)
{
}

void AbstractJoynrMessagingConnector::operationRequest(std::shared_ptr<IReplyCaller> replyCaller,
                                                       Request&& request,
                                                       boost::optional<MessagingQos> qos)
{
    if (auto ptr = messageSender.lock()) {
        ptr->sendRequest(proxyParticipantId,
                         providerParticipantId,
                         qos ? *qos : qosSettings,
                         request,
                         std::move(replyCaller),
                         providerDiscoveryEntry.getIsLocal());
    }
}

void AbstractJoynrMessagingConnector::operationOneWayRequest(OneWayRequest&& request,
                                                             boost::optional<MessagingQos> qos)
{
    if (auto ptr = messageSender.lock()) {
        ptr->sendOneWayRequest(proxyParticipantId,
                               providerParticipantId,
                               qos ? *qos : qosSettings,
                               request,
                               providerDiscoveryEntry.getIsLocal());
    }
}

} // namespace joynr