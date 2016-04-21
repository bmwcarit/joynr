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
#include "BroadcastFilterParametersSerializer.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/MapSerializer.h"
#include <string>

namespace joynr
{
// Register the BroadcastFilterParameters type id and serializer/deserializer
static const bool isBroadcastFilterParametersSerializerRegistered =
        SerializerRegistry::registerType<BroadcastFilterParameters>(
                "joynr.BroadcastFilterParameters");

template <>
void ClassDeserializerImpl<BroadcastFilterParameters>::deserialize(
        BroadcastFilterParameters& parameters,
        IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "filterParameters") {
            auto&& converted = convertMap<std::string>(field.value(), convertString);
            parameters.setFilterParameters(converted);
        }
    }
}

template <>
void ClassSerializerImpl<BroadcastFilterParameters>::serialize(
        const BroadcastFilterParameters& parameters,
        std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName":")" << JoynrTypeId<BroadcastFilterParameters>::getTypeName()
           << R"(",)";
    stream << R"("filterParameters": )";
    MapSerializer::serialize(parameters.getFilterParameters(), stream);
    stream << R"(})";
}
} // namespace joynr
