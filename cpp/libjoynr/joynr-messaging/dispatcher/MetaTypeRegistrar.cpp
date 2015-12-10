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
    registerMetaType<std::string>();
    registerMetaType<bool>();
    registerMetaType<float>();
    registerMetaType<double>();
    registerMetaType<int8_t>();
    registerMetaType<uint8_t>();
    registerMetaType<int16_t>();
    registerMetaType<uint16_t>();
    registerMetaType<int32_t>();
    registerMetaType<uint32_t>();
    registerMetaType<int64_t>();
    registerMetaType<uint64_t>();

    // Register a reply interpreter for void type
    std::lock_guard<std::mutex> lock(replyInterpretersMutex);
    replyInterpreters.insert({Util::getTypeId<void>(), new ReplyInterpreter<void>()});
}

MetaTypeRegistrar& MetaTypeRegistrar::instance()
{
    static std::mutex mutex;

    // Use double-checked locking so that, under normal use, a
    // mutex lock is not required.
    if (!registrarInstance) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!registrarInstance) {
            registrarInstance = new MetaTypeRegistrar();
        }
    }

    return *registrarInstance;
}

IPublicationInterpreter& MetaTypeRegistrar::getPublicationInterpreter(int typeId)
{
    std::lock_guard<std::mutex> lock(publicationInterpretersMutex);

    IPublicationInterpreter* ret;
    auto search = publicationInterpreters.find(typeId);
    if (search != publicationInterpreters.end()) {
        ret = search->second;
    }

    // It is a programming error if the interpreter does not exist
    assert(ret);
    return *ret;
}

IReplyInterpreter& MetaTypeRegistrar::getReplyInterpreter(int typeId)
{
    std::lock_guard<std::mutex> lock(replyInterpretersMutex);

    IReplyInterpreter* ret;
    auto search = replyInterpreters.find(typeId);
    if (search != replyInterpreters.end()) {
        ret = search->second;
    }

    // It is a programming error if the interpreter does not exist
    assert(ret);
    return *ret;
}

} // namespace joynr
