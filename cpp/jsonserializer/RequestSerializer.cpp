/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "RequestSerializer.h"
#include "joynr/ArraySerializer.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"

#include <string>

namespace joynr
{

// Register the Request type id and serializer/deserializer
static const bool isRequestSerializerRegistered =
        SerializerRegistry::registerType<Request>("joynr.Request");

template <>
void ClassDeserializerImpl<Request>::deserialize(Request& request, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "requestReplyId") {
            request.setRequestReplyId(field.value());
        } else if (field.name() == "methodName") {
            request.setMethodName(field.value());
        } else if (field.name() == "params") {
            IArray& array = field.value();
            auto&& converted = convertArray<Variant>(array, convertVariant);
            request.setParams(std::forward<std::vector<Variant>>(converted));
        } else if (field.name() == "paramDatatypes") {
            IArray& array = field.value();
            auto&& converted = convertArray<std::string>(array, convertString);
            request.setParamDatatypes(std::forward<std::vector<std::string>>(converted));
        }
    }
}

template <>
void ClassSerializerImpl<Request>::serialize(const Request& request, std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName":")" << JoynrTypeId<Request>::getTypeName() << R"(",)";
    stream << R"("methodName": ")" << request.getMethodName() << R"(",)";
    stream << R"("paramDatatypes": )";
    ArraySerializer::serialize<std::string>(request.getParamDatatypes(), stream);
    stream << R"(,"params": )";
    ArraySerializer::serialize<Variant>(request.getParams(), stream);
    stream << R"(,"requestReplyId": ")" << request.getRequestReplyId() << R"(")";
    stream << R"(})";
}

} // namespace joynr
