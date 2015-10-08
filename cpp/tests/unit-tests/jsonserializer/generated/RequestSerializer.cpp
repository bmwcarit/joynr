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
#include <utility>

namespace joynr
{

// Register the Request type id and serializer/deserializer
static const bool isRequestRegistered =
        Variant::registerType<Request>("joynr.infrastructure.Request");

static const bool isRequestSerializerRegistered =
        SerializerRegistry::registerType<Request>("joynr.infrastructure.Request");

template <>
void ClassDeserializer<Request>::deserialize(Request &t, IObject &o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "requestReplyId") {
            t.setRequestReplyId(field.value());

        } else if (field.name() == "methodName") {
            t.setMethodName(field.value());

        } else if (field.name() == "params") {

            IArray& array = field.value();
            auto&& converted = convertArray<Variant>(array, convertVariant);
            t.setParams(std::forward<std::vector<Variant>>(converted));
        }
    }
}

template <>
void ClassSerializer<Request>::serialize(const Request& request, std::ostream& stream)
{
    stream << "{";
    stream << "\"_typeName\": \"" << JoynrTypeId<Request>::getTypeName() << "\",";
    stream << "\"requestReplyId\": \"" << request.getRequestReplyId() << "\",";
    stream << "\"methodName\": \"" << request.getMethodName() << "\",";
    stream << "\"params\": ";
    ArraySerializer::serialize<Variant>(request.getParams(), stream);
    // ArraySerializer::serializeStrings(xxx, stream);
    // ClassSerializer<SomeObject>::serialize(object, stream);
    stream << "}";

}

} /* namespace joynr */
