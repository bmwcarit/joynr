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
#ifndef INTERFACEADDRESS_H_
#define INTERFACEADDRESS_H_

#include <cstddef>
#include <string>

#include "joynr/JoynrExport.h"

namespace joynr
{

/**
  * Immutable data class for messaging interface addresses.
  */
class JOYNR_EXPORT InterfaceAddress
{
public:
    InterfaceAddress();
    explicit InterfaceAddress(const std::string& domain, const std::string& interfaceName);
    const std::string& getDomain() const;
    const std::string& getInterface() const;

    bool operator==(const InterfaceAddress& interfaceAddress) const;
    bool operator<(const InterfaceAddress& interfaceAddress) const;

    /**
     * @brief Returns a hash code value for this object
     * @return a hash code value for this object.
     */
    std::size_t hashCode() const;

private:
    std::string _domain;
    std::string _interfaceName;
};

std::size_t hash_value(const InterfaceAddress& tInterfaceAddress);

} // namespace joynr

namespace std
{

/**
 * @brief Function object that implements a hash function for joynr::InterfaceAddress.
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template <>
struct hash<joynr::InterfaceAddress>
{

    /**
     * @brief method overriding default implementation of operator ()
     * @param tStructValue the operators argument
     * @return the ordinal number representing the enum value
     */
    std::size_t operator()(const joynr::InterfaceAddress& tInterfaceAddress) const
    {
        return joynr::hash_value(tInterfaceAddress);
    }
};
} // namespace std
#endif // INTERFACEADDRESS_H_
