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
#ifndef SINGLETHREADEDDELAYEDSCHEDULER_H
#define SINGLETHREADEDDELAYEDSCHEDULER_H

#include <chrono>
#include <atomic>
#include <mutex>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrCommonExport.h"
#include "joynr/Logger.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/Thread.h"
#include "joynr/BlockingQueue.h"

namespace boost
{
namespace asio
{
class io_service;
} // namespace asio
} // namespace boost

namespace joynr
{

class Runnable;

/**
 * @brief A single threaded implementation of the @ref DelayedScheduler
 * @note This implementation should not be used for runnables that take
 *      substantial time to complete.
 */
class JOYNRCOMMON_EXPORT SingleThreadedDelayedScheduler : public DelayedScheduler, private Thread
{
public:
    /**
     * @brief Constructor
     * @param threadName Name of the thread to be used for debugging reasons
     * @param defaultDelayMs Default delay used by @ref DelayedScheduler::schedule
     */
    explicit SingleThreadedDelayedScheduler(
            const std::string& threadName,
            boost::asio::io_service& ioService,
            std::chrono::milliseconds defaultDelayMs = std::chrono::milliseconds::zero());

    /**
     * @brief Destructor
     * @note Be sure to call @ref shutdown and wait for return before
     *      destroying this object
     */
    ~SingleThreadedDelayedScheduler() override;

    /**
     * @brief Does an ordinary shutdown of @ref SingleThreadedDelayedScheduler
     *      and its parents @ref DelayedScheduler and @ref Thread
     * @note Must be called before destructor is called
     */
    void shutdown() override;

private:
    /* Thread pure virtual function */
    void run() override;

    /*! @ref SingleThreadedDelayedScheduler is not allowed to be copied */
    DISALLOW_COPY_AND_ASSIGN(SingleThreadedDelayedScheduler);

private:
    /*! Logger */
    ADD_LOGGER(SingleThreadedDelayedScheduler);

    /*! Flag signaling @ref Thread to keep running */
    std::atomic_bool keepRunning;

    /*! Pointer to the currently running context */
    Runnable* currentlyRunning;

    /*! Queue of waiting work */
    BlockingQueue queue;

    std::mutex mutex;
};
} // namespace joynr

#endif // SINGLETHREADEDDELAYEDSCHEDULER_H
