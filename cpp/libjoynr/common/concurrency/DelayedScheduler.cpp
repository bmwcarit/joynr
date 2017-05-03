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
#include "joynr/DelayedScheduler.h"

#include <cassert>
#include <utility>

#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>

namespace joynr
{

INIT_LOGGER(DelayedScheduler);

DelayedScheduler::DelayedScheduler(std::function<void(Runnable*)> onWorkAvailable,
                                   boost::asio::io_service& ioService,
                                   std::chrono::milliseconds defaultDelayMs)
        : defaultDelayMs(defaultDelayMs),
          onWorkAvailable(onWorkAvailable),
          stoppingDelayedScheduler(false),
          delayedRunnables(),
          writeLock(),
          nextRunnableHandle(0),
          ioService(ioService)
{
}

DelayedScheduler::~DelayedScheduler()
{
    JOYNR_LOG_TRACE(logger, "Dtor called");
    // check if DelayedScheduler::shutdown() was called first
    assert(delayedRunnables.empty());
}

DelayedScheduler::RunnableHandle DelayedScheduler::schedule(Runnable* runnable,
                                                            std::chrono::milliseconds delay)
{
    std::lock_guard<std::mutex> lock(writeLock);

    JOYNR_LOG_TRACE(logger, "schedule: enter with {} ms delay", delay.count());

    if (stoppingDelayedScheduler) {
        if (runnable->isDeleteOnExit()) {
            delete runnable;
        }
        return INVALID_RUNNABLE_HANDLE;
    }

    if (delay == std::chrono::milliseconds::zero()) {
        JOYNR_LOG_TRACE(logger, "Forward runnable directly (no delay)");
        onWorkAvailable(runnable);
        return INVALID_RUNNABLE_HANDLE;
    }

    RunnableHandle newRunnableHandle = INVALID_RUNNABLE_HANDLE;

    newRunnableHandle = ++nextRunnableHandle;

    delayedRunnables.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(newRunnableHandle),
            std::forward_as_tuple(
                    std::unique_ptr<Runnable>(runnable),
                    ioService,
                    std::chrono::milliseconds(delay),
                    [this, newRunnableHandle](const boost::system::error_code& errorCode) {
                        if (!errorCode) {
                            std::lock_guard<std::mutex> lock(this->writeLock);
                            {
                                // Look up the runnable because it might have been removed
                                // by another thread while we were waiting for the mutex.
                                auto it = this->delayedRunnables.find(newRunnableHandle);

                                if (it == this->delayedRunnables.end()) {
                                    JOYNR_LOG_WARN(this->logger,
                                                   "Timed runnable with ID {} not found.",
                                                   newRunnableHandle);
                                    return;
                                }

                                if (!this->stoppingDelayedScheduler) {
                                    Runnable* runnable = it->second.takeRunnable();
                                    this->onWorkAvailable(runnable);
                                }
                                this->delayedRunnables.erase(it);
                            }
                        } else if (errorCode != boost::system::errc::operation_canceled) {
                            JOYNR_LOG_ERROR(this->logger,
                                            "Failed to schedule delayed runnable: {}",
                                            errorCode.message());
                        }
                    }));

    JOYNR_LOG_TRACE(logger, "Added timer with ID {}", newRunnableHandle);

    return newRunnableHandle;
}

DelayedScheduler::RunnableHandle DelayedScheduler::schedule(Runnable* runnable)
{
    return schedule(runnable, defaultDelayMs);
}

void DelayedScheduler::unschedule(const RunnableHandle runnableHandle)
{
    if (runnableHandle == INVALID_RUNNABLE_HANDLE) {
        JOYNR_LOG_WARN(logger, "unschedule() called with invalid runnable handle");
        return;
    }

    std::lock_guard<std::mutex> lock(writeLock);
    {
        auto it = delayedRunnables.find(runnableHandle);

        if (it == delayedRunnables.end()) {
            JOYNR_LOG_WARN(logger, "Timed runnable with ID {} not found.", runnableHandle);
            return;
        }

        delayedRunnables.erase(it);
    }

    JOYNR_LOG_TRACE(logger, "runnable with handle {} unscheduled", runnableHandle);
}

void DelayedScheduler::shutdown()
{
    std::lock_guard<std::mutex> lock(writeLock);
    stoppingDelayedScheduler = true;
    delayedRunnables.clear();
}

} // namespace joynr
