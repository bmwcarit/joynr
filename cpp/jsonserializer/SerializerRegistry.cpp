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
#include "joynr/SerializerRegistry.h"

namespace joynr
{

// Register serializers for basic types
static const bool isInt8SerializerRegistered = SerializerRegistry::registerNativeType<int8_t>("Int8");
static const bool isUInt8SerializerRegistered = SerializerRegistry::registerNativeType<uint8_t>("UInt8");
static const bool isInt16SerializerRegistered = SerializerRegistry::registerNativeType<int16_t>("Int16");
static const bool isUInt16SerializerRegistered = SerializerRegistry::registerNativeType<uint16_t>("UInt16");
static const bool isInt32SerializerRegistered =
        SerializerRegistry::registerNativeType<int32_t>("Int32");
static const bool isIntSerializerRegistered =
        SerializerRegistry::registerNativeType<int>("Integer"); // ambiguous with "Integer", if we use
                                                 // this one only then Dispatcher should
                                                 // be changed
static const bool isUInt32SerializerRegistered = SerializerRegistry::registerNativeType<uint32_t>("UInt32");
static const bool isInt64SerializerRegistered = SerializerRegistry::registerNativeType<int64_t>("Int64");
static const bool isUInt64SerializerRegistered = SerializerRegistry::registerNativeType<uint64_t>("UInt64");
static const bool isDoubleSerializerRegistered = SerializerRegistry::registerNativeType<double>("Double");
static const bool isFloatSerializerRegistered = SerializerRegistry::registerNativeType<float>("Float");
static const bool isBooleanSerializerRegistered = SerializerRegistry::registerNativeType<bool>("Boolean");
static const bool isStringSerializerRegistered = SerializerRegistry::registerNativeType<std::string>("String");

SerializerRegistry::SerializerRegistry() :
    metaObjects(),
    registryMutex()
{
}

SerializerRegistry& SerializerRegistry::getInstance()
{
    // This singleton pattern is thread safe in C++11
    static SerializerRegistry instance;
    return instance;
}

IMetaObject* SerializerRegistry::getMetaObject(const std::string& typeName)
{
    SerializerRegistry& registry = getInstance();
    std::unique_lock<std::mutex> lock(registry.registryMutex);

    auto entry = registry.metaObjects.find(typeName);
    if (entry == registry.metaObjects.end()) {
        throw std::invalid_argument(std::string("TypeId not known :") + typeName);
    }

    return entry->second.get();
}

std::unique_ptr<IClassDeserializer> SerializerRegistry::getDeserializer(const std::string& typeName)
{
    auto* entry = getMetaObject(typeName);
    return entry->createClassDeserializer();
}

std::unique_ptr<IClassSerializer> SerializerRegistry::getSerializer(const std::string& typeName)
{
    auto* entry = getMetaObject(typeName);
    return entry->createSerializer();
}

std::unique_ptr<IEnumDeserializer> SerializerRegistry::getEnumDeserializer(const std::string& typeName)
{
    auto* entry = getMetaObject(typeName);
    return entry->createEnumDeserializer();
}

} /* namespace joynr */
