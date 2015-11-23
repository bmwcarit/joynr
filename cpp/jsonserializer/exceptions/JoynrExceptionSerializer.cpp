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
#include "exceptions/JoynrExceptionSerializer.h"

#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"

namespace joynr
{

// Register the JoynrRuntimeException type id and serializer/deserializer
static const bool isJoynrRuntimeExceptionRegistered =
        SerializerRegistry::registerType<exceptions::JoynrRuntimeException>("joynr.exceptions.JoynrRuntimeException");

template <>
void ClassDeserializer<exceptions::JoynrRuntimeException>::deserialize(exceptions::JoynrRuntimeException& t, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "detailMessage") {
            t.setMessage(field.value());
        }
    }
}
void initSerialization (const std::string& typeName, std::ostream& stream) {
    stream << R"({)";
    stream << R"("_typeName": ")" << typeName << R"(",)";
}

void serializeExceptionWithDetailMessage(const std::string& typeName, const exceptions::JoynrException& exception, std::ostream& stream) {
    initSerialization(typeName, stream);
    stream << R"("detailMessage": ")" << exception.getMessage() << R"(")";
    stream << "}";
}


template <>
void ClassSerializer<exceptions::JoynrRuntimeException>::serialize(const exceptions::JoynrRuntimeException& exception, std::ostream& stream)
{
    serializeExceptionWithDetailMessage(JoynrTypeId<exceptions::JoynrRuntimeException>::getTypeName(), exception, stream);
}
} /* namespace joynr */
