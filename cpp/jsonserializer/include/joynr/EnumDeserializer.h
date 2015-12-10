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
#ifndef ENUMDESERIALIZER_H
#define ENUMDESERIALIZER_H

#include "joynr/Variant.h"

#include <functional>
#include <vector>
#include <utility>

namespace joynr
{

class IValue;
class IObject;

/**
 * @brief The IEnumDeserializer class is base class used for deserializing classes
 */
class IEnumDeserializer
{
public:
    /**
     * @brief ~IEnumDeserializer
     */
    virtual ~IEnumDeserializer() = default;
    /**
     * @brief deserializeVariant Every deserializer has be able to deserailize to Variant
     * @param value
     * @return Deserialized object enclosed in Variant.
     * If given object is not a Variant, returns empty Variant
     * (IVariantHolder* is nullptr)
     */
    virtual Variant deserializeVariant(IValue& value) = 0;
};

/**
 * @brief Type specific deserialization
 */
template <class T>
class EnumDeserializer : public IEnumDeserializer
{
public:
    ~EnumDeserializer() = default;

    /**
     * @brief deserialize Implementations are generated with the classes T,
     * to support deserialization to a type T this method has to be implemented
     * (e.g. generated code)
     * @param typeReference Reference to instance of given type
     * @param value Reference to object produced by Serializer Engine
     */
    static void deserialize(T& typeReference, const IValue& value);
    /**
     * @brief deserializeVariant
     * @param object
     * @return Variant (ref. IEnumDeserializer)
     */
    Variant deserializeVariant(IValue& object);
};

template <class T>
Variant EnumDeserializer<T>::deserializeVariant(IValue& o)
{
    Variant variant = Variant::make<T>();
    deserialize(variant.get<T>(), o);
    return variant;
}

/**
 * @brief deserialize not knowing the type
 * @param object
 * @return Variant
 */

Variant deserialize(IObject& object);

} /* namespace joynr */
#endif // ENUMDESERIALIZER_H
