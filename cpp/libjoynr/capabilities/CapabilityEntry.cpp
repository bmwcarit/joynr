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
#include "joynr/JoynrMessagingEndpointAddress.h"
#include "libjoynr/some-ip/SomeIpEndpointAddress.h"

namespace joynr {

CapabilityEntry::CapabilityEntry() :
    domain(),
    interfaceName(),
    qos(),
    participantId(),
    endpointAddresses(),
    global(true)
{
}

CapabilityEntry::CapabilityEntry(const CapabilityEntry &other)
    : QObject(),
      domain(other.domain),
      interfaceName(other.interfaceName),
      qos(other.qos),
      participantId(other.participantId),
      endpointAddresses(other.endpointAddresses),
      global(other.global)
{
}

CapabilityEntry::CapabilityEntry(const QString& domain, const QString& interfaceName, joynr::types::ProviderQos qos, const QString& participantId, QList<QSharedPointer<joynr::system::Address> > endpointAddresses, bool isGlobal, QObject *parent)
    : QObject(parent),
      domain(domain),
      interfaceName(interfaceName),
      qos(qos),
      participantId(participantId),
      endpointAddresses(endpointAddresses),
      global(isGlobal)
{
}

CapabilityEntry& CapabilityEntry::operator =(const CapabilityEntry & other) {
    this->interfaceName = other.interfaceName;
    this->domain = other.domain;
    this->qos = other.qos;
    this->participantId = other.participantId;
    this->endpointAddresses = endpointAddresses;
    this->global = other.global;
    return *this;
}

bool CapabilityEntry::operator ==(const CapabilityEntry& other) const  {
    bool result = this->interfaceName == other.interfaceName
            && this->domain == other.domain
            && this->participantId == other.participantId
            && this->endpointAddresses.size() == other.endpointAddresses.size()
            && this->global == other.global;

    if(!result) {
        return false;
    }

    // check endpoint addresses
    for(int index = 0; index < this->endpointAddresses.size(); index++) {
        joynr::system::Address myAddr = *this->endpointAddresses.at(index).data();
        joynr::system::Address otherAddr = *other.endpointAddresses.at(index).data();

        if(!(myAddr == otherAddr)) {
            return false;
        }
    }

    return true;
}


QString CapabilityEntry::getInterfaceName() const{
    return interfaceName;
}

QString CapabilityEntry::getDomain() const{
    return domain;
}

void CapabilityEntry::setInterfaceName(QString interfaceName) {
    this->interfaceName = interfaceName;
}

void CapabilityEntry::setDomain(QString domain){
    this->domain = domain;
}

joynr::types::ProviderQos CapabilityEntry::getQos() const{
    return qos;
}

void CapabilityEntry::setQos(joynr::types::ProviderQos qos) {
    this->qos = qos;
}

void CapabilityEntry::setParticipantId(QString participantId){
    this->participantId = participantId;
}

QString CapabilityEntry::getParticipantId() const{
    return participantId;
}

void CapabilityEntry::setEndpointAddresses(QList<QSharedPointer<joynr::system::Address> > endpointAddresses){
    this->endpointAddresses = endpointAddresses;
}

QList<QSharedPointer<joynr::system::Address> > CapabilityEntry::getEndpointAddresses() const{
    return endpointAddresses;
}

void CapabilityEntry::prependEndpointAddress(QSharedPointer<joynr::system::Address> endpointAddress){
    endpointAddresses.prepend(endpointAddress);
}


bool CapabilityEntry::isGlobal() const{
    return global;
}

void CapabilityEntry::setGlobal(bool global)
{
    this->global = global;
}

QString CapabilityEntry::toString() const
{
    return QString("{ domain: "+domain+", interfaceName: "+interfaceName+", participantId: "+participantId+"}");
}

} // namespace joynr
