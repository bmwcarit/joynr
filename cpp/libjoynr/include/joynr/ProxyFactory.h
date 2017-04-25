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

#ifndef PROXYFACTORY_H
#define PROXYFACTORY_H

#include <memory>
#include <string>

#include "joynr/ConnectorFactory.h"
#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class MessagingQos;

class JOYNR_EXPORT ProxyFactory
{
public:
    ProxyFactory(
            std::shared_ptr<const joynr::system::RoutingTypes::Address> messagingEndpointAddress,
            std::unique_ptr<ConnectorFactory> connectorFactory);

    // Create a proxy of type T
    template <class T>
    T* createProxy(const std::string& domain, const MessagingQos& qosSettings)
    {
        return new T(messagingEndpointAddress, connectorFactory.get(), domain, qosSettings);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ProxyFactory);
    std::shared_ptr<const joynr::system::RoutingTypes::Address> messagingEndpointAddress;
    std::unique_ptr<ConnectorFactory> connectorFactory;
};

} // namespace joynr
#endif // PROXYFACTORY_H
