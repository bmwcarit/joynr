/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#ifndef CONTEXTSTORAGE_H
#define CONTEXTSTORAGE_H

#include <mutex>
#include <utility>

namespace joynr
{

template <typename T>
class ContextStorage
{
public:
    static void set(const T& context)
    {
        std::lock_guard<std::recursive_mutex> lock(getMutex());
        get() = context;
    }

    static void set(T&& context)
    {
        std::lock_guard<std::recursive_mutex> lock(getMutex());
        get() = std::move(context);
    }

    static T& get()
    {
        std::lock_guard<std::recursive_mutex> lock(getMutex());
        static T context;
        return context;
    }

    static std::recursive_mutex& getMutex()
    {
        static std::recursive_mutex mutex;
        return mutex;
    }

    static void invalidate()
    {
        std::lock_guard<std::recursive_mutex> lock(getMutex());
        get().invalidate();
    }
};

} // namespace joynr

#endif // CONTEXTSTORAGE_H
