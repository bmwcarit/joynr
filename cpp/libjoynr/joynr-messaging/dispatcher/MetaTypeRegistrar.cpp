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

#include <cassert>

namespace joynr
{

MetaTypeRegistrar* MetaTypeRegistrar::registrarInstance = nullptr;

MetaTypeRegistrar::MetaTypeRegistrar() : publicationInterpretersMutex()
{
    // Register known types
    registerMetaType<std::string>();
    registerMetaType<bool>();
    registerMetaType<float>();
    registerMetaType<double>();
    registerMetaType<std::int8_t>();
    registerMetaType<std::uint8_t>();
    registerMetaType<std::int16_t>();
    registerMetaType<std::uint16_t>();
    registerMetaType<std::int32_t>();
    registerMetaType<std::uint32_t>();
    registerMetaType<std::int64_t>();
    registerMetaType<std::uint64_t>();
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

} // namespace joynr
