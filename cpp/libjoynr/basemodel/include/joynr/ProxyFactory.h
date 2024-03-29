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

#include "joynr/JoynrExport.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class MessagingQos;
class JoynrRuntimeImpl;

class JOYNR_EXPORT ProxyFactory
{
public:
    ProxyFactory(std::shared_ptr<JoynrMessagingConnectorFactory> connectorFactory);

    // Create a proxy of type T
    template <class T>
    std::shared_ptr<T> createProxy(std::shared_ptr<JoynrRuntimeImpl> runtime,
                                   const std::string& domain,
                                   const MessagingQos& qosSettings)
    {
        auto proxy = std::make_shared<T>(runtime, _connectorFactory, domain, qosSettings);
        return proxy;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ProxyFactory);
    std::shared_ptr<JoynrMessagingConnectorFactory> _connectorFactory;
};

} // namespace joynr
#endif // PROXYFACTORY_H
