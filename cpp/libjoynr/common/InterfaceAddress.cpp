/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "joynr/InterfaceAddress.h"

#include <boost/functional/hash.hpp>

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

bool InterfaceAddress::operator<(const InterfaceAddress& interfaceAddress) const
{
    if (domain == interfaceAddress.domain) {
        return interfaceName > interfaceAddress.getInterface();
    }
    return domain < interfaceAddress.getDomain();
}

std::size_t InterfaceAddress::hashCode() const
{
    std::size_t seed = 0;

    boost::hash_combine(seed, getDomain());
    boost::hash_combine(seed, getInterface());

    return seed;
}

std::size_t hash_value(const InterfaceAddress& tInterfaceAddress)
{
    return tInterfaceAddress.hashCode();
}

} // namespace joynr
