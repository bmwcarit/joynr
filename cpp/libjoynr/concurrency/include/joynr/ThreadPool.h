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

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

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
class JOYNR_EXPORT ThreadPool : public std::enable_shared_from_this<ThreadPool>
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
     * @brief Does an ordinary init of @ref ThreadPool
     * @note Must be called after constructor is called
     * since it requires shared_ptr to own object
     */
    void init();

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
    void execute(std::shared_ptr<Runnable> runnable);

private:
    /*! Disallow copy and assign */
    DISALLOW_COPY_AND_ASSIGN(ThreadPool);

    /*! Lifecycle for @ref threads */
    void threadLifecycle(std::shared_ptr<ThreadPool> thisSharedptr);

private:
    /*! Logger */
    ADD_LOGGER(ThreadPool)

    /*! Worker threads */
    std::vector<std::thread> _threads;

    /*! FIFO queue of work that could be done right now */
    BlockingQueue _scheduler;

    /*! Flag indicating @ref threads to keep running */
    std::atomic_bool _keepRunning;

    /*! Currently running work in @ref threads */
    std::set<std::shared_ptr<Runnable>> _currentlyRunning;

    std::mutex _mutex;

    std::uint8_t _numberOfThreads;

    std::string _name;
};

} // namespace joynr

#endif // THREADPOOL_H
