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
#ifndef INPROCESSCONNECTORFACTORY_H
#define INPROCESSCONNECTORFACTORY_H

#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/InProcessAddress.h"
#include "joynr/types/QtCommunicationMiddleware.h"
#include "joynr/IRequestCallerDirectory.h"

#include <string>
#include <memory>

namespace joynr
{

class InProcessPublicationSender;
class ISubscriptionManager;
class PublicationManager;

// Default implementation for the InProcessConnectorFactoryHelper
// Template specializations are provided in *InProcessConnector.h
template <class T>
class InProcessConnectorFactoryHelper
{
public:
    T* create(ISubscriptionManager* subscriptionManager,
              PublicationManager* publicationManager,
              const std::string& proxyParticipantId,
              const std::string& providerParticipantId,
              std::shared_ptr<InProcessAddress> address)
    {
        Q_UNUSED(subscriptionManager);
        Q_UNUSED(publicationManager);
        Q_UNUSED(proxyParticipantId);
        Q_UNUSED(providerParticipantId);
        Q_UNUSED(address);
        notImplemented();
        return 0;
    }
    void notImplemented();
};

// A factory that creates an InProcessConnector for a generated interface
class JOYNR_EXPORT InProcessConnectorFactory
{
public:
    InProcessConnectorFactory(ISubscriptionManager* subscriptionManager,
                              PublicationManager* publicationManager,
                              InProcessPublicationSender* inProcessPublicationSender,
                              IRequestCallerDirectory* requestCallerDirectory);

    bool canBeCreated(const joynr::types::CommunicationMiddleware::Enum& connection);
    virtual ~InProcessConnectorFactory()
    {
    }

    template <class T>
    T* create(const std::string& proxyParticipantId, const std::string& providerParticipantId)
    {
        std::shared_ptr<RequestCaller> requestCaller =
                requestCallerDirectory->lookupRequestCaller(providerParticipantId);
        std::shared_ptr<InProcessAddress> inProcessEndpointAddress(
                new InProcessAddress(requestCaller));

        return InProcessConnectorFactoryHelper<T>().create(subscriptionManager,
                                                           publicationManager,
                                                           inProcessPublicationSender,
                                                           proxyParticipantId,
                                                           providerParticipantId,
                                                           inProcessEndpointAddress);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(InProcessConnectorFactory);
    ISubscriptionManager* subscriptionManager;
    PublicationManager* publicationManager;
    InProcessPublicationSender* inProcessPublicationSender;
    IRequestCallerDirectory* requestCallerDirectory;
};

} // namespace joynr
#endif // INPROCESSCONNECTORFACTORY_H
