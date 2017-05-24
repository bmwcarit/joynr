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
#include "joynr/OneWayRequest.h"
#include "joynr/Util.h"

namespace joynr
{

OneWayRequest::OneWayRequest() : methodName(), paramDatatypes(), params()
{
}

bool OneWayRequest::operator==(const OneWayRequest& other) const
{
    return methodName == other.getMethodName() && paramDatatypes == other.paramDatatypes;
}

const std::string& OneWayRequest::getMethodName() const
{
    return methodName;
}

void OneWayRequest::setMethodName(const std::string& methodName)
{
    this->methodName = methodName;
}

void OneWayRequest::setMethodName(std::string&& methodName)
{
    this->methodName = std::move(methodName);
}

const std::vector<std::string>& OneWayRequest::getParamDatatypes() const
{
    return paramDatatypes;
}

void OneWayRequest::setParamDatatypes(const std::vector<std::string>& paramDatatypes)
{
    this->paramDatatypes = paramDatatypes;
}

void OneWayRequest::setParamDatatypes(std::vector<std::string>&& paramDatatypes)
{
    this->paramDatatypes = std::move(paramDatatypes);
}

} // namespace joynr
