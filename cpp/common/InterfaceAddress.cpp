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
#include "common/InterfaceAddress.h"
#include <QString>

namespace joynr
{

InterfaceAddress::InterfaceAddress() : domain(), interfaceName()
{
}

InterfaceAddress::InterfaceAddress(const std::string& domain, const std::string& interfaceName)
        : domain(domain), interfaceName(interfaceName)
{
}

const std::string& InterfaceAddress::getDomain() const
{
    return domain;
}

const std::string& InterfaceAddress::getInterface() const
{
    return interfaceName;
}

bool InterfaceAddress::operator==(const InterfaceAddress& interfaceAddress) const
{
    return ((domain == interfaceAddress.domain) &&
            (interfaceName == interfaceAddress.interfaceName));
}

uint qHash(const InterfaceAddress& interfaceAddress)
{
    return qHash(QString::fromStdString(interfaceAddress.getDomain())) * 31 +
           qHash(QString::fromStdString(interfaceAddress.getInterface()));
}

} // namespace joynr
