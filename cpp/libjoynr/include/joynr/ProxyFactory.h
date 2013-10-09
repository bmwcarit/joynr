/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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

#ifndef PROXYFACTORY_H
#define PROXYFACTORY_H
	

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"

#include "joynr/ICapabilities.h"
#include "joynr/ConnectorFactory.h"
#include "joynr/ProxyQos.h"

#include <QString>
#include <QSharedPointer>

namespace joynr {

class IClientCache;
class MessagingQos;
class JoynrMessageSender;

class JOYNR_EXPORT ProxyFactory {
public:
    ProxyFactory(ICapabilities* capabilitiesStub,
                 QSharedPointer<EndpointAddressBase> messagingEndpointAddress,
                 ConnectorFactory* connectorFactory,
                 IClientCache* cache);

    ~ProxyFactory();

    // Create a proxy of type T
    template<class T>
    T* createProxy(const QString& domain,const ProxyQos& proxyQos, const MessagingQos& qosSettings, bool cached) {
        return new T(capabilitiesStub, messagingEndpointAddress, connectorFactory, cache, domain, proxyQos, qosSettings, cached);
    }

private:
	DISALLOW_COPY_AND_ASSIGN(ProxyFactory);
	ICapabilities* capabilitiesStub;
	QSharedPointer<EndpointAddressBase> messagingEndpointAddress; 		
    ConnectorFactory* connectorFactory;
    IClientCache* cache;
};



} // namespace joynr
#endif //PROXYFACTORY_H
