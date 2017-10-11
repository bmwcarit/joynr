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
#include "joynr/ThreadPool.h"

#include <cassert>
#include <functional>
#include <set>

#include "joynr/Runnable.h"

namespace joynr
{

ThreadPool::ThreadPool(const std::string& /*name*/, std::uint8_t numberOfThreads)
        : threads(),
          scheduler(),
          keepRunning(true),
          currentlyRunning(),
          mutex(),
          numberOfThreads(numberOfThreads)
{
}

void ThreadPool::init()
{
    for (std::uint8_t i = 0; i < numberOfThreads; ++i) {
        threads.emplace_back(std::bind(&ThreadPool::threadLifecycle, this, shared_from_this()));
    }

#if 0 // This is not working in g_SystemIntegrationTests
#ifdef linux
    if (name.c_str() != nullptr) {
        for (auto thread = threads.begin(); thread != threads.end(); ++thread) {
            pthread_setname_np(thread->native_handle(), name.c_str());
        }
    }
#endif
#endif
}

ThreadPool::~ThreadPool()
{
    if (keepRunning) {
        shutdown();
    }
    assert(keepRunning == false);
    assert(threads.empty());
}

void ThreadPool::shutdown()
{
    keepRunning = false;

    // Signal scheduler that pending Runnables will not be
    // taken by this ThreadPool
    scheduler.shutdown();

    {
        std::lock_guard<std::mutex> lock(mutex);
        for (std::shared_ptr<Runnable> runnable : currentlyRunning) {
            runnable->shutdown();
        }
    }

    std::set<std::shared_ptr<Runnable>>::size_type maxRunning = 0;
    for (auto thread = threads.begin(); thread != threads.end(); ++thread) {
        // do not cause an abort waiting for ourselves
        if (std::this_thread::get_id() == thread->get_id()) {
            thread->detach();
            maxRunning = 1;
        } else if (thread->joinable()) {
            thread->join();
        }
    }
    threads.clear();

    {
        std::lock_guard<std::mutex> lock(mutex);
        // Runnables should be cleaned in the thread loop
        // except for the thread that runs this code in case
        // it was part of the ThreadPool
        assert(currentlyRunning.size() <= maxRunning);
    }
}

bool ThreadPool::isRunning()
{
    return keepRunning;
}

void ThreadPool::execute(std::shared_ptr<Runnable> runnable)
{
    scheduler.add(runnable);
}

void ThreadPool::threadLifecycle(std::shared_ptr<ThreadPool> thisSharedPtr)
{
    JOYNR_LOG_TRACE(logger(), "Thread enters lifecycle");

    while (thisSharedPtr->keepRunning) {

        JOYNR_LOG_TRACE(logger(), "Thread is waiting");
        // Take a runnable
        std::shared_ptr<Runnable> runnable = scheduler.take();

        if (runnable) {

            JOYNR_LOG_TRACE(logger(), "Thread got runnable and will do work");

            // Add runnable to the queue of currently running context
            {
                std::lock_guard<std::mutex> lock(thisSharedPtr->mutex);
                if (!thisSharedPtr->keepRunning) {
                    break;
                }
                thisSharedPtr->currentlyRunning.insert(runnable);
            }

            // Run the runnable
            runnable->run();

            JOYNR_LOG_TRACE(logger(), "Thread finished work");

            {
                std::lock_guard<std::mutex> lock(thisSharedPtr->mutex);
                thisSharedPtr->currentlyRunning.erase(runnable);
            }
        }
    }

    JOYNR_LOG_TRACE(logger(), "Thread leaves lifecycle");
}

} // namespace joynr
