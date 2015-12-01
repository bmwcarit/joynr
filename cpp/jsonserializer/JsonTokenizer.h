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
#ifndef JSONTOKENIZER_H
#define JSONTOKENIZER_H

#include "jsmn.h"
#include "IDeserializer.h"
#include "joynr/Variant.h"

#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <sstream>
#include <cstddef>

namespace joynr
{

class JsonTokenizer;

/**
 * @brief The JsonToken class wrapper for jsmntok_t token
 */
class JsonToken
{
public:
    /**
     * @brief JsonToken constructor
     * @param pStr
     * @param jsmnToken
     */
    JsonToken(const char *pStr, jsmntok_t jsmnToken);

    /**
     * @brief getType
     * @return
     */
    jsmntype_t getType() const;
    /**
     * @brief asString
     * @return
     */
    std::string asString() const;
    /**
     * @brief getSize
     * @return
     */
    std::size_t getSize() const;

private:
    jsmntype_t type;
    const char *start;
    const char *end;
    std::size_t size;
};

/**
 * @brief A JSON field has its fieldName (std::string) and fieldValue (IValue).
 * fieldValue may be IObject, IArray, or concrete basic type value
 */
class JsonField : public IField
{
public:
    /**
     * @brief JsonField
     * @param tokenizer
     */
    JsonField(JsonTokenizer& tokenizer);
    JsonField(const JsonField& other) = default;
    JsonField(JsonField&& other) = default;
    JsonField& operator=(const JsonField& other) = default;
    JsonField& operator=(JsonField&& other) = default;
    ~JsonField() = default;
    /**
     * @brief name get fieldName
     * @return
     */
    const std::string& name() const;
    /**
     * @brief value get fieldValue
     * @return
     */
    IValue& value();

private:
    JsonTokenizer& tokenizer;
    std::string fieldName;
    std::unique_ptr<IValue> tokenValue;
};

/**
 * @brief The JsonObject class holds one or more JsonFields
 */
class JsonObject : public IObject
{
public:
    /**
     * @brief JsonObject
     * @param tokenizer
     */
    JsonObject(JsonTokenizer& tokenizer);
    JsonObject(const JsonObject& other) = default;
    JsonObject& operator=(const JsonObject& other) = default;
    JsonObject& operator=(JsonObject&& other) = default;

    /**
     * @brief JsonObject
     * @param object
     */
    JsonObject(JsonObject&& object) = default;

    ~JsonObject() = default;

    /**
     * @brief hasNextField
     * @return
     */
    bool hasNextField() const;
    /**
     * @brief nextField
     * @return
     */
    IField& nextField();
private:
    JsonTokenizer& tokenizer;
    std::size_t size;
    std::size_t iterator;
    std::unique_ptr<JsonField> currentField;
};

/**
 * @brief The JsonArray class
 */
class JsonArray : public IArray
{
public:
    /**
     * @brief JsonArray
     * @param tokenizer
     */
    JsonArray(JsonTokenizer& tokenizer);
    /**
     * @brief JsonArray
     * @param array
     */
    JsonArray(JsonArray&& array) = default;
    JsonArray(const JsonArray& other) = default;
    JsonArray& operator=(const JsonArray& other) = default;
    JsonArray& operator=(JsonArray&& other) = default;
    ~JsonArray() = default;

    /**
     * @brief hasNextValue
     * @return
     */
    bool hasNextValue() const;
    /**
     * @brief nextValue
     * @return
     */
    IValue& nextValue();
private:
    JsonTokenizer& tokenizer;
    std::size_t size;
    std::size_t iterator;
    std::unique_ptr<IValue> currentValue;
};

//------- JsonValue ------------------------------------------------------------
/**
 * @brief The JsonValue class hold acctual JsonField value.
 * It can be another JsonObject, JsonArray, or concrete value.
 */
class JsonValue : public IValue
{
public:
    /**
     * @brief JsonValue
     * @param tokenizer
     */
    JsonValue(JsonTokenizer& tokenizer);
    JsonValue(JsonValue&& array) = default;
    JsonValue(const JsonValue& other) = default;
    JsonValue& operator=(const JsonValue& other) = default;
    JsonValue& operator=(JsonValue&& other) = default;
    ~JsonValue() = default;

    /**
     * @brief operator const std::string &
     */
    operator const std::string&() const;
    /**
     * @brief operator IArray &
     */
    operator IArray&();
    /**
     * @brief operator IObject &
     */
    operator IObject&();
    /**
     * @brief getBool converts to bool
     * @return
     */
    bool getBool() const;
    /**
     * @brief isArray
     * @return
     */
    bool isArray() const;
    /**
     * @brief isObject
     * @return
     */
    bool isObject() const;

    /**
     * @brief Get the value as a variant
     * @return The variant
     */
    Variant getVariant() const;

protected:
    /**
     * @brief conversion to int64_t, doesn't allow conversion of boolean to int
     */
    int64_t getInt64() const;
    /**
     * @brief conversion to double
     */
    double getDouble() const;
    /**
     * @brief conversion to uint64_t
     * @return
     */
    uint64_t getUInt64() const;
private:
    Variant value;
    JsonTokenizer& tokenizer;

    // Parse a variant from a token string
    Variant parseJsonPrimitive(const std::string& tokenString);
};

/**
 * @brief A tokenizer for JSON based on jsmn
 */
class JsonTokenizer : IDeserializer
{
public:
    /**
     * @brief Create a tokenizer
     * @param json The JSON to parse. Must remain valid for the lifetime
     *             of the object.
     */
    JsonTokenizer(const std::string& json);
    JsonTokenizer(const JsonTokenizer& other) = default;
    JsonTokenizer(JsonTokenizer&& other) = default;
    JsonTokenizer& operator=(const JsonTokenizer& other) = default;
    JsonTokenizer& operator=(JsonTokenizer&& other) = default;
    ~JsonTokenizer() = default;

    /**
     * @brief Returns the current token
     */
    JsonToken currentToken();

    /**
     * @brief Moves to the next token
     * @return the new current token
     */
    JsonToken nextToken();

    /**
     * @brief isValid
     * @return
     */
    bool isValid() const;
    /**
     * @brief hasNextObject
     * @return
     */
    bool hasNextObject() const;
    /**
     * @brief nextObject
     * @return
     */
    IObject& nextObject();
    /**
     * @brief hasNextValue sometimes json can contain only value, or array of values
     * @return
     */
    bool hasNextValue() const;
    /**
     * @brief nextValue sometimes json can contain only value, or array of values
     * @return
     */
    IValue &nextValue();

    double stringToDoubleLocaleIndependent(const std::string& doubleStr);

private:
    const std::string& source;
    std::vector<jsmntok_t> tokens;
    std::size_t currentIndex;
    jsmn_parser parser;
    bool valid;
    std::unique_ptr<JsonObject> currentObject;
    std::unique_ptr<JsonValue> currentValue;
    std::stringstream classicLocaleStream;

    static std::atomic<std::size_t> maxTokens;
};

} /* namespace joynr */
#endif // JSONTOKENIZER_H
