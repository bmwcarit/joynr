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
#ifndef UTIL_H
#define UTIL_H

#include <cassert>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <set>
#include <algorithm>

#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"

namespace joynr
{

class Logger;

namespace exceptions
{
class JoynrException;
} // namespace exceptions

namespace util
{
std::string removeEscapeFromSpecialChars(const std::string& inputStr);

/**
  * Splits a byte array representation of multiple JSON objects into
  * a list of byte arrays, each containing a single JSON object.
  */
std::vector<std::string> splitIntoJsonObjects(const std::string& jsonStream);

std::string attributeGetterFromName(const std::string& attributeName);

std::string loadStringFromFile(const std::string& fileName);
void saveStringToFile(const std::string& fileName, const std::string& strToSave);

// If the file is not open log the reason why. fileName is required for logging
bool isFileOpen(const std::fstream& file, const std::string& fileName);

template <typename T>
typename T::Enum convertVariantToEnum(const Variant& v)
{
    if (v.is<std::string>()) {
        std::string enumValueName = v.get<std::string>();
        return T::getEnum(enumValueName);
    } else {
        return v.get<typename T::Enum>();
    }
}

template <typename T>
std::vector<typename T::Enum> convertVariantVectorToEnumVector(
        const std::vector<Variant>& variantVector)
{
    std::vector<typename T::Enum> enumVector;
    enumVector.reserve(variantVector.size());
    for (const Variant& variant : variantVector) {
        enumVector.push_back(convertVariantToEnum<T>(variant));
    }
    return enumVector;
}

template <typename T>
std::vector<Variant> convertEnumVectorToVariantVector(
        const std::vector<typename T::Enum>& enumVector)
{
    std::vector<Variant> variantVector;
    variantVector.reserve(enumVector.size());
    for (const typename T::Enum& enumValue : enumVector) {
        variantVector.push_back(Variant::make<typename T::Enum>(enumValue));
    }
    return variantVector;
}

template <class T>
std::vector<Variant> convertVectorToVariantVector(const std::vector<T>& inputVector)
{
    std::vector<Variant> variantVector;
    variantVector.reserve(inputVector.size());
    for (const T& element : inputVector) {
        variantVector.push_back(Variant::make<T>(element));
    }
    return variantVector;
}

template <class T>
std::vector<T> convertVariantVectorToVector(const std::vector<Variant>& variantVector)
{
    std::vector<T> typeVector;
    typeVector.reserve(variantVector.size());

    for (Variant variant : variantVector) {
        typeVector.push_back(variant.get<T>());
    }

    return typeVector;
}

template <class T>
std::vector<T> convertIntListToEnumList(const std::vector<int>& inputList)
{
    std::vector<T> ret;
    ret.reserve(inputList.size());
    for (const int& i : inputList) {
        ret.push_back((T)i);
    }
    return ret;
}

template <class T>
std::vector<int> convertEnumListToIntList(const std::vector<T>& enumList)
{
    std::vector<int> enumAsIntList;
    enumAsIntList.reserve(enumList.size());
    for (const T& e : enumList) {
        enumAsIntList.push_back(e);
    }
    return enumAsIntList;
}

/**
 * Create a Uuid for use in Joynr.
 *
 * This is simply a wrapper around boost::uuid
 */
std::string createUuid();

/**
 * Log a serialized Joynr message
 */
void logSerializedMessage(Logger& logger,
                          const std::string& explanation,
                          const std::string& message);

void throwJoynrException(const exceptions::JoynrException& error);

template <typename... Ts>
int getTypeId();

template <typename T, typename... Ts>
int getTypeId_split()
{
    int prime = 31;
    return JoynrTypeId<T>::getTypeId() + prime * getTypeId<Ts...>();
}

template <typename... Ts>
int getTypeId()
{
    return getTypeId_split<Ts...>();
}

template <>
inline int getTypeId<>()
{
    return 0;
}

// this level of indirection is necessary to allow partial specialization
template <typename T>
struct ValueOfImpl
{
    static T valueOf(const Variant& variant)
    {
        return variant.get<T>();
    }
};

// partial specilization for lists of datatypes
template <typename T>
struct ValueOfImpl<std::vector<T>>
{
    static std::vector<T> valueOf(const Variant& variant)
    {
        return convertVariantVectorToVector<T>(variant.get<std::vector<Variant>>());
    }
};

template <typename T>
T valueOf(const Variant& variant)
{
    return ValueOfImpl<T>::valueOf(variant);
}

template <>
inline float valueOf<float>(const Variant& variant)
{
    return ValueOfImpl<double>::valueOf(variant);
}

template <>
inline std::string valueOf<std::string>(const Variant& variant)
{
    return removeEscapeFromSpecialChars(ValueOfImpl<std::string>::valueOf(variant));
}

template <>
inline std::vector<float> valueOf<std::vector<float>>(const Variant& variant)
{
    std::vector<double> doubles =
            convertVariantVectorToVector<double>(variant.get<std::vector<Variant>>());
    std::vector<float> floats(doubles.size());
    std::copy(doubles.cbegin(), doubles.cend(), floats.begin());
    return floats;
}

template <typename T>
std::set<T> vectorToSet(const std::vector<T>& v)
{
    return std::set<T>(v.begin(), v.end());
}

template <typename T>
bool setContainsSet(const std::set<T>& haystack, const std::set<T>& needle)
{
    bool contains = true;
    for (const T& element : haystack) {
        contains = (needle.count(element) == 1);
    }
    return contains;
}

template <typename T>
bool vectorContains(const std::vector<T>& v, const T& e)
{
    return v.end() != std::find(v.cbegin(), v.cend(), e);
}

template <typename T>
auto removeAll(std::vector<T>& v, const T& e)
{
    return v.erase(std::remove(v.begin(), v.end(), e), v.end());
}

/**
 *@brief this meta function allows to check whether a type U is derived from a template T
 */
template <template <typename...> class T, typename U>
struct IsDerivedFromTemplate
{
private:
    template <typename... Args>
    static decltype(static_cast<const T<Args...>&>(std::declval<U>()), std::true_type{}) test(
            const T<Args...>&);
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(IsDerivedFromTemplate::test(std::declval<U>()))::value;
};

} // namespace util

} // namespace joynr
#endif // UTIL_H
