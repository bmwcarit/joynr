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
#include "joynr/ClassDeserializer.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/IDeserializer.h"
#include "joynr/Util.h"
namespace joynr
{

/**
 * @brief deserialize not knowing the type
 * @param object
 * @return Variant
 */
Variant deserialize(IObject& o)
{
    // The object must have fields
    if (!o.hasNextField()) {
        throw std::invalid_argument("Object contains no fields");
    }

    // The first field has to contain the typename
    IField& field = o.nextField();
    if (field.name() != "_typeName") {
        throw std::invalid_argument("First field of object should be _typeName");
    }

    // Create an object of the correct type
    std::string typeName(field.value());
    std::unique_ptr<IClassDeserializer> deserializer =
            SerializerRegistry::getDeserializer(typeName);

    // Deserialize
    return deserializer->deserializeVariant(o);
}

Variant convertVariant(IValue &value)
{
    if (value.isObject()) {
        return deserialize(value);
    } else if (value.isArray()) {
        return Variant::make<std::vector<Variant>>(
                convertArray<Variant>(value, convertVariant));
    } else {
        // This covers all non-object values
        // i.e values without a _typename entry
        return value.getVariant();
    }
}

std::string convertString(IValue &value)
{
    return removeEscapeFromSpecialChars(static_cast<std::string>(value));
}

bool convertBool(IValue &value)
{
    return value.getBool();
}
} // namespace joynr
