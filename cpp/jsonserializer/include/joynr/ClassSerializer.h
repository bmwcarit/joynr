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
#ifndef CLASSSERIALIZER
#define CLASSSERIALIZER

#include "joynr/Variant.h"

#include <ostream>

namespace joynr
{

/**
 * @brief Interface implemented by classes that serialize other classes
 */
class IClassSerializer
{
public:
    virtual ~IClassSerializer(){}
    virtual void serializeVariant(const Variant& variant, std::ostream& stream) = 0;
};

/**
 * @brief Template for classes that serialize other classes
 */
template <class T>
class ClassSerializer : public IClassSerializer
{
public:
    virtual ~ClassSerializer() {}

    /**
     * @brief Serialize a variant
     */
    void serializeVariant(const Variant& variant, std::ostream& stream);

    /**
     * @brief Serialize the known type T
     */
    void serialize(const T& t, std::ostream& stream);
};

// Implementation of generic Variant serializer
template <class T>
void ClassSerializer<T>::serializeVariant(const Variant& variant, std::ostream& stream)
{
    serialize(variant.get<T>(), stream);
}

// Specialization for Variants
template <>
void ClassSerializer<Variant>::serializeVariant(const Variant &variant, std::ostream &stream);
template <>
void ClassSerializer<Variant>::serialize(const Variant &variant, std::ostream &stream);

} /* namespace joynr */
#endif // CLASSSERIALIZER

