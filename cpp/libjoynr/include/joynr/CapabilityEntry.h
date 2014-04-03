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
#ifndef CAPABILITYENTRY_H
#define CAPABILITYENTRY_H

/*
 * This is an internal datatype for the storage of capability details
 * for the capability directory.  This class contains attributes that
 * are necessary for determining whether to use the cache or not.
 */

#include "joynr/JoynrExport.h"

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QDataStream>
#include "joynr/system/Address.h"
#include "joynr/types/ProviderQos.h"


namespace joynr {

class JOYNR_EXPORT CapabilityEntry : QObject
{
    Q_OBJECT
    Q_PROPERTY(QString interface READ getInterfaceName WRITE setInterfaceName)
    Q_PROPERTY(QString domain READ getDomain WRITE setDomain)
    Q_PROPERTY(joynr__types__ProviderQos qos READ getQos WRITE setQos)
    Q_PROPERTY(QString participantId READ getParticipantId WRITE setParticipantId)
    Q_PROPERTY(QList<QSharedPointer<joynr::system::Address> > endpointAddresses READ getEndpointAddresses WRITE setEndpointAddresses)
    Q_PROPERTY(bool global READ isGlobal)

public:

    CapabilityEntry();

    CapabilityEntry(const CapabilityEntry &other);

    CapabilityEntry(
            const QString& domain,
            const QString& interfaceName,
            joynr::types::ProviderQos qos,
            const QString& participantId,
            QList<QSharedPointer<joynr::system::Address> > endpointAddresses,
            bool isGlobal,
            QObject* parent = 0
    );

    CapabilityEntry& operator =(const CapabilityEntry& other);

    bool operator ==(const CapabilityEntry& other) const;

    QString getInterfaceName() const;
    QString getDomain() const;
    void setInterfaceName(QString interfaceName);
    void setDomain(QString domain);

    types::ProviderQos getQos() const;
    void setQos(joynr::types::ProviderQos qos);

    QString getParticipantId() const;
    void setParticipantId(QString participantId);


    void setEndpointAddresses(QList<QSharedPointer<joynr::system::Address> > endpointAddresses);
    QList<QSharedPointer<joynr::system::Address> > getEndpointAddresses() const;

    void prependEndpointAddress(QSharedPointer<joynr::system::Address> endpointAddress);

    bool isGlobal() const;
    void setGlobal(bool global);

    QString toString() const;

 private:
    QString domain;
    QString interfaceName;
    types::ProviderQos qos;
    QString participantId;
    QList<QSharedPointer<joynr::system::Address> > endpointAddresses;
    bool global;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::CapabilityEntry)

#endif //CAPABILITYENTRY_H
