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
#ifndef ONEWAYREQUEST_H
#define ONEWAYREQUEST_H

#include <string>
#include <vector>

#include "joynr/JoynrExport.h"
#include "joynr/serializer/SerializationPlaceholder.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class JOYNR_EXPORT OneWayRequest
{
public:
    OneWayRequest();

    OneWayRequest(OneWayRequest&&) = default;
    OneWayRequest& operator=(OneWayRequest&&) = default;

    bool operator==(const OneWayRequest& other) const;

    const std::string& getMethodName() const;
    void setMethodName(const std::string& methodName);
    void setMethodName(std::string&& methodName);

    const std::vector<std::string>& getParamDatatypes() const;
    void setParamDatatypes(const std::vector<std::string>& paramDatatypes);
    void setParamDatatypes(std::vector<std::string>&& paramDatatypes);

    template <typename... Ts>
    void setParams(Ts&&... values)
    {
        _params.setData(std::forward<Ts>(values)...);
    }

    template <typename... Ts>
    void getParams(Ts&... values)
    {
        _params.getData(values...);
    }

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(_methodName), MUESLI_NVP(_paramDatatypes), MUESLI_NVP(_params));
    }

private:
    std::string _methodName;
    std::vector<std::string> _paramDatatypes;
    joynr::serializer::SerializationPlaceholder _params;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::OneWayRequest, "joynr.OneWayRequest")

#endif // ONEWAYREQUEST_H
