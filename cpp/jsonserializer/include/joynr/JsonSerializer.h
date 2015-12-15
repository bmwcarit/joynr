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
#ifndef JSONSERIALIZER_H
#define JSONSERIALIZER_H

#include "joynr/joynrlogging.h"
#include <vector>
#include <sstream>
#include "joynr/Variant.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/JsonTokenizer.h"
#include "joynr/ArraySerializer.h"
#include <iomanip>

namespace joynr
{

/**
 * @brief Tool class to provide JSON serialization
 */
class JsonSerializer
{
public:
    /**
     * @brief Serializes a Variant into JSON format.
     *
     * @param variant the object to serialize.
     * @return std::string the serialized variant in JSON format, UTF-8 encoding.
     */
    static std::string serialize(const Variant& variant)
    {
        std::stringstream stream;
        auto serializer = SerializerRegistry::getSerializer(variant.getTypeName());
        serializer->serializeVariant(variant, stream);
        return stream.str();
    }

    /**
     * @brief Serializes a object into JSON format.
     *
     * @param object to serialize.
     * @return std::string the string in JSON format.
     */
    template <typename T>
    static std::string serialize(const T& object)
    {
        std::stringstream stream;
        stream << std::setprecision(9);
        auto serializer = ClassSerializer<T>{};
        serializer.serialize(object, stream);
        return stream.str();
    }

    static std::string serializeVector(const std::vector<Variant> vector)
    {
        std::stringstream stream;
        ArraySerializer::serialize<Variant>(vector, stream);
        return stream.str();
    }

    template <class T>
    /**
     * @brief Deserializes a json std::string in JSON format to the given template type T.
     *
     * Template type T must be registered as Variant and supported by appropriate @ref ClassDeserializer.
     * The json std::string must be a
     * valid JSON representation of the template type T.
     *
     * @param json The JSON representation of template type T.
     * @return The deserialized object, or NULL in case of deserialization error
     */
    static T* deserialize(const std::string& json)
    {
        JsonTokenizer tokenizer(json);

        T* object = new T();
        if (tokenizer.hasNextObject()) {
            ClassDeserializer<T>::deserialize(*object, tokenizer.nextObject());
        }

        return object;
    }

    template <class T>
    static std::vector<T*> deserializeVector(const std::string& json)
    {
        JsonTokenizer tokenizer(json);

        std::vector<T*> resultVector;
        if (tokenizer.hasNextValue()) {
            IValue& value = tokenizer.nextValue();
            if (value.isArray()) {
                IArray& array = value;
                auto&& converted = convertArray<Variant>(array, convertVariant);
                for (Variant v : converted) {
                    T* pointer = new T(v.get<T>());
                    resultVector.push_back(pointer);
                }
            }
        }

        return resultVector;
    }

private:
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // JSONSERIALIZER_H
