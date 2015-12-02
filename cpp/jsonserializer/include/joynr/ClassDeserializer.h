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
#ifndef CLASSDESERIALIZER_H
#define CLASSDESERIALIZER_H

#include "joynr/Variant.h"
#include "IDeserializer.h"

#include <functional>
#include <vector>
#include <utility>
#include <map>

namespace joynr
{

class IObject;

/**
 * @brief The IClassDeserializer class is base class used for deserializing classes
 */
class IClassDeserializer
{
public:
    /**
     * @brief ~IClassDeserializer
     */
    virtual ~IClassDeserializer() {}
    /**
     * @brief deserializeVariant Every deserializer has be able to deserailize to Variant
     * @param object
     * @return Deserialized object enclosed in Variant.
     * If given object is not a Variant, returns empty Variant
     * (IVariantHolder* is nullptr)
     */
    virtual Variant deserializeVariant(IObject& object) = 0;
};

/**
 * @brief Type specific deserialization
 */
template <class T>
class ClassDeserializer : public IClassDeserializer
{
public:
    ~ClassDeserializer() {}

    /**
     * @brief deserialize Implementations are generated with the classes T,
     * to support deserialization to a type T this method has to be implemented
     * (e.g. generated code)
     * @param typeReference Reference to instance of given type
     * @param object Reference to object produced by Serializer Engine
     */
    static void deserialize(T& typeReference, IObject& value);
    /**
     * @brief deserializeVariant
     * @param object
     * @return Variant (ref. IClassDeserializer)
     */
    Variant deserializeVariant(IObject& object);
};

template <class T>
Variant ClassDeserializer<T>::deserializeVariant(IObject& o)
{
    Variant variant = Variant::make<T>();
    deserialize(variant.get<T>(), o);
    return variant;
}

/**
 * @brief convertVariant IValue to Variant (e.g. JsonValue to Variant)
 * @param value reference to concrete IValue
 * @return Variant
 */
Variant convertVariant(IValue& value);

/**
 * @brief convertString
 * @param value
 * @return
 */
std::string convertString(IValue& value);

/**
 * @brief Converts an IArray into a std::vector, used for deserialization
 */
template <typename T>
std::vector<T> convertArray(IArray& array, std::function<T(IValue&)> fn)
{
    std::vector<T> resultVector;

    while (array.hasNextValue()) {
        resultVector.push_back(fn(array.nextValue()));
    }
    return resultVector;
}

/**
 * @brief Converts an IArray into a std::vector, used for deserialization
 */
template <typename T>
std::vector<T> convertArray(IArray& array, std::function<void(T&, IValue&)> fn)
{
    std::vector<T> resultVector;

    while (array.hasNextValue()) {
        T value;
        fn(value, array.nextValue());
        resultVector.push_back(value);
    }
    return resultVector;
}

/**
 * @brief Converts an IObject into a std::map, used for deserialization
 */
template <typename T>
std::map<std::string, T> convertMap(IObject& map, std::function<T(IValue&)> fn)
{
    std::map<std::string, T> resultMap;

    while (map.hasNextField()) {
        IField& keyValuePair = map.nextField();
        std::string keyName = keyValuePair.name();
        T value = fn(keyValuePair.value());
        resultMap.emplace(keyName, std::move(value));
    }
    return resultMap;
}

/**
 * @brief convertObject
 * @param value
 * @return
 */
template<typename T>
T convertObject(IValue& value)
{
    ClassDeserializer<T> deserializer;
    T obj;
    deserializer.deserialize(obj, value);
    return obj;
}

template<typename T>
T convertIntType(IValue& value)
{
    return value.getIntType<T>();
}

template<typename T>
T convertDoubleType(IValue& value)
{
    return value.getDoubleType<T>();
}

template<typename T>
T convertUIntType(IValue& value)
{
    return value.getUIntType<T>();
}

} /* namespace joynr */
#endif // CLASSDESERIALIZER_H
