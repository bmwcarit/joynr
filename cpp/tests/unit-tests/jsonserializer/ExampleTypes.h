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
#ifndef EXAMPLETYPES_H
#define EXAMPLETYPES_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/Variant.h"

#include <string>
#include <vector>
#include <utility>

namespace joynr
{
//---- An empty type -----------------------------------------------------------

struct SomeType {};

// A serializer for the empty type
template <>
void ClassSerializer<SomeType>::serialize(const SomeType& t, std::ostream& stream);

// A deserializer for the empty type
template <>
void ClassDeserializer<SomeType>::deserialize(SomeType& t, IObject& o);

//---- A simple type -----------------------------------------------------------

struct SomeOtherType
{
    SomeOtherType() : a(0) {}
    SomeOtherType(int a) : a(a) {}

    int getA() const { return a; }
    void setA(int value) { a = value; }

private:
    int a;
};

// A serializer for the simple type
template <>
void ClassSerializer<SomeOtherType>::serialize(const SomeOtherType& t, std::ostream& stream);

// A deserializer for the simple type
template <>
void ClassDeserializer<SomeOtherType>::deserialize(SomeOtherType& t, IObject& o);

//---- MasterAccessControlEntry -------------------------------------------------

class ExamplePermission
{
public:
    enum Enum : uint32_t {
        YES = 0, ASK = 1,  NO = 2
    };
};

class ExampleMasterAccessControlEntry
{
public:
    ExampleMasterAccessControlEntry() :
        operation(),
        defaultConsumerPermission(ExamplePermission::NO),
        possibleConsumerPermissions()
    {}


    inline const std::string& getOperation() const
    {
        return operation;
    }

    inline const ExamplePermission::Enum& getDefaultConsumerPermission() const
    {
        return defaultConsumerPermission;
    }

    inline const std::vector<ExamplePermission::Enum>& getPossibleConsumerPermissions() const
    {
        return possibleConsumerPermissions;
    }

    inline void setOperation(const std::string& operation)
    {
        this->operation = operation;
    }

    inline void setDefaultConsumerPermission(const ExamplePermission::Enum& defaultConsumerPermission)
    {
        this->defaultConsumerPermission = defaultConsumerPermission;
    }

    inline void setPossibleConsumerPermissions(const std::vector<ExamplePermission::Enum>& possibleConsumerPermissions)
    {
        this->possibleConsumerPermissions = possibleConsumerPermissions;
    }

private:
    std::string operation;
    ExamplePermission::Enum defaultConsumerPermission;
    std::vector<ExamplePermission::Enum> possibleConsumerPermissions;
};

// A serializer for ExampleMasterAccessControlEntry
template <>
void ClassSerializer<ExampleMasterAccessControlEntry>::serialize(const ExampleMasterAccessControlEntry& mace, std::ostream& stream);

// A deserializer for ExampleMasterAccessControlEntry
template <>
void ClassDeserializer<ExampleMasterAccessControlEntry>::deserialize(ExampleMasterAccessControlEntry& t, IObject& o);

} // namespace joynr
#endif // EXAMPLETYPES_H
