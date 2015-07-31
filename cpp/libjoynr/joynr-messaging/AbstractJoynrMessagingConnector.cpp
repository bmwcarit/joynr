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
#include "joynr/AbstractJoynrMessagingConnector.h"

#include "joynr/exceptions.h"
#include "joynr/JoynrMessageSender.h"

namespace joynr
{

using namespace joynr_logging;
Logger* AbstractJoynrMessagingConnector::logger =
        Logging::getInstance()->getLogger("AbstractJoynrMessagingConnector",
                                          "AbstractJoynrMessagingConnector");

AbstractJoynrMessagingConnector::AbstractJoynrMessagingConnector(
        IJoynrMessageSender* joynrMessageSender,
        ISubscriptionManager* subscriptionManager,
        const std::string& domain,
        const std::string& interfaceName,
        const std::string proxyParticipantId,
        const std::string& providerParticipantId,
        const MessagingQos& qosSettings,
        IClientCache* cache,
        bool cached)
        : joynrMessageSender(joynrMessageSender),
          subscriptionManager(subscriptionManager),
          domain(domain),
          interfaceName(interfaceName),
          proxyParticipantId(proxyParticipantId),
          providerParticipantId(providerParticipantId),
          qosSettings(qosSettings),
          cache(cache),
          cached(cached)
{
}

bool AbstractJoynrMessagingConnector::usesClusterController() const
{
    return true;
}

void AbstractJoynrMessagingConnector::operationRequest(QSharedPointer<IReplyCaller> replyCaller,
                                                       const Request& request)
{
    sendRequest(request, replyCaller);
}

void AbstractJoynrMessagingConnector::sendRequest(const Request& request,
                                                  QSharedPointer<IReplyCaller> replyCaller)
{
    joynrMessageSender->sendRequest(
            proxyParticipantId, providerParticipantId, qosSettings, request, replyCaller);
}

} // namespace joynr
