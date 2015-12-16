/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#ifndef METATYPEREGISTRAR_H
#define METATYPEREGISTRAR_H

#include "joynr/JoynrExport.h"

#include "joynr/PublicationInterpreter.h"
#include "joynr/ReplyInterpreter.h"
#include <mutex>
#include <unordered_map>

#include "joynr/JoynrTypeId.h"

namespace joynr
{

/**
 * A class that registers metatypes so that ReplyInterpreters and
 * PublicationInterpreters can be instantiated.
 */
class JOYNR_EXPORT MetaTypeRegistrar
{
public:
    /**
      * This class is implemented as a singleton. This method gets the instance
      * of the MetaTypeRegistrar.
      * It returns a reference to enforce that the caller does not have ownership.
      * The singleton instance is created lazy during the first call. It is valid
      * for the whole lifetime of the application and gets deleted when the process
      * shuts down.
      *
      * @return a reference to the singleton instance.
      */
    static MetaTypeRegistrar& instance();

    /**
     * Register a metatype
     */
    template <class T>
    void registerMetaType();

    /**
     * Register a metatype
     */
    template <class T1, class T2, class... Ts>
    void registerMetaType();

    /**
     * Register an enum metatype
     */
    template <class T>
    void registerEnumMetaType();

    /**
     * Register a composite reply metatype
     */
    template <class... Ts>
    void registerReplyMetaType();

    /**
     * Get the publication interpreter with the given type id.
     * Returns a reference to enforce that the caller does not have ownership.
     * Publication interpreters are created and registered with the metatype registrar
     * in the I&lt;Interface&gt; constructor when a &lt;Interface&gt;Proxy is created.
     * They are valid for the whole lifetime of the application and get deleted when
     * the process shuts down.
     *
     * @return a reference to the publication interpreter
     */
    IPublicationInterpreter& getPublicationInterpreter(int typeId);

    /**
     * Get the reply interpreter with the given type id.
     * Returns a reference to enforce that the caller does not have ownership.
     * Reply interpreters are created and registered with the metatype registrar
     * in the I&lt;Interface&gt; constructor when a &lt;Interface&gt;Proxy is created.
     * They are valid for the whole lifetime of the application and get deleted when
     * the process shuts down.
     *
     * @return a reference to the reply interpreter
     */
    IReplyInterpreter& getReplyInterpreter(int typeId);

private:
    MetaTypeRegistrar();
    DISALLOW_COPY_AND_ASSIGN(MetaTypeRegistrar);

    static MetaTypeRegistrar* registrarInstance;

    // Helper functions that add publication and reply interpreters
    template <class T>
    void addEnumPublicationInterpreter(int typeId);
    template <class... Ts>
    void addPublicationInterpreter();
    template <class... Ts>
    void addReplyInterpreterForReplyType();
    template <class... Ts>
    void addPublicationInterpreterForBroadcastType();

    // A threadsafe hash holding PublicationInterpreters
    std::unordered_map<int, IPublicationInterpreter*> publicationInterpreters;
    std::mutex publicationInterpretersMutex;

    // A threadsafe hash holding ReplyInterpreters
    std::unordered_map<int, IReplyInterpreter*> replyInterpreters;
    std::mutex replyInterpretersMutex;
};

template <class T>
void MetaTypeRegistrar::addEnumPublicationInterpreter(int typeId)
{
    if (publicationInterpreters.find(typeId) == publicationInterpreters.end()) {
        publicationInterpreters.insert({typeId, new EnumPublicationInterpreter<T>()});
    }
}

// For enums, the metatype Id is T::Enum or std::vector<T::Enum>
// However, the publication and reply interpreters must be created as type T or QList<T>
template <class T>
void MetaTypeRegistrar::registerEnumMetaType()
{
    {
        std::lock_guard<std::mutex> lock(publicationInterpretersMutex);
        addEnumPublicationInterpreter<T>(Util::getTypeId<typename T::Enum>());
        addEnumPublicationInterpreter<std::vector<T>>(
                Util::getTypeId<std::vector<typename T::Enum>>());
    }
}

template <class T>
void MetaTypeRegistrar::registerMetaType()
{
    {
        std::lock_guard<std::mutex> lock(publicationInterpretersMutex);
        addPublicationInterpreter<T>();
        addPublicationInterpreter<std::vector<T>>();
    }
}

template <class T1, class T2, class... Ts>
void MetaTypeRegistrar::registerMetaType()
{
    {
        std::lock_guard<std::mutex> lock(publicationInterpretersMutex);
        addPublicationInterpreter<T1, T2, Ts...>();
    }
}

template <class... Ts>
void MetaTypeRegistrar::addPublicationInterpreter()
{
    int typeId = Util::getTypeId<Ts...>();

    if (publicationInterpreters.find(typeId) == publicationInterpreters.end()) {
        publicationInterpreters.insert({typeId, new PublicationInterpreter<Ts...>()});
    }
}

template <class... Ts>
void MetaTypeRegistrar::registerReplyMetaType()
{
    {
        std::lock_guard<std::mutex> lock(replyInterpretersMutex);
        int typeId = Util::getTypeId<Ts...>();

        if (replyInterpreters.find(typeId) == replyInterpreters.end()) {
            replyInterpreters.insert({typeId, new ReplyInterpreter<Ts...>()});
        }
    }
}

} // namespace joynr
#endif // METATYPEREGISTRAR_H
