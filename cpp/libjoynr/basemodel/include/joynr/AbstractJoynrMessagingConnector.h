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
#ifndef ABSTRACTJOYNRMESSAGINGCONNECTOR_H
#define ABSTRACTJOYNRMESSAGINGCONNECTOR_H

#include <memory>
#include <string>

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/MessagingQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

class IMessageSender;
class IReplyCaller;
class ISubscriptionManager;
class OneWayRequest;
class Request;

class JOYNR_EXPORT AbstractJoynrMessagingConnector
{
public:
    AbstractJoynrMessagingConnector(
            std::weak_ptr<IMessageSender> messageSender,
            std::weak_ptr<ISubscriptionManager> subscriptionManager,
            const std::string& domain,
            const std::string& interfaceName,
            const std::string& proxyParticipantId,
            const MessagingQos& qosSettings,
            const types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry);

    virtual ~AbstractJoynrMessagingConnector() = default;

    /**
     * @brief Makes a request and returns the received response via the callback.
     *
     * @param replyCaller
     * @param request
     * @param qos
     */
    void operationRequest(std::shared_ptr<IReplyCaller> replyCaller,
                          Request&& request,
                          boost::optional<MessagingQos> qos = boost::none);
    void operationOneWayRequest(OneWayRequest&& request,
                                boost::optional<MessagingQos> qos = boost::none);

protected:
    std::weak_ptr<IMessageSender> _messageSender;
    std::weak_ptr<ISubscriptionManager> _subscriptionManager;
    std::string _domain;
    std::string _interfaceName;
    std::string _proxyParticipantId;
    std::string _providerParticipantId;
    MessagingQos _qosSettings;
    types::DiscoveryEntryWithMetaInfo _providerDiscoveryEntry;
    ADD_LOGGER(AbstractJoynrMessagingConnector)

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractJoynrMessagingConnector);
};

} // namespace joynr
#endif // ABSTRACTJOYNRMESSAGINGCONNECTOR_H
