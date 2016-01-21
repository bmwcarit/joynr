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
#include "joynr/RequestStatusCode.h"

#include <sstream>

namespace joynr
{

RequestStatusCode RequestStatusCode::OK = RequestStatusCode(0, "OK");
RequestStatusCode RequestStatusCode::NOT_STARTED = RequestStatusCode(1, "Not started");
RequestStatusCode RequestStatusCode::IN_PROGRESS = RequestStatusCode(2, "In progress");

RequestStatusCode RequestStatusCode::ERROR = RequestStatusCode(300, "Error");
RequestStatusCode RequestStatusCode::ERROR_TIMEOUT_DISCOVERY =
        RequestStatusCode(301, "Error timout waiting for discovery");
RequestStatusCode RequestStatusCode::ERROR_TIMEOUT_WAITING_FOR_RESPONSE =
        RequestStatusCode(302, "Error timeout waiting for the response");
RequestStatusCode RequestStatusCode::ERROR_CANNOT_PARSE_RETURN_VALUE = RequestStatusCode(
        303,
        "Error in ReplyCaller when attempting to cast the return type to the desired type");

RequestStatusCode::RequestStatusCode(std::uint32_t id, std::string description)
        : id(id), description(description)
{
}

std::string RequestStatusCode::toString() const
{
    std::ostringstream typeAsString;
    typeAsString << "[RequestStatusCode id: " << id << " description: " << description << "]";
    return typeAsString.str();
}

std::uint32_t RequestStatusCode::getId() const
{
    return id;
}

bool RequestStatusCode::operator==(const RequestStatusCode& requestStatusCode) const
{
    return id == requestStatusCode.getId();
}

bool RequestStatusCode::operator!=(const RequestStatusCode& requestStatusCode) const
{
    return !(*this == requestStatusCode);
}

} // namespace joynr
