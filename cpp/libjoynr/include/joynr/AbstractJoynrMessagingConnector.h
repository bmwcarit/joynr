/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <cassert>
#include <string>
#include <memory>
#include <boost/any.hpp>

#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/MessagingQos.h"
#include "joynr/Logger.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/IConnector.h"
#include "joynr/IReplyCaller.h"
#include "joynr/ReplyCaller.h"
#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class IJoynrMessageSender;
class ISubscriptionManager;

class JOYNR_EXPORT AbstractJoynrMessagingConnector : public IConnector
{
public:
    AbstractJoynrMessagingConnector(std::shared_ptr<IJoynrMessageSender> joynrMessageSender,
                                    std::shared_ptr<ISubscriptionManager> subscriptionManager,
                                    const std::string& domain,
                                    const std::string& interfaceName,
                                    const std::string& proxyParticipantId,
                                    const std::string& providerParticipantId,
                                    const MessagingQos& qosSettings);
    bool usesClusterController() const override;
    ~AbstractJoynrMessagingConnector() override = default;

    /**
     * @brief Makes a request and returns the received response via the callback.
     *
     * @param methodName
     * @param replyCaller

     */
    template <typename T>
    void attributeRequest(const std::string& methodName, std::shared_ptr<IReplyCaller> replyCaller)
    {
        std::string attributeID = domain + ":" + interfaceName + ":" + methodName;
        Request request;
        // explicitly set to no parameters
        request.setParams();
        request.setMethodName(methodName);
        sendRequest(request, replyCaller);
    }

    /**
     * @brief Makes a request and returns the received response via the callback.
     *
     * @param replyCaller
     * @param request
     */
    void operationRequest(std::shared_ptr<IReplyCaller> replyCaller, const Request& request);
    void operationOneWayRequest(const OneWayRequest& request);

protected:
    std::shared_ptr<IJoynrMessageSender> joynrMessageSender;
    std::shared_ptr<ISubscriptionManager> subscriptionManager;
    std::string domain;
    std::string interfaceName;
    std::string proxyParticipantId;
    std::string providerParticipantId;
    MessagingQos qosSettings;
    ADD_LOGGER(AbstractJoynrMessagingConnector);

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractJoynrMessagingConnector);

    // Request jsonRequest;
    void sendRequest(const Request& request, std::shared_ptr<IReplyCaller> replyCaller);
    void sendOneWayRequest(const OneWayRequest& request);
};

} // namespace joynr
#endif // ABSTRACTJOYNRMESSAGINGCONNECTOR_H
