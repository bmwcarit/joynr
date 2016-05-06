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
#ifndef ONEWAYREQUEST_H
#define ONEWAYREQUEST_H

#include <string>
#include <vector>

#include "joynr/JoynrCommonExport.h"

#include "joynr/Variant.h"

namespace joynr
{

class JOYNRCOMMON_EXPORT OneWayRequest
{
public:
    OneWayRequest();

    OneWayRequest(const OneWayRequest&) = default;
    OneWayRequest& operator=(const OneWayRequest&) = default;

    OneWayRequest(OneWayRequest&&) = default;
    OneWayRequest& operator=(OneWayRequest&&) = default;

    bool operator==(const OneWayRequest& other) const;

    const std::string& getMethodName() const;
    void setMethodName(const std::string& methodName);

    std::vector<Variant> getParams() const;
    void setParams(std::vector<Variant> params);

    void addParam(Variant value, std::string datatype);

    std::vector<std::string> getParamDatatypes() const;
    void setParamDatatypes(std::vector<std::string> paramDatatypes);

private:
    std::string methodName;
    std::vector<Variant> params;
    std::vector<std::string> paramDatatypes;
};

} // namespace joynr
#endif // ONEWAYREQUEST_H
