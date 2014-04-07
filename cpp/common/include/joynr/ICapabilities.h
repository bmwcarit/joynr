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
#ifndef ICAPABILITIES_H
#define ICAPABILITIES_H

#include "joynr/MessagingQos.h"
#include "joynr/system/Address.h"
#include "joynr/CapabilityEntry.h"
#include "joynr/DiscoveryQos.h"
#include <QString>
#include <QSharedPointer>

namespace joynr {

/**
  * Interface for capabilities lookup and registration. Used by the CapabilitiesRegistrar and the arbitrator classes.
  *
  *
  */

class ICapabilities {
public:
    virtual ~ICapabilities(){ }

    static int& NO_TIMEOUT(){
        static int no_timeout = -1;
        return no_timeout;
    }

    //currently timeout is only important for lookups, because they might depend on an synchronous answer
    //from the CapabilitiesClient. But once we use dbus instead of inprocess, timeouts might be meaningfull for
    //other methods as well.

    virtual void add(
            const QString& domain,
            const QString& interfaceName,
            const QString& participantId,
            const types::ProviderQos& qos,
            QList<QSharedPointer<joynr::system::Address> > endpointAddressList,
            QSharedPointer<joynr::system::Address> messagingStubAddress,
            const qint64& timeout_ms
    ) = 0;
    virtual void addEndpoint(
            const QString& participantId,
            QSharedPointer<joynr::system::Address> messagingStubAddress,
            const qint64& timeout_ms
    ) = 0;
    virtual QList<CapabilityEntry> lookup(
            const QString& domain,
            const QString& interfaceName,
            const DiscoveryQos& discoveryQos
    ) = 0;
    virtual QList<CapabilityEntry> lookup(
            const QString& participantId,
            const DiscoveryQos& discoveryQos
    ) = 0;
    virtual void remove(const QString& participantId, const qint64& timeout_ms) = 0;
};


} // namespace joynr
#endif //ICAPABILITIES_H
