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
#include <QMutex>
#include <QMutexLocker>
#include <QHash>
#include <QMetaType>

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
     * Register an enum metatype
     */
    template <class T>
    void registerEnumMetaType();

    /**
     * Register a composite metatype
     */
    template <class... Ts>
    void registerCompositeMetaType();

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
    template <class T>
    void addEnumReplyInterpreter(int typeId);
    template <class T>
    void addPublicationInterpreter();
    template <class T>
    void addReplyInterpreter();
    template <class... Ts>
    void addPublicationInterpreterForCompositeType();

    // A threadsafe hash holding PublicationInterpreters
    QHash<int, IPublicationInterpreter*> publicationInterpreters;
    QMutex publicationInterpretersMutex;

    // A threadsafe hash holding ReplyInterpreters
    QHash<int, IReplyInterpreter*> replyInterpreters;
    QMutex replyInterpretersMutex;
};

template <class T>
void MetaTypeRegistrar::addEnumPublicationInterpreter(int typeId)
{
    if (!publicationInterpreters.contains(typeId)) {
        publicationInterpreters.insert(typeId, new EnumPublicationInterpreter<T>());
    }
}

template <class T>
void MetaTypeRegistrar::addEnumReplyInterpreter(int typeId)
{
    if (!replyInterpreters.contains(typeId)) {
        replyInterpreters.insert(typeId, new EnumReplyInterpreter<T>());
    }
}

// For enums, the metatype Id is T::Enum or QList<T::Enum>
// However, the publication and reply interpreters must be created as type T or QList<T>
template <class T>
void MetaTypeRegistrar::registerEnumMetaType()
{
    {
        QMutexLocker locker(&publicationInterpretersMutex);
        addEnumPublicationInterpreter<T>(qMetaTypeId<typename T::Enum>());
        addEnumPublicationInterpreter<QList<T>>(qMetaTypeId<QList<typename T::Enum>>());
    }
    {
        QMutexLocker locker(&replyInterpretersMutex);
        addEnumReplyInterpreter<T>(qMetaTypeId<typename T::Enum>());
        addEnumReplyInterpreter<QList<T>>(qMetaTypeId<QList<typename T::Enum>>());
    }
}

template <class T>
void MetaTypeRegistrar::addPublicationInterpreter()
{
    int typeId = qMetaTypeId<T>();

    if (!publicationInterpreters.contains(typeId)) {
        publicationInterpreters.insert(typeId, new PublicationInterpreter<T>());
    }
}

template <class T>
void MetaTypeRegistrar::addReplyInterpreter()
{
    int typeId = qMetaTypeId<T>();

    if (!replyInterpreters.contains(typeId)) {
        replyInterpreters.insert(typeId, new ReplyInterpreter<T>());
    }
}

template <class T>
void MetaTypeRegistrar::registerMetaType()
{
    {
        QMutexLocker locker(&publicationInterpretersMutex);
        addPublicationInterpreter<T>();
        addPublicationInterpreter<QList<T>>();
    }
    {
        QMutexLocker locker(&replyInterpretersMutex);
        addReplyInterpreter<T>();
        addReplyInterpreter<QList<T>>();
    }
}

template <class... Ts>
void MetaTypeRegistrar::registerCompositeMetaType()
{
    {
        QMutexLocker locker(&publicationInterpretersMutex);
        addPublicationInterpreterForCompositeType<Ts...>();
    }
}

template <class... Ts>
void MetaTypeRegistrar::addPublicationInterpreterForCompositeType()
{
    int typeId = Util::getTypeId<Ts...>();

    if (!publicationInterpreters.contains(typeId)) {
        publicationInterpreters.insert(typeId, new BroadcastPublicationInterpreter<Ts...>());
    }
}

} // namespace joynr
#endif // METATYPEREGISTRAR_H
