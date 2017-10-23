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
#ifndef CONNECTORFACTORY_H
#define CONNECTORFACTORY_H

#include <memory>
#include <string>

#include "joynr/InProcessConnectorFactory.h"
#include "joynr/JoynrExport.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/Logger.h"
#include "joynr/MessagingQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

class InProcessDispatcher;

class JOYNR_EXPORT ConnectorFactory
{
public:
    ConnectorFactory(
            std::shared_ptr<InProcessConnectorFactory> inProcessConnectorFactory,
            std::unique_ptr<JoynrMessagingConnectorFactory> joynrMessagingConnectorFactory);

    virtual ~ConnectorFactory() = default;

    template <class T>
    std::unique_ptr<T> create(const std::string& domain,
                              const std::string proxyParticipantId,
                              const MessagingQos& qosSettings,
                              bool createInProcessConnector,
                              const types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry)
    {
        if (createInProcessConnector) {
            return inProcessConnectorFactory->create<T>(
                    proxyParticipantId, providerDiscoveryEntry.getParticipantId());
        } else {
            return joynrMessagingConnectorFactory->create<T>(
                    domain, proxyParticipantId, qosSettings, providerDiscoveryEntry);
        }
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ConnectorFactory);
    std::shared_ptr<InProcessConnectorFactory> inProcessConnectorFactory;
    std::unique_ptr<JoynrMessagingConnectorFactory> joynrMessagingConnectorFactory;
    ADD_LOGGER(ConnectorFactory)
};

} // namespace joynr
#endif // CONNECTORFACTORY_H
