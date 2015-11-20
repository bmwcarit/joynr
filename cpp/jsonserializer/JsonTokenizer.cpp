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

#include "JsonTokenizer.h"
#include "joynr/SerializerRegistry.h"

#include <utility>
#include <cassert>
#include <cctype>

namespace joynr
{

// Register types used in variants
static const bool isJsonArrayRegistered = Variant::registerType<JsonArray>("JsonArray");
static const bool isJsonObjectRegistered = Variant::registerType<JsonObject>("JsonObject");
static const bool isJsonValueRegistered = Variant::registerType<JsonValue>("JsonValue");

//--------- Utils -------------------------------------------------------------

template<typename T, typename... TArgs>
std::unique_ptr<T> makeUnique(TArgs&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<TArgs>(args)...));
}

//--------- JsonToken ---------------------------------------------------------

JsonToken::JsonToken(const char *pStr, jsmntok_t jsmnToken) :
    type(jsmnToken.type),
    start(pStr + jsmnToken.start),
    end(pStr + jsmnToken.end),
    size(jsmnToken.size)
{
}

jsmntype_t JsonToken::getType() const
{
    return type;
}

std::string JsonToken::asString() const
{
    return std::string(start, end);
}

size_t JsonToken::getSize() const
{
    return size;
}

//--------- JsonObject --------------------------------------------------------

JsonObject::JsonObject(JsonTokenizer &tokenizer) :
    tokenizer(tokenizer),
    size(tokenizer.currentToken().getSize()),
    iterator(0),
    currentField()
{
}

JsonObject::JsonObject(JsonObject &&object) :
    tokenizer(object.tokenizer),
    size(object.size),
    iterator(object.iterator),
    currentField(std::move(object.currentField))
{

}

JsonObject::~JsonObject()
{
}

bool JsonObject::hasNextField() const
{
    return iterator < size;
}

IField &JsonObject::nextField()
{
    tokenizer.nextToken();
    iterator += 1;
    currentField = makeUnique<JsonField>(tokenizer);
    return *currentField;
}

//--------- JsonArray ---------------------------------------------------------

JsonArray::JsonArray(JsonTokenizer &tokenizer) :
    tokenizer(tokenizer),
    size(tokenizer.currentToken().getSize()),
    iterator(0),
    currentValue()
{
}

JsonArray::JsonArray(JsonArray&& array) :
   tokenizer(array.tokenizer),
   size(array.size),
   iterator(array.iterator),
   currentValue(std::move(array.currentValue))
{
}

JsonArray::~JsonArray()
{
}

bool JsonArray::hasNextValue() const
{
    return iterator < size;
}

IValue &JsonArray::nextValue()
{
    tokenizer.nextToken();
    iterator += 1;
    currentValue = makeUnique<JsonValue>(tokenizer);
    return *currentValue;
}

//--------- JsonValue ---------------------------------------------------------

JsonValue::JsonValue(JsonTokenizer &tokenizer) :
    value()
{
    jsmntype_t tokenType = tokenizer.currentToken().getType();

    switch(tokenType) {
    case JSMN_ARRAY:
        value = std::move(Variant::make<JsonArray>(tokenizer));
        break;
    case JSMN_OBJECT:
        value = std::move(Variant::make<JsonObject>(tokenizer));
        break;
    case JSMN_PRIMITIVE:
        value = parseJsonPrimitive(tokenizer.currentToken().asString());
        break;
    case JSMN_STRING:
        value = std::move(Variant::make<std::string>(tokenizer.currentToken().asString()));
        break;
    default:
        // Unknown type
        break;
    }
}

JsonValue::~JsonValue()
{
}

bool JsonValue::getBool() const
{
    if (!value.is<bool>()) {
        throw std::invalid_argument("Extracting int from non-string JsonValue");
    }
    return value.get<bool>();
}

JsonValue::operator const std::string&() const
{
    if (!value.is<std::string>()) {
        throw std::invalid_argument("Extracting string from non-string JsonValue");
    }
    return value.get<std::string>();
}

int64_t JsonValue::getInt64() const
{
    if (value.is<int64_t>()) {
        return value.get<int64_t>();
    } else if (value.is<uint64_t>()) {
        return static_cast<int64_t>(value.get<std::uint64_t>());
    } else {
        throw std::invalid_argument("Extracting int64_t from non-int JsonValue");
    }
}

double JsonValue::getDouble() const
{

    if (value.is<double>()) {
        return value.get<double>();
    } else if (value.is<int64_t>()) {
        return static_cast<double>(value.get<int64_t>());
    } else if (value.is<uint64_t>()) {
        return static_cast<double>(value.get<uint64_t>());
    }
    throw std::invalid_argument("Extracting double from non-double JsonValue");
}

