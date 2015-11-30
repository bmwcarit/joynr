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
#include "JoynrMessageSerializer.h"

#include "joynr/MapSerializer.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"

#include <string>
#include <utility>
#include <regex>

namespace joynr
{

// Register the Request type id and serializer/deserializer
static const bool isRequestRegistered =
        SerializerRegistry::registerType<JoynrMessage>("joynr.JoynrMessage");

static std::string  removeEscapeFromSpecialChars(const std::string& inputStr){
    std::string unEscapedString;
    std::regex expr ("(\\\\\")");
    std::regex_replace (std::back_inserter(unEscapedString), inputStr.begin(), inputStr.end(), expr, R"(")");

    return unEscapedString;
}

template <>
void ClassDeserializer<JoynrMessage>::deserialize(JoynrMessage& t, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "type") {
            t.setType(field.value());
        } else if (field.name() == "header") {
            auto&& converted = convertMap<std::string>(field.value(), convertString);
            t.setHeader(converted);
        } else if (field.name() == "payload") {
            t.setPayload( removeEscapeFromSpecialChars(field.value()));
        }
    }
}

template <>
void ClassSerializer<JoynrMessage>::serialize(const JoynrMessage& msg, std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName": ")" << JoynrTypeId<JoynrMessage>::getTypeName() << R"(",)";
    stream << R"("header": )";
    MapSerializer::serialize<std::string>(msg.getHeader(), stream);
    stream << R"(,"payload": )";
    ClassSerializer<std::string> stringSerializer{};
    stringSerializer.serialize(msg.getPayload(), stream);
    stream << R"(,"type": ")" << msg.getType() << R"(")";
    stream << "}";
}

} /* namespace joynr */
