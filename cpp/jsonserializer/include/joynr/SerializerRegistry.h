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
#ifndef SerializerRegistry_H
#define SerializerRegistry_H

#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>

#include "joynr/JoynrTypeId.h"
#include "ClassDeserializer.h"
#include "EnumDeserializer.h"
#include "ClassSerializer.h"

namespace joynr
{
/**
 * @brief The MetaObject abstract class
 */
class IMetaObject
{
public:
    /**
     * @brief ~IMetaObject
     */
    virtual ~IMetaObject() = default;
    /**
     * @brief createDeserializer
     * @return
     */
    virtual std::unique_ptr<IClassDeserializer> createClassDeserializer()
    {
        // By default there is no deserializer for types - they are treated as string
        // for conversion to Variant
        return nullptr;
    }

    virtual std::unique_ptr<IEnumDeserializer> createEnumDeserializer()
    {
        // By default there is no deserializer for types - they are treated as string
        // for conversion to Variant
        return nullptr;
    }

    /**
     * @brief createSerializer
     * @return
     */
    virtual std::unique_ptr<IClassSerializer> createSerializer() = 0;
};

/**
 * @brief Class Type specific meta objects
 */
template <class T>
class ClassMetaObjectType : public IMetaObject
{
public:
    std::unique_ptr<IClassDeserializer> createClassDeserializer()
    {
        return std::unique_ptr<IClassDeserializer>(new ClassDeserializer<T>());
    }

    std::unique_ptr<IClassSerializer> createSerializer()
    {
        return std::unique_ptr<IClassSerializer>(new ClassSerializer<T>());
    }
};

/**
 * @brief Enum Type specific meta objects
 */
template <class T>
class EnumMetaObjectType : public IMetaObject
{
public:
    std::unique_ptr<IEnumDeserializer> createEnumDeserializer()
    {
        return std::unique_ptr<IEnumDeserializer>(new EnumDeserializer<T>());
    }

    std::unique_ptr<IClassSerializer> createSerializer()
    {
        return std::unique_ptr<IClassSerializer>(new ClassSerializer<T>());
    }
};

/**
 * @brief Native Type specific meta objects
 */
template <class T>
class NativeMetaObjectType : public IMetaObject
{
public:
    std::unique_ptr<IClassSerializer> createSerializer()
    {
        return std::unique_ptr<IClassSerializer>(new ClassSerializer<T>());
    }
};

//---- Type registry that creates objects from their names ---------------------
/**
 * @brief The SerializerRegistry class keeps track of registered types and it can create
 * objects based on their TypeId (name)
 */
class SerializerRegistry
{
public:
    /**
     * @brief Register type T serializer/deserializer
     * @param typeName
     */
    template <class T>
    static bool registerType(const std::string& typeName);
    /**
     * @brief Register native type T serializer only
     * @param typeName
     */
    template <class T>
    static bool registerNativeType(const std::string& typeName);

    /**
     * @brief Register enum type serializer/deserializer
     * @param typeName
     */
    template <class T>
    static bool registerEnum(const std::string& typeName);
    /**
     * @brief getDeserializer Create a deserializer from the given typeId
     * @param typeId
     * @return A deserializer for the type with the given typeId
     */
    static std::unique_ptr<IClassDeserializer> getDeserializer(const std::string& typeName);
    /**
     * @brief getSerializer Create a serializer from the given typeName
     * @param typeName
     * @return A deserializer for the type with the given typeName
     */
    static std::unique_ptr<IClassSerializer> getSerializer(const std::string& typeName);
    /**
     * @brief getEnumSerializer Create a serializer from the given typeName
     * @param typeName
     * @return A deserializer for the type with the given typeName
     */
    static std::unique_ptr<IEnumDeserializer> getEnumDeserializer(const std::string& typeName);

    SerializerRegistry(const SerializerRegistry&) = delete;
    void operator=(const SerializerRegistry&) = delete;
private:
    std::unordered_map<std::string,std::unique_ptr<IMetaObject>> metaObjects;
    std::mutex registryMutex;

    SerializerRegistry();

    static SerializerRegistry& getInstance();
    static IMetaObject *getMetaObject(const std::string& typeName);
};

template <class T>
bool SerializerRegistry::registerType(const std::string& typeName)
{
    // Create serializer/deserializer for given type
    {
        SerializerRegistry& registry = getInstance();
        std::unique_lock<std::mutex> lock(registry.registryMutex);

        registry.metaObjects[typeName] = std::unique_ptr<IMetaObject>(new ClassMetaObjectType<T>());
    }
    return true;
}

template <class T>
bool SerializerRegistry::registerNativeType(const std::string& typeName)
{
    // Create only serializer for native types
    {
        SerializerRegistry& registry = getInstance();
        std::unique_lock<std::mutex> lock(registry.registryMutex);

        registry.metaObjects[typeName] = std::unique_ptr<IMetaObject>(new NativeMetaObjectType<T>());
    }
    return true;
}

template <class T>
bool SerializerRegistry::registerEnum(const std::string& typeName)
{
    // Create special enum serializer/deserializer for given enum
    {
        SerializerRegistry& registry = getInstance();
        std::unique_lock<std::mutex> lock(registry.registryMutex);

        registry.metaObjects[typeName] = std::unique_ptr<IMetaObject>(new EnumMetaObjectType<T>());
    }
    return true;
}

} /* namespace joynr */
#endif // SerializerRegistry_H
