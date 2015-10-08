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
#include "TEverythingStructSerializer.h"
#include "joynr/ArraySerializer.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"

namespace joynr {

using namespace joynr::types::TestTypes;

// Register the TEverythingStruct type id (_typeName value) and serializer/deserializer
static const bool isTEverythingStructSerializerRegistered =
        SerializerRegistry::registerType<TEverythingStruct>("joynr.types.TestTypes.TEverythingStruct");

static const bool isWordSerializerRegistered =
        SerializerRegistry::registerType<Word>("joynr.types.TestTypes.Word");

template<>
void ClassSerializer<TEverythingStruct>::serialize(const TEverythingStruct &everythingStruct, std::ostream &stream)
{
    stream << "{";
    stream << "\"_typeName\": \"" << JoynrTypeId<TEverythingStruct>::getTypeName() << "\",";
    stream << "\"tInt8\": " << std::to_string(everythingStruct.getTInt8()) << " ,";
    stream << "\"tUInt8\": " << std::to_string(everythingStruct.getTUInt8()) << " ,";
    stream << "\"tInt16\": " << std::to_string(everythingStruct.getTInt16()) << " ,";
    stream << "\"tUInt16\": " << std::to_string(everythingStruct.getTUInt16()) << " ,";
    stream << "\"tInt32\": " << std::to_string(everythingStruct.getTInt32()) << " ,";
    stream << "\"tUInt32\": " << std::to_string(everythingStruct.getTUInt32()) << " ,";
    stream << "\"tInt64\": " << std::to_string(everythingStruct.getTInt64()) << " ,";
    stream << "\"tUInt64\": " << std::to_string(everythingStruct.getTUInt64()) << " ,";
    stream << "\"tDouble\": " << everythingStruct.getTDouble() << " ,";
    stream << "\"tFloat\": " << everythingStruct.getTFloat() << " ,";
    stream << "\"tString\": \"" << everythingStruct.getTString() << "\",";
    stream << "\"tBoolean\": ";
    if (everythingStruct.getTBoolean()) {
        stream << "true";
    } else {
        stream << "false";
    }
    stream << " ,";
    stream << "\"tByteBuffer\": ";
    ArraySerializer::serialize<uint8_t>(everythingStruct.getTByteBuffer(), stream);
    stream << ",";
    stream << "\"tUInt8Array\": ";
    ArraySerializer::serialize<uint8_t>(everythingStruct.getTUInt8Array(), stream);
    stream << ",";
    stream << "\"tEnum\": \"" << TEnum::getLiteral(everythingStruct.getTEnum()) << "\",";
    stream << "\"tEnumArray\": ";
    ArraySerializer::serialize<TEnum::Enum>(everythingStruct.getTEnumArray(), stream);
    stream << ",";
    stream << "\"tStringArray\": ";
    ArraySerializer::serialize<std::string>(everythingStruct.getTStringArray(), stream);
    stream << ",";
    stream << "\"word\": ";
    ClassSerializer<Word> wordSerializer{};
    wordSerializer.serialize(everythingStruct.getWord(), stream);
    stream << ",";
    stream << "\"wordArray\": ";
    ArraySerializer::serialize<Word>(everythingStruct.getWordArray(), stream);
    stream << "}";
}

// deserialize to TEnum::Enum
types::TestTypes::TEnum::Enum convertToTEnum(IValue& value)
{
    std::string text = value;

    // following could be done much simpler if TEnum type could cast from std::string
    if (text == "TLITERALA") {
        return types::TestTypes::TEnum::TLITERALA;
    } else if (text == "TLITERALB") {
        return types::TestTypes::TEnum::TLITERALB;
    } else {
        throw std::invalid_argument("Unknown enum value");
    }
}

// deserialize to Vowel::Enum
types::TestTypes::Vowel::Enum convertToVowelEnum(IValue& value)
{
    std::string text = value;

    // following could be done much simpler if Vowel type could cast from std::string
    if (text == "A") {
        return types::TestTypes::Vowel::A;
    } else if (text == "E") {
        return types::TestTypes::Vowel::E;
    } else if (text == "I") {
        return types::TestTypes::Vowel::I;
    } else if (text == "O") {
        return types::TestTypes::Vowel::O;
    } else if (text == "U") {
        return types::TestTypes::Vowel::U;
    } else {
        throw std::invalid_argument("Unknown enum value");
    }
}

template<>
void ClassDeserializer<TEverythingStruct>::deserialize(TEverythingStruct &everythingStruct, IObject &object)
{
    while (object.hasNextField()) {
        IField& field = object.nextField();
        if (field.name() == "tInt8") {
            everythingStruct.setTInt8(field.value().getIntType<int8_t>());
        } else if (field.name() == "tInt16") {
            everythingStruct.setTInt16(field.value().getIntType<int16_t>());
        } else if (field.name() == "tInt32") {
            everythingStruct.setTInt32(field.value().getIntType<int32_t>());
        } else if (field.name() == "tInt64") {
            everythingStruct.setTInt64(field.value().getIntType<int64_t>());
        } if (field.name() == "tUInt8") {
            everythingStruct.setTUInt8(field.value().getUIntType<uint8_t>());
        } else if (field.name() == "tUInt16") {
            everythingStruct.setTUInt16(field.value().getUIntType<uint16_t>());
        } else if (field.name() == "tUInt32") {
            everythingStruct.setTUInt32(field.value().getUIntType<uint32_t>());
        } else if (field.name() == "tUInt64") {
            everythingStruct.setTUInt64(field.value().getUIntType<uint64_t>());
        } else if (field.name() == "tDouble") {
            everythingStruct.setTDouble(field.value().getDoubleType<double>());
        } else if (field.name() == "tFloat") {
            everythingStruct.setTFloat(field.value().getDoubleType<float>());
        } else if (field.name() == "tString") {
            everythingStruct.setTString(field.value());
        } else if (field.name() == "tBoolean") {
            everythingStruct.setTBoolean(field.value().getBool());
        } else if (field.name() == "tByteBuffer") {
            IArray& array = field.value();
            auto&& converted = convertArray<uint8_t>(array, convertUIntType<uint8_t>);
            everythingStruct.setTByteBuffer(std::forward<std::vector<uint8_t>>(converted));
        } else if (field.name() == "tUInt8Array") {
            IArray& array = field.value();
            auto&& converted = convertArray<uint8_t>(array, convertUIntType<uint8_t>);
            everythingStruct.setTUInt8Array(std::forward<std::vector<uint8_t>>(converted));
        } else if (field.name() == "tEnum") {
            everythingStruct.setTEnum(convertToTEnum(field.value()));
        } else if (field.name() == "tEnumArray") {
            IArray& array = field.value();
            auto&& converted = convertArray<types::TestTypes::TEnum::Enum>(array, convertToTEnum);
            everythingStruct.setTEnumArray(std::forward<std::vector<types::TestTypes::TEnum::Enum>>(converted));
        } else if (field.name() == "tStringArray") {
            IArray& array = field.value();
            auto&& converted = convertArray<std::string>(array, convertString);
            everythingStruct.setTStringArray(std::forward<std::vector<std::string>>(converted));
        } else if (field.name() == "word") {
            ClassDeserializer<Word> wordDeserializer;
            Word wordContainer{};
            wordDeserializer.deserialize(wordContainer, field.value());
            everythingStruct.setWord(wordContainer);
        } else if (field.name() == "wordArray") {
            IArray& array = field.value();
            auto&& converted = convertArray<Word>(array, convertObject<Word>);
            everythingStruct.setWordArray(std::forward<std::vector<Word>>(converted));
        }
    }
}

template<>
void ClassSerializer<TEnum::Enum>::serialize(const TEnum::Enum &tEnum, std::ostream &stream)
{
    stream << "\"" << types::TestTypes::TEnum::getLiteral(tEnum) << "\"";
}

template <>
void ClassSerializer<Vowel::Enum>::serialize(const Vowel::Enum& vowelEnum, std::ostream& stream)
{
    stream << "\"" << Vowel::getLiteral(vowelEnum) << "\"";
}

template<>
void ClassSerializer<Word>::serialize(const Word &word, std::ostream &stream)
{
    stream << "{";
    stream << "\"_typeName\": \"" << JoynrTypeId<Word>::getTypeName() << "\",";
    stream << "\"vowels\": ";
    ArraySerializer::serialize<Vowel::Enum>(word.getVowels(), stream);
    stream << "}";
}

template<>
void joynr::ClassDeserializer<Word>::deserialize(Word &word, IObject &object)
{
    while (object.hasNextField()) {
        IField& field = object.nextField();

        if (field.name() == "vowels") {
            IArray& array = field.value();
            auto&& converted = convertArray<Vowel::Enum>(array, convertToVowelEnum);
            word.setVowels(std::forward<std::vector<Vowel::Enum>>(converted));
        }
    }
}

} /* namespace joynr */
