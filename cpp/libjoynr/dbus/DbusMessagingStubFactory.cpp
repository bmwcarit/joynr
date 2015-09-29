/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#include "DbusMessagingStubFactory.h"
#include "joynr/system/RoutingTypes/QtCommonApiDbusAddress.h"
#include "common/dbus/DbusMessagingStubAdapter.h"

#include <QMutexLocker>

namespace joynr
{

DbusMessagingStubFactory::DbusMessagingStubFactory() : stubMap(), mutex()
{
}

bool DbusMessagingStubFactory::canCreate(const joynr::system::RoutingTypes::QtAddress& destAddress)
{
    return destAddress.inherits(
            system::RoutingTypes::QtCommonApiDbusAddress::staticMetaObject.className());
}

QSharedPointer<IMessaging> DbusMessagingStubFactory::create(
        const joynr::system::RoutingTypes::QtAddress& destAddress)
{
    const system::RoutingTypes::QtCommonApiDbusAddress* dbusAddress =
            dynamic_cast<const system::RoutingTypes::QtCommonApiDbusAddress*>(&destAddress);
    QString address = dbusAddress->getDomain() + ":" + dbusAddress->getServiceName() + ":" +
                      dbusAddress->getParticipantId();
    // lookup address
    {
        QMutexLocker locker(&mutex);
        if (!stubMap.contains(address)) {
            // create new stub
            auto stub = QSharedPointer<IMessaging>(new DbusMessagingStubAdapter(address));
            stubMap.insert(address, stub);
        }
    }
    return stubMap.value(address);
}

} // namespace joynr
