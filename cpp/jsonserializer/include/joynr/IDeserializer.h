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
#ifndef DESERIALIZER
#define DESERIALIZER

#include <string>

#include "joynr/Variant.h"

namespace joynr
{

class IValue;
class IArray;
class IObject;

/**
 * @brief The IField class holds [name, value] information
 */
class IField
{
public:
    virtual ~IField() = default;

    virtual const std::string& name() const = 0;
    virtual const IValue& key() const = 0;
    virtual IValue& value() = 0;
};

/**
 * @brief The IValue class holds possible field values.
 * It allows clients to examine the type if the value it holds.
 */
class IValue
{
public:
    virtual ~IValue() = default;

    /**
     * @brief Get the value of the field as a string
     * @return The value of the field or an empty string if conversion
     *         was not possible
     */
    virtual operator const std::string&() const = 0;

    /**
     * @brief Get the value of the field as an array
     * @return An array
     */
    virtual operator IArray&() = 0;

    /**
     * @brief Get the value of the field as an object
     * @return An object
     */
    virtual operator IObject&() = 0;

    /**
     * @brief Is this value an array?
     */
    virtual bool isArray() const = 0;

    /**
     * @brief Is this value a string?
     */
    virtual bool isString() const = 0;

    /**
     * @brief Is this value an object?
     */
    virtual bool isObject() const = 0;

    template<typename T>
    T getIntType() const
    {
        return static_cast<T>(getInt64());
    }

    template<typename T>
    T getDoubleType() const
    {
        return static_cast<T>(getDouble());
    }

    template<typename T>
    T getUIntType() const
    {
        return static_cast<T>(getUInt64());
    }

    /**
     * @brief Get the value as a boolean
     * @return Boolean value
     */
    virtual bool getBool() const = 0;

    /**
     * @brief Get the value as a variant
     * @return Variant value
     */
    virtual Variant getVariant() const = 0;

protected:

    /**
     * @brief Get the value of the field as an int64_t, covers all int types
     * @return The value of the field or 0 if conversion
     *         was not possible
     */
    virtual int64_t getInt64() const = 0;

    /**
     * @brief Get the value of the field as an uint64_t, covers all uint types
     * @return The value of the field or 0 if conversion
     *         was not possible
     */
    virtual uint64_t getUInt64() const = 0;

    /**
     * @brief Get the value of the field as an double, covers all double and float
     * @return The value of the field or 0.0 if conversion
     *         was not possible
     */
    virtual double getDouble() const = 0;
};

/**
 * @brief The IObject class holds the object. Object consists of fields.
 */
class IObject
{
public:
    /**
     * @brief ~IObject
     */
    virtual ~IObject() = default;
    /**
     * @brief hasNextField
     * @return
     */
    virtual bool hasNextField() const = 0;
    /**
     * @brief Get the next field as a reference
     * @return A reference that is valid until nextField() is called again
     */
    virtual IField& nextField() = 0;
};

/**
 * @brief The IArray class is an abstract for an array (e.g. std::vector)
 */
class IArray
{
public:
    /**
     * @brief ~IArray
     */
    virtual ~IArray() = default;
    /**
     * @brief hasNextValue
     * @return
     */
    virtual bool hasNextValue() const = 0;
    /**
     * @brief Get the next value as a reference
     * @return A reference that is valid until nextValue() is called again
     */
    virtual IValue& nextValue() = 0;
};

/**
 * @brief Interface for a deserializer
 */
class IDeserializer
{
public:
    /**
     * @brief ~IDeserializer
     */
    virtual ~IDeserializer() = default;
    /**
     * @brief Can objects be deserialized?
     */
    virtual bool isValid() const = 0;
    /**
     * @brief Are objects available for deserialization
     */
    virtual bool hasNextObject() const = 0;
    /**
     * @brief Deserialize the next object
     * @return A reference that is valid until nextObject() is called again
     */
    virtual IObject& nextObject() = 0;
    /**
    * @brief Are there values available for deserialization
    */
    virtual bool hasNextValue() const = 0;
    /**
    * @brief Deserialize the next value
    * @return A reference that is valid until nextObject() is called again
    */
    virtual IValue& nextValue() = 0;
};

} // namespace joynr
#endif // DESERIALIZER

