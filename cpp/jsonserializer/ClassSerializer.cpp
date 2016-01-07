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
#include "joynr/ClassSerializer.h"
#include "joynr/SerializerRegistry.h"
#include <iomanip>
#include <sstream>
#include <regex>

namespace joynr
{

template <>
void ClassSerializer<std::int8_t>::serialize(const std::int8_t& i, std::ostream& stream)
{
    stream << std::to_string(i);
}

template <>
void ClassSerializer<std::uint8_t>::serialize(const std::uint8_t& i, std::ostream& stream)
{
    stream << std::to_string(i);
}

template <>
void ClassSerializer<std::int16_t>::serialize(const std::int16_t& i, std::ostream& stream)
{
    stream << std::to_string(i);
}

template <>
void ClassSerializer<std::uint16_t>::serialize(const std::uint16_t& i, std::ostream& stream)
{
    stream << std::to_string(i);
}

template <>
void ClassSerializer<std::int32_t>::serialize(const std::int32_t& i, std::ostream& stream)
{
    stream << std::to_string(i);
}

template <>
void ClassSerializer<std::uint32_t>::serialize(const std::uint32_t& i, std::ostream& stream)
{
    stream << std::to_string(i);
}

template <>
void ClassSerializer<std::int64_t>::serialize(const std::int64_t& i, std::ostream& stream)
{
    stream << std::to_string(i);
}

template <>
void ClassSerializer<std::uint64_t>::serialize(const std::uint64_t& i, std::ostream& stream)
{
    stream << std::to_string(i);
}

template <>
void ClassSerializer<double>::serialize(const double& i, std::ostream& stream)
{
    std::stringstream buffer;
    buffer << std::setprecision(std::numeric_limits<double>::digits10) << i;
    std::string doubleStr = buffer.str();
    buffer.flush();
    stream << doubleStr;
    if (doubleStr.find('.') == std::string::npos && doubleStr.find('e') == std::string::npos) {
        stream << ".0";
    }
}

template <>
void ClassSerializer<float>::serialize(const float& i, std::ostream& stream)
{
    std::stringstream buffer;
    buffer << std::setprecision(std::numeric_limits<float>::digits10) << i;
    std::string floatStr = buffer.str();
    buffer.flush();
    stream << floatStr;
    if (floatStr.find('.') == std::string::npos && floatStr.find('e') == std::string::npos) {
        stream << ".0";
    }
}

template <>
void ClassSerializer<bool>::serialize(const bool& i, std::ostream& stream)
{
    stream << std::boolalpha << i;
}

/**
 * @brief addEscapeForSpecialCharacters Escapes special characters in a string
 * @param str
 * @return
 */
std::string addEscapeForSpecialCharacters(const std::string& str) {
    std::string escapedString;
    std::regex expr (R"((\\|"))");
    std::regex_replace (std::back_inserter(escapedString), str.begin(), str.end(), expr, std::string(R"(\$&)"));

    return escapedString;
}

template <>
void ClassSerializer<std::string>::serialize(const std::string& s, std::ostream& stream)
{
    stream << '"' << addEscapeForSpecialCharacters(s) << '"';
}

template <>
void ClassSerializer<Variant>::serializeVariant(const Variant &variant, std::ostream &stream)
{
    serialize(variant, stream);
}

template <>
void ClassSerializer<Variant>::serialize(const Variant &variant,
                                             std::ostream &stream)
{
    if (variant.isEmpty()) {
        return;
    }

    std::string typeName = variant.getTypeName();
    auto serializer = SerializerRegistry::getSerializer(typeName);

    // Check that a Joynr serializer is available
    if (serializer) {
        serializer->serializeVariant(variant, stream);
    }
}

} // namespace joynr
