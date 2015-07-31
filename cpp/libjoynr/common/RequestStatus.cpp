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
#include "joynr/RequestStatus.h"

#include <sstream>
#include <iostream>

#include "joynr/TypeUtil.h"

namespace joynr
{

RequestStatus::RequestStatus() : code(RequestStatusCode::NOT_STARTED), description()
{
}

RequestStatus::RequestStatus(RequestStatusCode requestStatus) : code(requestStatus), description()
{
}

RequestStatus::RequestStatus(RequestStatusCode requestStatus, const std::string& description)
        : code(requestStatus), description()
{
    addDescription(description);
}

bool RequestStatus::successful() const
{
    return code == RequestStatusCode::OK;
}

RequestStatusCode RequestStatus::getCode() const
{
    return code;
}

void RequestStatus::setCode(const RequestStatusCode& code)
{
    this->code = code;
}

std::list<std::string> RequestStatus::getDescription() const
{
    return description;
}

void RequestStatus::addDescription(const std::string& description)
{
    this->description.push_back(description);
}

std::string RequestStatus::toString() const
{
    std::ostringstream typeAsString;
    typeAsString << "RequestStatus{";
    typeAsString << "code:" + getCode().toString();
    typeAsString << ", ";
    typeAsString << "description: [ ";
    for (std::string desc : description) {
        typeAsString << desc << ", ";
    }
    typeAsString.seekp(2, std::ios_base::end);
    typeAsString << "] }";
    return typeAsString.str();
}

} // namespace joynr
