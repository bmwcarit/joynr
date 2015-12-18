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
#ifndef ABSTRACTJOYNRMESSAGINGCONNECTOR_H
#define ABSTRACTJOYNRMESSAGINGCONNECTOR_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/MessagingQos.h"
#include "joynr/joynrlogging.h"
#include "joynr/ArbitrationStatus.h"
#include "joynr/IArbitrationListener.h"
#include "joynr/IClientCache.h"
#include "joynr/RequestStatus.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/IConnector.h"
#include "joynr/IReplyCaller.h"
#include "joynr/ReplyCaller.h"
#include "joynr/JoynrExport.h"
#include <string>
#include <memory>

namespace joynr
{

class IJoynrMessageSender;
class ISubscriptionManager;

class JOYNR_EXPORT AbstractJoynrMessagingConnector : public IConnector
{
public:
    AbstractJoynrMessagingConnector(IJoynrMessageSender* joynrMessageSender,
                                    ISubscriptionManager* subscriptionManager,
                                    const std::string& domain,
                                    const std::string& interfaceName,
                                    const std::string proxyParticipantId,
                                    const std::string& providerParticipantId,
                                    const MessagingQos& qosSettings,
                                    IClientCache* cache,
                                    bool cached);
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

        if (cached) {
            Variant entry = cache->lookUp(attributeID);
            if (entry.isEmpty()) {
                LOG_DEBUG(logger,
                          FormatString("Cached value for %1 is not valid").arg(methodName).str());
            } else if (!entry.is<T>()) {
                LOG_DEBUG(logger,
                          FormatString("Cached value for %1 cannot be converted to type T")
                                  .arg(methodName)
                                  .str());
                assert(false);
            } else {
                LOG_DEBUG(
                        logger,
                        FormatString("Returning cached value for method %1").arg(methodName).str());
                std::shared_ptr<ReplyCaller<T>> typedReplyCaller =
                        std::dynamic_pointer_cast<ReplyCaller<T>>(replyCaller);
                typedReplyCaller->returnValue(entry.get<T>());
            }
        } else {
            Request request;
            request.setMethodName(methodName);
            sendRequest(request, replyCaller);
            // TODO the retrieved values are never stored into the cache.
        }
    }

    /**
     * @brief Makes a request and returns the received response via the callback.
     *
     * @param replyCaller
     * @param request
     */
    void operationRequest(std::shared_ptr<IReplyCaller> replyCaller, const Request& request);

protected:
    IJoynrMessageSender* joynrMessageSender;
    ISubscriptionManager* subscriptionManager;
    std::string domain;
    std::string interfaceName;
    std::string proxyParticipantId;
    std::string providerParticipantId;
    MessagingQos qosSettings;
    IClientCache* cache;
    bool cached;
    static joynr_logging::Logger* logger;

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractJoynrMessagingConnector);

    // Request jsonRequest;
    void sendRequest(const Request& request, std::shared_ptr<IReplyCaller> replyCaller);
};

} // namespace joynr
#endif // ABSTRACTJOYNRMESSAGINGCONNECTOR_H
