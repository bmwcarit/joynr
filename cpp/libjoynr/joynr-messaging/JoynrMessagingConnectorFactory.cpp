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

#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/system/Address.h"
#include "joynr/JoynrMessagingEndpointAddress.h"
#include "joynr/JoynrMessagingViaCCEndpointAddress.h"

#include <QSharedPointer>

namespace joynr {

JoynrMessagingConnectorFactory::JoynrMessagingConnectorFactory(IJoynrMessageSender* messageSender,
                                                             SubscriptionManager* subscriptionManager) :
        messageSender(messageSender),
        subscriptionManager(subscriptionManager)
{
}


bool JoynrMessagingConnectorFactory::canBeCreated(QSharedPointer<joynr::system::Address> endpointAddress)
{
    QString endpointClassName = endpointAddress->metaObject()->className();
    //for creating the connector, no information is needed, so JoynrMessagingEndPointAddress and JoynrMessagingViaCCEndpointAddress should both work.
    return (endpointClassName == JoynrMessagingEndpointAddress::ENDPOINT_ADDRESS_TYPE()) ||
            (endpointClassName == JoynrMessagingViaCCEndpointAddress::ENDPOINT_ADDRESS_TYPE());
}

} // namespace joynr
