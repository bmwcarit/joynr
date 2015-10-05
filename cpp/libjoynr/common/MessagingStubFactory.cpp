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
#include "joynr/MessagingStubFactory.h"

#include "joynr/IMessaging.h"

#include <cassert>
#include <QMutexLocker>

namespace joynr
{

MessagingStubFactory::~MessagingStubFactory()
{
    while (!factoryList.isEmpty()) {
        delete factoryList.takeFirst();
    }
}

MessagingStubFactory::MessagingStubFactory()
        : address2MessagingStubDirectory("MessagingStubFactory-MessagingStubDirectory"),
          factoryList(),
          mutex()
{
}

QSharedPointer<IMessaging> MessagingStubFactory::create(
        const joynr::system::RoutingTypes::QtAddress& destinationAddress)
{
    {
        QMutexLocker locker(&this->mutex);

        if (!address2MessagingStubDirectory.contains(destinationAddress)) {
            // search for the corresponding factory
            for (QList<IMiddlewareMessagingStubFactory*>::iterator it = this->factoryList.begin();
                 it != factoryList.end();
                 ++it) {
                if ((*it)->canCreate(destinationAddress)) {
                    QSharedPointer<IMessaging> stub = (*it)->create(destinationAddress);
                    address2MessagingStubDirectory.add(destinationAddress, stub);

                    assert(!stub.isNull());
                    return stub;
                }
            }
        }
    }

    return address2MessagingStubDirectory.lookup(destinationAddress);
}

void MessagingStubFactory::remove(const joynr::system::RoutingTypes::QtAddress& destinationAddress)
{
    address2MessagingStubDirectory.remove(destinationAddress);
}

bool MessagingStubFactory::contains(
        const joynr::system::RoutingTypes::QtAddress& destinationAddress)
{
    return address2MessagingStubDirectory.contains(destinationAddress);
}

void MessagingStubFactory::registerStubFactory(IMiddlewareMessagingStubFactory* factory)
{
    this->factoryList.append(factory);
}

} // namespace joynr
