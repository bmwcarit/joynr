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
#include "common/some-ip/SomeIpEndpointAddress.h"

namespace joynr {

const QString& SomeIpEndpointAddress::ENDPOINT_ADDRESS_TYPE() {
    static QString value("SomeIpEndpointAddress");
    return value;
}

SomeIpEndpointAddress::SomeIpEndpointAddress() :
    EndpointAddressBase(),
    ipAddress(),
    port(-1)
{
}

/*
IpMiddlewareEndpointAddress::IpMiddlewareEndpointAddress(){

}
*/
SomeIpEndpointAddress::~SomeIpEndpointAddress(){

}

SomeIpEndpointAddress &SomeIpEndpointAddress::operator =(const SomeIpEndpointAddress &other)
{
    EndpointAddressBase::operator =(other);
    ipAddress = other.getIpAddress();
    port = other.getPort();
    return *this;
}

bool SomeIpEndpointAddress::operator ==(const SomeIpEndpointAddress &other) const
{
    return EndpointAddressBase::operator ==(other)
            && ipAddress == other.getIpAddress()
            && port == other.getPort();
}

QString SomeIpEndpointAddress::getIpAddress() const
{
    return ipAddress;
}

int SomeIpEndpointAddress::getPort() const
{
    return port;
}

void SomeIpEndpointAddress::setIpAddress(const QString &ipAddress)
{
    this->ipAddress = ipAddress;
}

SomeIpEndpointAddress::SomeIpEndpointAddress(const QString &ipAddress, int port) :
    EndpointAddressBase(),
    ipAddress(ipAddress),
    port(port)
{
}

SomeIpEndpointAddress::SomeIpEndpointAddress(const SomeIpEndpointAddress &other) :
    EndpointAddressBase(other),
    ipAddress(other.getIpAddress()),
    port(other.getPort())
{
}

void SomeIpEndpointAddress::setPort(int port)
{
    this->port = port;
}


} // namespace joynr