uint64_t JsonValue::getUInt64() const
{
    if (!value.is<uint64_t>()) {
        throw std::invalid_argument("Extracting uint from non-uint JsonValue");
    }
    return value.get<uint64_t>();
}

Variant JsonValue::parseJsonPrimitive(const std::string &tokenString)
{
    // Is this true or false?
    if (tokenString == "true") {
        return Variant::make<bool>(true);
    } else if (tokenString == "false") {
        return Variant::make<bool>(false);
    }

    // Should the number be treated as an int or a float?
    // Assume a uint and see if any characters contradict this
    enum class NumberType { UINT, INT, FLOAT };
    NumberType numberType = NumberType::UINT;

    bool isFirstChar = true;

    for (char ch : tokenString) {
        // The first character can be a digit or a minus
        if (isFirstChar) {
            isFirstChar = false;
            if (ch == '-') {
                numberType = NumberType::INT;
            } else if (!std::isdigit(ch)) {
                // This is not a number - return an empty variant
                return Variant{};
            }
        } else if (!std::isdigit(ch))  {
            // Any non-digits imply floating point
            if (ch == '.' || ch == 'e') {
                numberType = NumberType::FLOAT;
            } else {
                // This is not a number - return an empty variant
                return Variant{};
            }
        }
    }

    // Convert based on the type of number
    if (numberType == NumberType::INT) {
        int64_t number = std::stoll(tokenString);
        return Variant::make<int64_t>(number);
    } else if (numberType == NumberType::UINT){
        uint64_t number = std::stoull(tokenString);
        return Variant::make<uint64_t>(number);
    } else {
        double number = std::stod(tokenString);
        return Variant::make<double>(number);
    }
}

JsonValue::operator IArray&()
{
    if (!value.is<JsonArray>()) {
        throw std::invalid_argument("Extracting array from non-array JsonValue");
    }
    return value.get<JsonArray>();
}

JsonValue::operator IObject&()
{
    if (!value.is<JsonObject>()) {
        throw std::invalid_argument("Extracting object from non-object JsonValue");
    }
    return value.get<JsonObject>();
}

bool JsonValue::isArray() const
{
    return value.is<JsonArray>();
}

bool JsonValue::isObject() const
{
    return value.is<JsonObject>();
}

Variant JsonValue::getVariant() const
{
    return value;
}

//--------- JsonField ---------------------------------------------------------

JsonField::JsonField(JsonTokenizer &tokenizer) :
    tokenizer(tokenizer),
    fieldName(tokenizer.currentToken().asString()),
    tokenValue()
{
    tokenizer.nextToken();
    tokenValue = makeUnique<JsonValue>(tokenizer);
}

JsonField::~JsonField()
{
}

const std::string &JsonField::name() const
{
    return fieldName;
}

IValue &JsonField::value()
{
    return *tokenValue;
}

//--------- JsonTokenizer -----------------------------------------------------

std::atomic_size_t JsonTokenizer::maxTokens{256};

JsonTokenizer::JsonTokenizer(const std::string &json) :
    source(json),
    tokens(maxTokens),
    currentIndex(0),
    parser(),
    valid(false),
    currentObject()
{
    jsmn_init(&parser);

    // Parse the JSON - valid remains false if any error occurs
    int res;
    while (true) {
        res = jsmn_parse(&parser, source.c_str(), json.size(),
                         tokens.data(), tokens.capacity());

        // Check for success
        if (res >= 0) {
            valid = true;
            break;
        }

        // If the token array is too small this application is parsing
        // unusually big JSON objects
        if (res == JSMN_ERROR_NOMEM) {
            size_t newMax = maxTokens.load() * 2;
            maxTokens.store(newMax);
            tokens.resize(newMax);
        } else {
            // A permanent error occured
            break;
        }
    }
}

JsonTokenizer::~JsonTokenizer()
{
}

bool JsonTokenizer::isValid() const
{
    return valid;
}

bool JsonTokenizer::hasNextObject() const
{
    // TODO: check the JSON and see how jsmn handles
    //       multiple objects
    return true;
}

IObject &JsonTokenizer::nextObject()
{
    assert(hasNextObject());
    currentObject = makeUnique<JsonObject>(*this);
    return *currentObject;
}

// TODO: optimise this so the token is not recreated every time
//       have invalid JsonToken type
JsonToken JsonTokenizer::currentToken()
{
    return JsonToken(source.c_str(), tokens[currentIndex]);
}


JsonToken JsonTokenizer::nextToken()
{
    if (currentIndex >= tokens.size()) {
        throw std::out_of_range("No more Json tokens");
    }

    return JsonToken(source.c_str(), tokens[++currentIndex]);
}

} /* namespace joynr */

