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
#include "joynr/MetaTypeRegistrar.h"
#include "joynr/DeclareMetatypeUtil.h"

namespace joynr
{

MetaTypeRegistrar* MetaTypeRegistrar::registrarInstance = 0;

MetaTypeRegistrar::MetaTypeRegistrar()
        : publicationInterpreters(),
          publicationInterpretersMutex(),
          replyInterpreters(),
          replyInterpretersMutex()
{
    // Register known types
    registerMetaType<QString>();
    registerMetaType<bool>();
    registerMetaType<int>();
    registerMetaType<double>();
    registerMetaType<qint64>();
    registerMetaType<qint8>();

    // Register a reply interpreter for void type
    QMutexLocker locker(&replyInterpretersMutex);
    replyInterpreters.insert(Util::getTypeId<void>(), new ReplyInterpreter<void>());
}

MetaTypeRegistrar& MetaTypeRegistrar::instance()
{
    static QMutex mutex;

    // Use double-checked locking so that, under normal use, a
    // mutex lock is not required.
    if (!registrarInstance) {
        QMutexLocker locker(&mutex);
        if (!registrarInstance) {
            registrarInstance = new MetaTypeRegistrar();
        }
    }

    return *registrarInstance;
}

IPublicationInterpreter& MetaTypeRegistrar::getPublicationInterpreter(int typeId)
{
    QMutexLocker locker(&publicationInterpretersMutex);

    IPublicationInterpreter* ret = publicationInterpreters.value(typeId);

    // It is a programming error if the interpreter does not exist
    assert(ret);
    return *ret;
}

IReplyInterpreter& MetaTypeRegistrar::getReplyInterpreter(int typeId)
{
    QMutexLocker locker(&replyInterpretersMutex);

    IReplyInterpreter* ret = replyInterpreters.value(typeId);

    // It is a programming error if the interpreter does not exist
    assert(ret);
    return *ret;
}

} // namespace joynr
