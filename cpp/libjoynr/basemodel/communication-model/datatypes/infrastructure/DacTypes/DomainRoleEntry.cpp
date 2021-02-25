/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

#include <sstream>
#include <string>

#include "joynr/HashUtil.h"
#include "joynr/infrastructure/DacTypes/DomainRoleEntry.h"
#include <boost/functional/hash.hpp>

namespace joynr
{
namespace infrastructure
{
namespace DacTypes
{

const std::int32_t DomainRoleEntry::MAJOR_VERSION = 0;
const std::int32_t DomainRoleEntry::MINOR_VERSION = 0;

DomainRoleEntry::DomainRoleEntry()
        : uid(""), domains(), role(joynr::infrastructure::DacTypes::Role::MASTER)
{
}

DomainRoleEntry::DomainRoleEntry(const std::string& _uid,
                                 const std::vector<std::string>& _domains,
                                 const joynr::infrastructure::DacTypes::Role::Enum& _role)
        : uid(_uid), domains(_domains), role(_role)
{
}

std::size_t DomainRoleEntry::hashCode() const
{
    std::size_t seed = 0;

    boost::hash_combine(seed, getUid());
    boost::hash_combine(seed, getDomains());
    boost::hash_combine(seed, getRole());

    return seed;
}

std::string DomainRoleEntry::getRoleInternal() const
{
    return joynr::infrastructure::DacTypes::Role::getLiteral(this->role);
}

std::string DomainRoleEntry::toString() const
{
    std::ostringstream typeAsString;
    typeAsString << "DomainRoleEntry{";
    typeAsString << "uid:" + getUid();
    typeAsString << ", ";
    typeAsString << " unprinted List domains  ";
    typeAsString << ", ";
    typeAsString << "role:" + getRoleInternal();
    typeAsString << "}";
    return typeAsString.str();
}

// printing DomainRoleEntry with google-test and google-mock
void PrintTo(const DomainRoleEntry& domainRoleEntry, ::std::ostream* os)
{
    *os << "DomainRoleEntry::" << domainRoleEntry.toString();
}

std::size_t hash_value(const DomainRoleEntry& domainRoleEntryValue)
{
    return domainRoleEntryValue.hashCode();
}

} // namespace DacTypes
} // namespace infrastructure
} // namespace joynr
