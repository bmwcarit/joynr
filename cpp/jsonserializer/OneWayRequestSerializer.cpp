/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "OneWayRequestSerializer.h"

#include <string>
#include <utility> 
#include <vector>
#include <ostream>

#include "joynr/ArraySerializer.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"
#include "joynr/IDeserializer.h"

namespace joynr
{

// Register the OneWayRequest type id and serializer/deserializer
static const bool isOneWayRequestSerializerRegistered =
        SerializerRegistry::registerType<OneWayRequest>("joynr.OneWayRequest");

template <>
void ClassDeserializerImpl<OneWayRequest>::deserialize(OneWayRequest& request, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "methodName") {
            request.setMethodName(field.value());
        } else if (field.name() == "params") {
            IArray& array = field.value();
            auto&& converted = convertArray<Variant>(array, convertVariant);
            request.setParamsVariant(std::forward<std::vector<Variant>>(converted));
        } else if (field.name() == "paramDatatypes") {
            IArray& array = field.value();
            auto&& converted = convertArray<std::string>(array, convertString);
            request.setParamDatatypes(std::forward<std::vector<std::string>>(converted));
        }
    }
}

template <>
void ClassSerializerImpl<OneWayRequest>::serialize(const OneWayRequest& request, std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName":")" << JoynrTypeId<OneWayRequest>::getTypeName() << R"(",)";
    stream << R"("methodName": ")" << request.getMethodName() << R"(",)";
    stream << R"("paramDatatypes": )";
    ArraySerializer::serialize<std::string>(request.getParamDatatypes(), stream);
    stream << R"(,"params": )";
    ArraySerializer::serialize<Variant>(request.getParamsVariant(), stream);
    stream << R"(})";
}

} // namespace joynr
