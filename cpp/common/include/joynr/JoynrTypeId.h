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
#ifndef TYPEID_H
#define TYPEID_H

#include <string>
#include <cassert>

namespace joynr
{

/**
 * @brief A holder of TypeIds (TypeId is std::string)
 */
template <typename T>
class JoynrTypeId
{
public:
    /**
     * @brief JoynrTypeId
     */
    JoynrTypeId();
    /**
     * @brief instance returns unique instance of particular JoynrTypeId specialization
     * It is not possible two JoynrTypeId's for the same TypeId
     * @return
     */
    static JoynrTypeId& instance();
    /**
     * @brief setTypeId
     * @param name
     */
    static void create(const std::string& typeName);
    /**
     * @brief getTypeId
     * @return
     */
    static int getTypeId();
    /**
     * @brief getTypeName
     * @return
     */
    static std::string getTypeName();

private:
    int typeId;
    std::string typeName;
};

template <typename T>
JoynrTypeId<T>::JoynrTypeId()
        : typeId(0), typeName()
{
}

template <typename T>
JoynrTypeId<T>& JoynrTypeId<T>::instance()
{
    static JoynrTypeId<T> singleton;
    return singleton;
}

template <typename T>
int JoynrTypeId<T>::getTypeId()
{
    return instance().typeId;
}

template <typename T>
std::string JoynrTypeId<T>::getTypeName()
{
    return instance().typeName;
}

template <typename T>
void JoynrTypeId<T>::create(const std::string& typeName)
{
    assert(getTypeId() == 0);
    instance().typeId = std::hash<std::string>()(typeName);
    instance().typeName = typeName;
    assert(getTypeId() != 0);
}

/**
 * This might be useful if we decide to use some other sources for TypeId,
 * like RTTI, or compiler __PRETTY_FUNCTION__, for now it is turned off.
 */
#if 0
// Defines a TypeId for classes that require it.
// Can optionally use RTTI if available.

// __GXX_RTTI is set by g++, icc and clang
#ifdef __GXX_RTTI
#define USE_RTTI
#endif

#ifdef USE_RTTI
//---- Type id that uses RTTI --------------------------------------------------

#include <typeindex>
#include <utility>

typedef std::type_index TypeId;
#else

//---- Type id that does not use RTTI ------------------------------------------

typedef const char * TypeId;

// Type id that does not use RTTI
template <typename T>
typename TypeInfo
{
public:
    static TypeId typeId;

    static TypeId getTypeId() {
        // __PRETTY_FUNCTION__ includes template type information and is
        // a pointer to a static char[]
        return __PRETTY_FUNCTION__;
    }
};

template <typename T>
TypeId TypeInfo<T>::typeId = TypeInfo<T>::getTypeId();

#endif

// Function that returns the type id of template type T
template <typename T>
TypeId getTemplateTypeId()
{
#ifdef USE_RTTI
        return std::type_index(typeid(T));
#else
        return TypeInfo<T>::typeId;
#endif
}
#endif

} // namespace joynr
#endif // TYPEID_H
