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
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cstdint>
#include <vector>
#include <set>
#include <thread>
#include <string>
#include <mutex>
#include <atomic>

#include "joynr/BlockingQueue.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class Runnable;

/**
 * @class ThreadPool
 * @brief A container of a fixed number of threads doing work provided
 *      by @ref Runnable
 */
class JOYNR_EXPORT ThreadPool
{
public:
    /**
     * Constructor
     * @param name Name of the hosted threads
     * @param numberOfThreads Number of threads to be allocated and available
     */
    ThreadPool(const std::string& name, const std::uint8_t numberOfThreads);

    /**
     * Destructor
     */
    virtual ~ThreadPool();

    /**
     * @brief Does an ordinary shutdown of @ref ThreadPool
     * @note Must be called before destructor is called
     */
    void shutdown();

    /**
     * Returns the state of the @ref ThreadPool
     * @return @c true if @ref ThreadPool is running
     */
    bool isRunning();

    /**
     * Executes work by adding to the queue
     * @param runnable Runnable to be executed
     */
    void execute(Runnable* runnable);

private:
    /*! Disallow copy and assign */
    DISALLOW_COPY_AND_ASSIGN(ThreadPool);

    /*! Lifecycle for @ref threads */
    void threadLifecycle();

private:
    /*! Logger */
    ADD_LOGGER(ThreadPool);

    /*! Worker threads */
    std::vector<std::thread> threads;

    /*! FIFO queue of work that could be done right now */
    BlockingQueue scheduler;

    /*! Flag indicating @ref threads to keep running */
    std::atomic_bool keepRunning;

    /*! Currently running work in @ref threads */
    std::set<Runnable*> currentlyRunning;

    std::mutex mutex;
};

} // namespace joynr

#endif // THREADPOOL_H
