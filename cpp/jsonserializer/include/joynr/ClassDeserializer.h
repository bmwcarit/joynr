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

#include <functional>
#include <vector>
#include <utility>
#include <map>
#include <type_traits>
#include <string>

#include "joynr/Variant.h"
#include "joynr/Util.h"
#include "IDeserializer.h"
#include "PrimitiveDeserializer.h"

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
    virtual ~IClassDeserializer() = default;
    /**
     * @brief deserializeVariant Every deserializer has be able to deserialize to Variant
     * @param object
     * @return Deserialized object enclosed in Variant.
     * If given object is not a Variant, returns empty Variant
     * (IVariantHolder* is nullptr)
     */
    virtual Variant deserializeVariant(IObject& object) = 0;
};

/**
 * @brief this template is specialized for every supported type
 */
template <typename T, typename = void>
struct ClassDeserializerImpl
{
    static void deserialize(T& typeReference, IObject& value);
};

/**
 * @brief Type specific deserialization
 */
template <class T>
class ClassDeserializer : public IClassDeserializer
{
public:
    ~ClassDeserializer() override = default;

    /**
     * @brief deserialize Implementations are generated with the classes T,
     * to support deserialization to a type T this method has to be implemented
     * (e.g. generated code)
     * @param typeReference Reference to instance of given type
     * @param object Reference to object produced by Serializer Engine
     */
    static void deserialize(T& typeReference, IObject& value)
    {
        ClassDeserializerImpl<T>::deserialize(typeReference, value);
    }

    /**
     * @brief deserializeVariant
     * @param object
     * @return Variant (ref. IClassDeserializer)
     */
    Variant deserializeVariant(IObject& object) override;
};

template <typename T, typename = void>
struct SelectedDeserializer : ClassDeserializer<T> {};

template <typename T>
struct SelectedDeserializer<T,
                                std::enable_if_t<
                                std::is_enum<T>::value || std::is_same<std::string, T>::value>
                            >
        : PrimitiveDeserializer<T> {};


template <typename T>
struct SelectedDeserializer<T, std::enable_if_t<std::is_integral<T>::value>>
{
    static void deserialize(T& typeReference, const IValue& value)
    {
        typeReference = value.getIntType<T>();
    }
};

template <typename T>
struct SelectedDeserializer<T, std::enable_if_t<std::is_floating_point<T>::value>>
{
    static void deserialize(T& typeReference, const IValue& value)
    {
        typeReference = value.getDoubleType<T>();
    }
};

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

template<typename T>
T convertUIntType(IValue& value)
{
    return value.getUIntType<T>();
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
 * @brief convertBool
 * @param value
 * @return
 */
bool convertBool(IValue& value);

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


template <typename T, typename = void>
struct TypeConverter
{
    static T convert(IValue& value)
    {
        return convertObject<T>(value);
    }
};

template <typename T>
struct TypeConverter<T, std::enable_if_t<std::is_unsigned<T>::value>>
{
    static T convert(IValue& value)
    {
        return convertIntType<T>(value);
    }
};

template <typename T>
struct TypeConverter<T, std::enable_if_t<std::is_floating_point<T>::value>>
{
    static T convert(IValue& value)
    {
        return convertDoubleType<T>(value);
    }
};

template <typename T>
struct TypeConverter<T, std::enable_if_t<std::is_signed<T>::value>>
{
    static T convert(IValue& value)
    {
        return convertIntType<T>(value);
    }
};

template <>
struct TypeConverter<std::string>
{
    static std::string convert(IValue& value)
    {
        return convertString(value);
    }
};

template <>
struct TypeConverter<bool>
{
    static bool convert(IValue& value)
    {
        return convertBool(value);
    }
};

template <>
struct TypeConverter<Variant>
{
    static Variant convert(IValue& value)
    {
        return convertVariant(value);
    }
};

template <typename T>
struct TypeConverter<T, std::enable_if_t<util::IsDerivedFromTemplate<std::map, T>::value>>
{
    static T convert(IValue& value)
    {
        return convertMap<T>(value);
    }
};


template <typename T>
struct ArrayConverter
{
    static std::vector<T> convert(IArray& array)
    {
        std::vector<T> resultVector;

        while (array.hasNextValue()) {
            resultVector.emplace_back(TypeConverter<T>::convert(array.nextValue()));
        }
        return resultVector;
    }
};

template <typename T>
struct SelectedDeserializer<std::vector<T>>
{
    static void deserialize(std::vector<T>& typeReference, IValue& value)
    {
        IArray& array = value;
        typeReference = ArrayConverter<T>::convert(array);
    }
};


/**
 * @brief partial specialization for map deserialization
 */
template <typename T>
struct ClassDeserializerImpl<T, std::enable_if_t<util::IsDerivedFromTemplate<std::map, T>::value>>
{
    static void deserialize(T& map, IObject& object)
    {
        while (object.hasNextField()) {
            IField& field = object.nextField();
            if (field.key().isString() && field.name() == "_typeName") {
                continue;
            }
            using Key = typename T::key_type;
            Key key;
            SelectedDeserializer<Key>::deserialize(key, field.key());

            using Value = typename T::mapped_type;
            Value value;
            SelectedDeserializer<Value>::deserialize(value, field.value());
            map.insert({key, value});
        }
    }
};

template <class T>
Variant ClassDeserializer<T>::deserializeVariant(IObject& object)
{
    Variant variant = Variant::make<T>();
    deserialize(variant.get<T>(), object);
    return variant;
}

} // namespace joynr
#endif // CLASSDESERIALIZER_H
