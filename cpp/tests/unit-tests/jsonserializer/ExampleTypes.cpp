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
#include "ExampleTypes.h"
#include "joynr/SerializerRegistry.h"

#include <string>
#include <vector>

// Everything here should be generated!!

using namespace joynr;

// Register types in type registry

// 3.7.1 Static storage duration
//
// 2. If an object of static storage duration has initialization or a destructor
//    with side effects, it shall not be eliminated even if it appears to be unused,
//    except that a class object or its copy may be eliminated as specified in 12.8.
static const bool isSomeTypeRegistered =
        Variant::registerType<SomeType>("joynr.SomeType");
static const bool isSomeTypeSerializerRegistered =
        SerializerRegistry::registerType<SomeType>("joynr.SomeType");
static const bool isOtherSomeTypeRegistered =
        Variant::registerType<SomeOtherType>("joynr.SomeOtherType");
static const bool isOtherSomeTypeSerializerRegistered =
        SerializerRegistry::registerType<SomeOtherType>("joynr.SomeOtherType");
static const bool isExampleMasterAccessControlEntryRegistered =
        Variant::registerType<ExampleMasterAccessControlEntry>("joynr.infrastructure.ExampleMasterAccessControlEntry");
static const bool isExampleMasterAccessControlEntrySerializerRegistered =
        SerializerRegistry::registerType<ExampleMasterAccessControlEntry>("joynr.infrastructure.ExampleMasterAccessControlEntry");

// A deserializer for the empty type -------------------------------------------
template <>
void ClassDeserializer<SomeType>::deserialize(SomeType& t, IObject& o)
{
}

template <>
void ClassSerializer<SomeType>::serialize(const SomeType& t, std::ostream& stream)
{
    // Empty
}

// A deserializer for the simple type ------------------------------------------
template <>
void ClassDeserializer<SomeOtherType>::deserialize(SomeOtherType& t, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "a") {
            t.setA(field.value().getIntType<int32_t>());
        }
    }
}

template <>
void ClassSerializer<SomeOtherType>::serialize(const SomeOtherType& t, std::ostream& stream)
{
    stream << "{";
    stream << "\"_typeName\": \"" << JoynrTypeId<SomeOtherType>::getTypeName() << "\",";
    stream << "\"a\": " << t.getA();
    stream << "}";
}

// A deserializer for MasterAccessControlEntry --------------------------------

ExamplePermission::Enum convertToExamplePermissionEnum(IValue& value)
{
    std::string text = value;

    if (text == "YES") {
        return ExamplePermission::YES;
    } else if (text == "NO") {
        return ExamplePermission::NO;
    } else if (text == "ASK") {
        return ExamplePermission::ASK;
    } else {
        throw std::invalid_argument("Unknown enum value");
    }
}

template <>
void ClassDeserializer<ExampleMasterAccessControlEntry>::deserialize(
        ExampleMasterAccessControlEntry& t, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "operation") {
            t.setOperation(field.value());
        } else if (field.name() == "defaultConsumerPermission") {
            ExamplePermission::Enum e = convertToExamplePermissionEnum(field.value());
            t.setDefaultConsumerPermission(e);
        } else if (field.name() == "possibleConsumerPermissions"){
            IArray& array = field.value();
            auto convertedArray = convertArray<ExamplePermission::Enum>(array, convertToExamplePermissionEnum);
            t.setPossibleConsumerPermissions(convertedArray);
        }
    }
}

template <>
void ClassSerializer<ExampleMasterAccessControlEntry>::serialize(const ExampleMasterAccessControlEntry& t, std::ostream& stream)
{
    // TODO: implement
}

#if SERIALIZER
template <>
void Serializer<ExampleMasterAccessControlEntry>(const ExampleMasterAccessControlEntry& entry)
{
    serializeStringField("operation",entry.getOperation());
    serializeEnumField("defaultConsumerPermission", entry.getDefaultConsumerPermission());
}
#endif

#if USE_PROTOBUFS
// Example deserializer that uses protobufs
template <> ClassDeserializer<ExampleMasterAccessControlEntry>::deserialize(
        ExampleMasterAccessControlEntry& t, protobuf::ExampleMasterAccessControlEntry& protobuf)
{
    t.setOperation(protobuf.operation());
    t.setDefaultConsumerPermission(protobuf.default_consumer_permission());
    t.setPossibleConsumerPermissions(protobuf.possible_consumer_permissions());
}
#endif

