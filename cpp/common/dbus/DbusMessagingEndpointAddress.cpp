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
#include "common/dbus/DbusMessagingEndpointAddress.h"

namespace joynr {

const QString& DbusMessagingEndpointAddress::ENDPOINT_ADDRESS_TYPE() {
    static QString value("joynr::DbusMessagingEndpointAddress");
    return value;
}

DbusMessagingEndpointAddress::DbusMessagingEndpointAddress()
    : EndpointAddressBase(),
      serviceAddress()
{
}

DbusMessagingEndpointAddress::DbusMessagingEndpointAddress(const QString &serviceAddress)
    : EndpointAddressBase(),
      serviceAddress(serviceAddress)
{
}

DbusMessagingEndpointAddress::DbusMessagingEndpointAddress(const DbusMessagingEndpointAddress &other)
    : EndpointAddressBase(),
      serviceAddress(other.getServiceAddress())
{
}

DbusMessagingEndpointAddress::~DbusMessagingEndpointAddress() {
}

DbusMessagingEndpointAddress &DbusMessagingEndpointAddress::operator =(const DbusMessagingEndpointAddress &other)
{
    EndpointAddressBase::operator =(other);
    serviceAddress = other.getServiceAddress();
    return *this;
}

bool DbusMessagingEndpointAddress::operator ==(const DbusMessagingEndpointAddress &other) const
{
    return EndpointAddressBase::operator ==(other)
            && serviceAddress == other.getServiceAddress();
}

QString DbusMessagingEndpointAddress::getServiceAddress() const {
    return serviceAddress;
}

void DbusMessagingEndpointAddress::setServiceAddress(const QString &serviceAddress) {
    this->serviceAddress = serviceAddress;
}


} // namespace joynr
