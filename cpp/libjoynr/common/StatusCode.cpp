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
#include "joynr/StatusCode.h"

#include <sstream>

namespace joynr
{

StatusCode StatusCode::SUCCESS = StatusCode(0, "Success");
StatusCode StatusCode::IN_PROGRESS = StatusCode(2, "In progress");

StatusCode StatusCode::ERROR = StatusCode(300, "Error");

StatusCode::StatusCode(std::uint32_t id, std::string description) : id(id), description(description)
{
}

std::string StatusCode::toString() const
{
    std::ostringstream typeAsString;
    typeAsString << "[StatusCode id: " << id << " description: " << description << "]";
    return typeAsString.str();
}

std::uint32_t StatusCode::getId() const
{
    return id;
}

bool StatusCode::operator==(const StatusCode& statusCode) const
{
    return id == statusCode.getId();
}

bool StatusCode::operator!=(const StatusCode& statusCode) const
{
    return !(*this == statusCode);
}

bool StatusCode::success() const
{
    return *this == SUCCESS;
}

bool StatusCode::inProgress() const
{
    return *this == IN_PROGRESS;
}

} // namespace joynr
