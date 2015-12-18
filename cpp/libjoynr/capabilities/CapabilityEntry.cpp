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
        const std::string& domain,
        const std::string& interfaceName,
        joynr::types::ProviderQos qos,
        const std::string& participantId,
        std::vector<joynr::types::CommunicationMiddleware::Enum> middlewareConnections,
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

std::string CapabilityEntry::getInterfaceName() const
{
    return interfaceName;
}

std::string CapabilityEntry::getDomain() const
{
    return domain;
}

void CapabilityEntry::setInterfaceName(std::string interfaceName)
{
    this->interfaceName = interfaceName;
}

void CapabilityEntry::setDomain(std::string domain)
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

void CapabilityEntry::setParticipantId(std::string participantId)
{
    this->participantId = participantId;
}

std::string CapabilityEntry::getParticipantId() const
{
    return participantId;
}

void CapabilityEntry::setMiddlewareConnections(
        std::vector<joynr::types::CommunicationMiddleware::Enum> middlewareConnections)
{
    this->middlewareConnections = middlewareConnections;
}

std::vector<joynr::types::CommunicationMiddleware::Enum> CapabilityEntry::getMiddlewareConnections()
        const
{
    return middlewareConnections;
}

void CapabilityEntry::prependMiddlewareConnection(
        joynr::types::CommunicationMiddleware::Enum middlewareConnection)
{
    middlewareConnections.insert(middlewareConnections.begin(), middlewareConnection);
}

bool CapabilityEntry::isGlobal() const
{
    return global;
}

void CapabilityEntry::setGlobal(bool global)
{
    this->global = global;
}

std::string CapabilityEntry::toString() const
{
    return std::string("{ domain: " + domain + ", interfaceName: " + interfaceName +
                       ", participantId: " + participantId + "}");
}

} // namespace joynr
