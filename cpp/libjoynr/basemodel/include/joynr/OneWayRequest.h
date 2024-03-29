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
    void setMethodName(const std::string& methodNameLocal);
    void setMethodName(std::string&& methodNameLocal);

    const std::vector<std::string>& getParamDatatypes() const;
    void setParamDatatypes(const std::vector<std::string>& paramDatatypesLocal);
    void setParamDatatypes(std::vector<std::string>&& paramDatatypesLocal);

    template <typename... Ts>
    void setParams(Ts&&... values)
    {
        params.setData(std::forward<Ts>(values)...);
    }

    template <typename... Ts>
    void getParams(Ts&... values)
    {
        params.getData(values...);
    }

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(methodName), MUESLI_NVP(paramDatatypes), MUESLI_NVP(params));
    }

private:
    std::string methodName;
    std::vector<std::string> paramDatatypes;
    joynr::serializer::SerializationPlaceholder params;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::OneWayRequest, "joynr.OneWayRequest")

#endif // ONEWAYREQUEST_H
