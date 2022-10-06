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

#ifndef JOYNR_INFRASTRUCTURE_DACTYPES_ROLE_H
#define JOYNR_INFRASTRUCTURE_DACTYPES_ROLE_H

#include <cstdint>
#include <ostream>
#include <string>

#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{
namespace infrastructure
{
namespace DacTypes
{

/**
 * @brief Enumeration wrapper class Role
 *
 * @version 0.0
 */
struct Role : public joynr::exceptions::ApplicationExceptionError {

    using ApplicationExceptionError::ApplicationExceptionError;
    Role() = default;
    ~Role() override = default;

    /**
     * @brief The role of a user defines the rights for changing Access Control Lists (ACLs).
     * @version 0.0
     */
    enum Enum : std::uint32_t {
        /**
         * @brief Allows for changing Master Access Control List (Master ACLs), or Master
         * Registration Control List (Master RCL).
         */
        MASTER = 0,
        /**
         * @brief Allows for changing Owner Access Control Lists (Owner ACLs), or Owner Registration
         * Control List (Owner RCL).
         */
        OWNER = 1
    };

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

    /**
     * @brief Copy constructor
     * @param o the object to copy from
     */
    Role(const Role& o) = delete;

    /**
     * @brief Get the matching enum name for an ordinal number
     * @param roleValue The ordinal number
     * @return The string representing the enum for the given ordinal number
     */
    static std::string getLiteral(const Role::Enum& roleValue);

    /**
     * @brief Get the matching enum for a string
     * @param roleString The string representing the enum value
     * @return The enum value representing the string
     */
    static Role::Enum getEnum(const std::string& roleString);

    /**
     * @brief Get the matching ordinal number for an enum
     * @param roleValue The enum
     * @return The ordinal number representing the enum
     */
    static std::uint32_t getOrdinal(Role::Enum roleValue);

    /**
     * @brief Get the typeName of the enumeration type
     * @return The typeName of the enumeration type
     */
    static std::string getTypeName();
};

// Printing Role with google-test and google-mock.
/**
 * @brief Print values of MessagingQos object
 * @param messagingQos The current object instance
 * @param os The output stream to send the output to
 */
void PrintTo(const joynr::infrastructure::DacTypes::Role::Enum& roleValue, ::std::ostream* os);

} // namespace DacTypes
} // namespace infrastructure
} // namespace joynr

namespace std
{

/**
 * @brief Function object that implements a hash function for joynr::infrastructure::DacTypes::Role.
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template <>
struct hash<joynr::infrastructure::DacTypes::Role::Enum> {

    /**
     * @brief method overriding default implementation of operator ()
     * @param roleValue the operators argument
     * @return the ordinal number representing the enum value
     */
    std::size_t operator()(const joynr::infrastructure::DacTypes::Role::Enum& roleValue) const
    {
        return joynr::infrastructure::DacTypes::Role::getOrdinal(roleValue);
    }
};
} // namespace std

MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::infrastructure::DacTypes::Role,
                                 joynr::exceptions::ApplicationExceptionError,
                                 "joynr.infrastructure.DacTypes.Role")

namespace muesli
{
template <>
struct EnumTraits<joynr::infrastructure::DacTypes::Role::Enum> {
    using Wrapper = joynr::infrastructure::DacTypes::Role;
};
} // namespace muesli

#endif // JOYNR_INFRASTRUCTURE_DACTYPES_ROLE_H
