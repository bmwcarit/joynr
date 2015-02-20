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
#include "joynr/CapabilityEntry.h"
#include "joynr/JsonSerializer.h"

namespace joynr
{

CapabilityEntry::CapabilityEntry()
        : domain(), interfaceName(), qos(), participantId(), middlewareConnections(), global(true)
{
}

CapabilityEntry::CapabilityEntry(const CapabilityEntry& other)
        : QObject(),
          domain(other.domain),
          interfaceName(other.interfaceName),
          qos(other.qos),
          participantId(other.participantId),
          middlewareConnections(other.middlewareConnections),
          global(other.global)
{
}

CapabilityEntry::CapabilityEntry(
        const QString& domain,
        const QString& interfaceName,
        joynr::types::ProviderQos qos,
        const QString& participantId,
        QList<joynr::system::CommunicationMiddleware::Enum> middlewareConnections,
        bool isGlobal,
        QObject* parent)
        : QObject(parent),
          domain(domain),
          interfaceName(interfaceName),
          qos(qos),
          participantId(participantId),
          middlewareConnections(middlewareConnections),
          global(isGlobal)
{
}

CapabilityEntry& CapabilityEntry::operator=(const CapabilityEntry& other)
{
    this->interfaceName = other.interfaceName;
    this->domain = other.domain;
    this->qos = other.qos;
    this->participantId = other.participantId;
    this->middlewareConnections = middlewareConnections;
    this->global = other.global;
    return *this;
}

bool CapabilityEntry::operator==(const CapabilityEntry& other) const
{
    return this->interfaceName == other.interfaceName && this->domain == other.domain &&
           this->participantId == other.participantId &&
           this->middlewareConnections == other.middlewareConnections &&
           this->global == other.global;
}

QString CapabilityEntry::getInterfaceName() const
{
    return interfaceName;
}

QString CapabilityEntry::getDomain() const
{
    return domain;
}

void CapabilityEntry::setInterfaceName(QString interfaceName)
{
    this->interfaceName = interfaceName;
}

void CapabilityEntry::setDomain(QString domain)
{
    this->domain = domain;
}

joynr::types::ProviderQos CapabilityEntry::getQos() const
{
    return qos;
}

void CapabilityEntry::setQos(joynr::types::ProviderQos qos)
{
    this->qos = qos;
}

void CapabilityEntry::setParticipantId(QString participantId)
{
    this->participantId = participantId;
}

QString CapabilityEntry::getParticipantId() const
{
    return participantId;
}

void CapabilityEntry::setMiddlewareConnections(
        QList<joynr::system::CommunicationMiddleware::Enum> middlewareConnections)
{
    this->middlewareConnections = middlewareConnections;
}

QList<joynr::system::CommunicationMiddleware::Enum> CapabilityEntry::getMiddlewareConnections()
        const
{
    return middlewareConnections;
}

void CapabilityEntry::prependMiddlewareConnection(
        joynr::system::CommunicationMiddleware::Enum middlewareConnection)
{
    middlewareConnections.prepend(middlewareConnection);
}

bool CapabilityEntry::isGlobal() const
{
    return global;
}

void CapabilityEntry::setGlobal(bool global)
{
    this->global = global;
}

QString CapabilityEntry::toString() const
{
    return QString("{ domain: " + domain + ", interfaceName: " + interfaceName +
                   ", participantId: " + participantId + "}");
}

} // namespace joynr
