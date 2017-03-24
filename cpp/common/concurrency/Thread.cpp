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
#include "joynr/Thread.h"

#include <cassert>

namespace joynr
{

Thread::Thread(const std::string& name) : thread(nullptr), name(name)
{
}

Thread::~Thread()
{
    stop();
}

bool Thread::start()
{
    if (thread != nullptr) {
        return false;
    }

    thread = new std::thread(&Thread::run, this);

    assert(thread != nullptr);

#if 0 // This is not working in g_SystemIntegrationTests
#ifdef linux
    if (name.c_str() != nullptr) {
        if (pthread_setname_np(thread->native_handle(), name.c_str()) == 0) {
            return true;
        }
        return false;
    }
#else
    return (thread != nullptr);
#endif
#endif
    return true;
}

void Thread::stop()
{
    if (thread != nullptr) {
        if (thread->joinable()) {
            thread->join();
        }
        delete thread;
        thread = nullptr;
    }
}

} // namespace joynr
