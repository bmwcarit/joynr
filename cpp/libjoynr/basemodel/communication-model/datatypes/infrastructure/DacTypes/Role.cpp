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

#include "joynr/infrastructure/DacTypes/Role.h"
#include <sstream>

namespace joynr
{
namespace infrastructure
{
namespace DacTypes
{

const std::int32_t Role::MAJOR_VERSION = 0;
const std::int32_t Role::MINOR_VERSION = 0;

std::string Role::getLiteral(const Role::Enum& roleValue)
{
    std::string literal;
    switch (roleValue) {
    case MASTER:
        literal = std::string("MASTER");
        break;
    case OWNER:
        literal = std::string("OWNER");
        break;
    }
    if (literal.empty()) {
        throw std::invalid_argument("Role: No literal found for value \"" +
                                    std::to_string(roleValue) + "\"");
    }
    return literal;
}

Role::Enum Role::getEnum(const std::string& roleString)
{
    if (roleString == std::string("MASTER")) {
        return MASTER;
    }
    if (roleString == std::string("OWNER")) {
        return OWNER;
    }
    std::stringstream errorMessage(roleString);
    errorMessage << " is unknown literal for Role";
    throw std::invalid_argument(errorMessage.str());
}

std::string Role::getTypeName()
{
    return "joynr.infrastructure.DacTypes.Role";
}

std::uint32_t Role::getOrdinal(Role::Enum roleValue)
{
    return static_cast<std::uint32_t>(roleValue);
}

// Printing Role with google-test and google-mock.
void PrintTo(const Role::Enum& roleValue, ::std::ostream* os)
{
    *os << "Role::" << Role::getLiteral(roleValue) << " (" << Role::getOrdinal(roleValue) << ")";
}

} // namespace DacTypes
} // namespace infrastructure
} // namespace joynr
