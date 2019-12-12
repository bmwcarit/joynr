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
#ifndef DELAYEDSCHEDULER_H
#define DELAYEDSCHEDULER_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/DelayedRunnable.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

/**
 * @class DelayedScheduler
 * @brief Using a @ref Timer and @ref BlockingQueue to execute a runnable delayed
 */
class JOYNR_EXPORT DelayedScheduler : public std::enable_shared_from_this<DelayedScheduler>
{
public:
    /*! Handle to reference a @ref Runnable scheduled to work */
    typedef std::uint32_t RunnableHandle;

    /*! Invalid handle */
    static const RunnableHandle _INVALID_RUNNABLE_HANDLE = 0;

    /**
     * @brief Constructor
     * @param onWorkAvailable Callback done if work is available after timer
     *      has expired
     * @param defaultDelayMs Default delay used by @ref schedule
     */
    DelayedScheduler(std::function<void(std::shared_ptr<Runnable>)> onWorkAvailable,
                     boost::asio::io_service& ioService,
                     std::chrono::milliseconds defaultDelayMs = std::chrono::milliseconds::zero());

    /**
     * @brief Destructor
     * @note Be sure to call @ref shutdown and wait for return before
     *      destroying this object
     */
    virtual ~DelayedScheduler();

    /**
     * @brief Schedule a @ref Runnable to be added to execution queue
     * @param runnable Runnable to be added to queue
     * @param delay Number of milliseconds to delay adding the @ref Runnable
     *      to the queue.
     *      If this parameter value is invalid (aka null) its default value will be used.
     * @return Handle referencing the given @ref Runnable in this scheduler. If
     *      @ref INVALID_RUNNABLE_HANDLE is returned, the @ref Runnable either
     *      is directly submitted to run or the @ref DelayedScheduler is already
     *      shutting down.
     */
    virtual RunnableHandle schedule(std::shared_ptr<Runnable> runnable,
                                    std::chrono::milliseconds delay);

    /**
     * @brief Schedule a @ref Runnable to be added to execution queue
     *
     * This method calls @ref schedule with the default delay.
     *
     * @param runnable Runnable to be added to queue
     * @return Handle referencing the given @ref Runnable in this scheduler. If
     *      @ref INVALID_RUNNABLE_HANDLE is returned, the @ref Runnable either
     *      is directly submitted to run or the @ref DelayedScheduler is already
     *      shutting down.
     */
    virtual RunnableHandle schedule(std::shared_ptr<Runnable> runnable);

    /**
     * @brief Try to remove a @ref Runnable while waiting
     * @param runnableHandle Handle given by @ref schedule
     */
    virtual void unschedule(const RunnableHandle runnableHandle);

    /**
     * @brief Does an ordinary shutdown of @ref DelayedScheduler
     * @note Must be called before destructor is called
     */
    virtual void shutdown();

protected:
    /*! Logger */
    ADD_LOGGER(DelayedScheduler)

private:
    /*! @ref DelayedScheduler is not allowed to be copied */
    DISALLOW_COPY_AND_ASSIGN(DelayedScheduler);

private:
    /*! Default delay set by the constructor */
    const std::chrono::milliseconds _defaultDelayMs;

    /*! Callback on timer expired */
    std::function<void(std::shared_ptr<Runnable>)> _onWorkAvailable;

    /*! Flag indicating @ref DelayedScheduler will be stopped */
    bool _stoppingDelayedScheduler;

    std::unordered_map<RunnableHandle, DelayedRunnable> _delayedRunnables;

    /*! Guard to limit write access to @ref delayedRunnables */
    std::mutex _writeLock;

    /*! Next runnable handle which will be returned by ::schedule */
    RunnableHandle _nextRunnableHandle;

    /*! Used for async timers. */
    boost::asio::io_service& _ioService;
};

} // namespace joynr
#endif // DELAYEDSCHEDULER_H
