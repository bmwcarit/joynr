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

#ifndef THREADLOCALCONTEXTSTORAGE_H
#define THREADLOCALCONTEXTSTORAGE_H

namespace joynr
{

template <typename T>
class ThreadLocalContextStorage
{
public:
    static void set(const T& context)
    {
        get() = context;
    }

    static void set(T&& context)
    {
        get() = std::move(context);
    }

    static T& get()
    {
        static thread_local T context;
        return context;
    }

    static void invalidate()
    {
        get().invalidate();
    }
};

} // namespace joynr

#endif // THREADLOCALCONTEXTSTORAGE_H
