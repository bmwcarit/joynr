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
#ifndef TYPE_JOYNR_INFRASTRUCTURE_DACTYPES_DOMAINROLEENTRY_H
#define TYPE_JOYNR_INFRASTRUCTURE_DACTYPES_DOMAINROLEENTRY_H

#include <cstddef>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "joynr/ByteBuffer.h"
#include "joynr/Util.h"

// include complex Datatype headers.
#include "joynr/infrastructure/DacTypes/Role.h"

#include "joynr/serializer/Serializer.h"

namespace joynr
{
namespace infrastructure
{
namespace DacTypes
{

/**
 * @brief The Domain Role Entry (DRE) stores domains for users/role combination. User may become
 * specified role when accessing given domains ACEs/RCEs in ACL/RCL. DREs are stored in the Domain
 * Roles Table (DRT) using the pair (uid, role) as combined primary key.
 * @version 0.0
 */
class DomainRoleEntry
{

public:
    /**
     * @brief MAJOR_VERSION The major version of this struct as specified in the
     * type collection or interface in the Franca model.
     */
    static const std::int32_t MAJOR_VERSION;
    /**
     * @brief MINOR_VERSION The minor version of this struct as specified in the
     * type collection or interface in the Franca model.
     */
    static const std::int32_t MINOR_VERSION;

    // general methods

    // default constructor
    /** @brief Constructor */
    DomainRoleEntry();

    // constructor setting all fields
    /**
     * @brief Parameterized constructor
     * @param uid The unique user ID (UID) this entry applies to.
     * @param domains A list of domains this entry applies to. A domain might also contain the
     * wildcard character (asterisk sign) to refer to all (sub-) domains.
     * @param role The role that is assigned to the specified user for the specified domains.
     */
    explicit DomainRoleEntry(const std::string& _uid,
                             const std::vector<std::string>& _domains,
                             const joynr::infrastructure::DacTypes::Role::Enum& _role);

    /** @brief Copy constructor */
    DomainRoleEntry(const DomainRoleEntry&) = default;

    /** @brief Move constructor */
    DomainRoleEntry(DomainRoleEntry&&) = default;

    /** @brief Destructor */
    ~DomainRoleEntry() = default;

    /**
     * @brief Stringifies the class
     * @return stringified class content
     */
    std::string toString() const;

    /**
     * @brief Returns a hash code value for this object
     * @return a hash code value for this object.
     */
    std::size_t hashCode() const;

    /**
     * @brief assigns an object
     * @return reference to the object assigned to
     */
    DomainRoleEntry& operator=(const DomainRoleEntry&) = default;

    /**
     * @brief move assigns an object
     * @return reference to the object assigned to
     */
    DomainRoleEntry& operator=(DomainRoleEntry&&) = default;

    /**
     * @brief "equal to" operator
     * @param other reference to the object to compare to
     * @return true if objects are equal, false otherwise
     */
    bool operator==(const DomainRoleEntry& other) const
    {
        return this->equals(other, joynr::util::MAX_ULPS);
    }

    /**
     * @brief "not equal to" operator
     * @param other reference to the object to compare to
     * @return true if objects are not equal, false otherwise
     */
    bool operator!=(const DomainRoleEntry& other) const
    {
        return !(*this == other);
    }

    // getters
    /**
     * @brief Gets Uid
     * @return The unique user ID (UID) this entry applies to.
     */
    inline const std::string& getUid() const
    {
        return uid;
    }
    /**
     * @brief Gets Domains
     * @return A list of domains this entry applies to. A domain might also contain the wildcard
     * character (asterisk sign) to refer to all (sub-) domains.
     */
    inline const std::vector<std::string>& getDomains() const
    {
        return domains;
    }
    /**
     * @brief Gets Role
     * @return The role that is assigned to the specified user for the specified domains.
     */
    inline const joynr::infrastructure::DacTypes::Role::Enum& getRole() const
    {
        return role;
    }

    // setters
    /**
     * @brief Sets Uid
     * @param uid The unique user ID (UID) this entry applies to.
     */
    inline void setUid(const std::string& _uid)
    {
        this->uid = _uid;
    }
    /**
     * @brief Sets Domains
     * @param domains A list of domains this entry applies to. A domain might also contain the
     * wildcard character (asterisk sign) to refer to all (sub-) domains.
     */
    inline void setDomains(const std::vector<std::string>& _domains)
    {
        this->domains = _domains;
    }
    /**
     * @brief Sets Role
     * @param role The role that is assigned to the specified user for the specified domains.
     */
    inline void setRole(const joynr::infrastructure::DacTypes::Role::Enum& _role)
    {
        this->role = _role;
    }

    /**
     * @brief equals method
     * @param other reference to the object to compare to
     * @param maxUlps maximum number of ULPs (Units in the Last Place) that are tolerated when
     * comparing to floating point values
     * @return true if objects are equal, false otherwise
     */
    bool equals(const DomainRoleEntry& other, std::size_t maxUlps) const
    {
        return this->equalsInternal(other, maxUlps);
    }

protected:
    // printing DomainRoleEntry with google-test and google-mock
    /**
     * @brief Print values of a DomainRoleEntry object
     * @param domainRoleEntry The current object instance
     * @param os The output stream to send the output to
     */
    friend void PrintTo(const DomainRoleEntry& domainRoleEntry, ::std::ostream* os);

    /**
     * @brief equals method
     * @param other reference to the object to compare to
     * @return true if objects are equal, false otherwise
     */
    bool equalsInternal(const DomainRoleEntry& other, std::size_t maxUlps) const
    {
        return joynr::util::compareValues(this->uid, other.uid, maxUlps) &&
               joynr::util::compareValues(this->domains, other.domains, maxUlps) &&
               joynr::util::compareValues(this->role, other.role, maxUlps);
    }

private:
    // serialize DomainRoleEntry with muesli
    template <typename Archive>
    friend void serialize(Archive& archive, DomainRoleEntry& domainroleentryObj);

    // members
    std::string uid;
    std::vector<std::string> domains;
    joynr::infrastructure::DacTypes::Role::Enum role;
    std::string getRoleInternal() const;
};

std::size_t hash_value(const DomainRoleEntry& domainRoleEntryValue);

// serialize DomainRoleEntry with muesli
template <typename Archive>
void serialize(Archive& archive, DomainRoleEntry& domainroleentryObj)
{
    archive(muesli::make_nvp("uid", domainroleentryObj.uid),
            muesli::make_nvp("domains", domainroleentryObj.domains),
            muesli::make_nvp("role", domainroleentryObj.role));
}

} // namespace DacTypes
} // namespace infrastructure
} // namespace joynr

namespace std
{

/**
 * @brief Function object that implements a hash function for
 * joynr::infrastructure::DacTypes::DomainRoleEntry.
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template <>
struct hash<joynr::infrastructure::DacTypes::DomainRoleEntry>
{

    /**
     * @brief method overriding default implementation of operator ()
     * @param domainRoleEntryValue the operators argument
     * @return the ordinal number representing the enum value
     */
    std::size_t operator()(
            const joynr::infrastructure::DacTypes::DomainRoleEntry& domainRoleEntryValue) const
    {
        return joynr::infrastructure::DacTypes::hash_value(domainRoleEntryValue);
    }
};
} // namespace std

MUESLI_REGISTER_TYPE(joynr::infrastructure::DacTypes::DomainRoleEntry,
                     "joynr.infrastructure.DacTypes.DomainRoleEntry")

#endif // TYPE_JOYNR_INFRASTRUCTURE_DACTYPES_DOMAINROLEENTRY_H
