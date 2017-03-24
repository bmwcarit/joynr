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

#include "joynr/JoynrMessageSender.h"

namespace joynr
{

INIT_LOGGER(AbstractJoynrMessagingConnector);

AbstractJoynrMessagingConnector::AbstractJoynrMessagingConnector(
        std::shared_ptr<IJoynrMessageSender> joynrMessageSender,
        std::shared_ptr<ISubscriptionManager> subscriptionManager,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string& proxyParticipantId,
        const MessagingQos& qosSettings,
        const types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry)
        : joynrMessageSender(joynrMessageSender),
          subscriptionManager(subscriptionManager),
          domain(domain),
          interfaceName(interfaceName),
          proxyParticipantId(proxyParticipantId),
          providerParticipantId(providerDiscoveryEntry.getParticipantId()),
          qosSettings(qosSettings),
          providerDiscoveryEntry(providerDiscoveryEntry)
{
}

bool AbstractJoynrMessagingConnector::usesClusterController() const
{
    return true;
}

void AbstractJoynrMessagingConnector::operationRequest(std::shared_ptr<IReplyCaller> replyCaller,
                                                       const Request& request)
{
    sendRequest(request, replyCaller);
}

void AbstractJoynrMessagingConnector::operationOneWayRequest(const OneWayRequest& request)
{
    sendOneWayRequest(request);
}

void AbstractJoynrMessagingConnector::sendRequest(const Request& request,
                                                  std::shared_ptr<IReplyCaller> replyCaller)
{
    joynrMessageSender->sendRequest(proxyParticipantId,
                                    providerParticipantId,
                                    qosSettings,
                                    request,
                                    replyCaller,
                                    providerDiscoveryEntry.getIsLocal());
}

void AbstractJoynrMessagingConnector::sendOneWayRequest(const OneWayRequest& request)
{
    joynrMessageSender->sendOneWayRequest(proxyParticipantId,
                                          providerParticipantId,
                                          qosSettings,
                                          request,
                                          providerDiscoveryEntry.getIsLocal());
}
} // namespace joynr
