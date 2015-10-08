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
    return entry->createDeserializer();
}

std::unique_ptr<IClassSerializer> SerializerRegistry::getSerializer(const std::string& typeName)
{
    auto* entry = getMetaObject(typeName);
    return entry->createSerializer();
}


} /* namespace joynr */
