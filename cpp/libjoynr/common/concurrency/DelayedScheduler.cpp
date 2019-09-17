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

#include "joynr/Util.h"

namespace joynr
{

DelayedScheduler::DelayedScheduler(std::function<void(std::shared_ptr<Runnable>)> onWorkAvailable,
                                   boost::asio::io_service& ioService,
                                   std::chrono::milliseconds defaultDelayMs)
        : std::enable_shared_from_this<DelayedScheduler>(),
          _defaultDelayMs(defaultDelayMs),
          _onWorkAvailable(onWorkAvailable),
          _stoppingDelayedScheduler(false),
          _delayedRunnables(),
          _writeLock(),
          _nextRunnableHandle(0),
          _ioService(ioService)
{
}

DelayedScheduler::~DelayedScheduler()
{
    JOYNR_LOG_TRACE(logger(), "Dtor called");
    // check if DelayedScheduler::shutdown() was called first
    assert(_delayedRunnables.empty());
}

DelayedScheduler::RunnableHandle DelayedScheduler::schedule(std::shared_ptr<Runnable> runnable,
                                                            std::chrono::milliseconds delay)
{
    std::lock_guard<std::mutex> lock1(_writeLock);

    JOYNR_LOG_TRACE(logger(), "schedule: enter with {} ms delay", delay.count());

    if (_stoppingDelayedScheduler) {
        // if (runnable->isDeleteOnExit()) {
        //    delete runnable;
        //}
        return _INVALID_RUNNABLE_HANDLE;
    }

    if (delay == std::chrono::milliseconds::zero()) {
        JOYNR_LOG_TRACE(logger(), "Forward runnable directly (no delay)");
        _onWorkAvailable(runnable);
        return _INVALID_RUNNABLE_HANDLE;
    }

    RunnableHandle newRunnableHandle = _INVALID_RUNNABLE_HANDLE;

    newRunnableHandle = ++_nextRunnableHandle;

    _delayedRunnables.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(newRunnableHandle),
            std::forward_as_tuple(runnable,
                                  _ioService,
                                  std::chrono::milliseconds(delay),
                                  [
                                    thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
                                    newRunnableHandle
                                  ](const boost::system::error_code& errorCode) {
                auto thisSharedPtr = thisWeakPtr.lock();
                if (!thisSharedPtr) {
                    JOYNR_LOG_ERROR(logger(),
                                    "Failed to schedule delayed runnable because "
                                    "DelayedScheduler no longer exists");
                    return;
                }
                if (!errorCode) {
                    std::lock_guard<std::mutex> lock2(thisSharedPtr->_writeLock);
                    {
                        // Look up the runnable because it might have been removed
                        // by another thread while we were waiting for the mutex.
                        auto it = thisSharedPtr->_delayedRunnables.find(newRunnableHandle);

                        if (it == thisSharedPtr->_delayedRunnables.end()) {
                            JOYNR_LOG_WARN(logger(),
                                           "Timed runnable with ID {} not found.",
                                           newRunnableHandle);
                            return;
                        }

                        if (!thisSharedPtr->_stoppingDelayedScheduler) {
                            std::shared_ptr<Runnable> runnableLocal = it->second.takeRunnable();
                            thisSharedPtr->_onWorkAvailable(runnableLocal);
                        }
                        thisSharedPtr->_delayedRunnables.erase(it);
                    }
                } else if (errorCode != boost::system::errc::operation_canceled) {
                    JOYNR_LOG_ERROR(logger(),
                                    "Failed to schedule delayed runnable: {}",
                                    errorCode.message());
                }
            }));

    JOYNR_LOG_TRACE(logger(), "Added timer with ID {}", newRunnableHandle);

    return newRunnableHandle;
}

DelayedScheduler::RunnableHandle DelayedScheduler::schedule(std::shared_ptr<Runnable> runnable)
{
    return schedule(std::move(runnable), _defaultDelayMs);
}

void DelayedScheduler::unschedule(const RunnableHandle runnableHandle)
{
    if (runnableHandle == _INVALID_RUNNABLE_HANDLE) {
        JOYNR_LOG_WARN(logger(), "unschedule() called with invalid runnable handle");
        return;
    }

    std::lock_guard<std::mutex> lock3(_writeLock);
    {
        auto it = _delayedRunnables.find(runnableHandle);

        if (it == _delayedRunnables.end()) {
            JOYNR_LOG_WARN(logger(), "Timed runnable with ID {} not found.", runnableHandle);
            return;
        }

        _delayedRunnables.erase(it);
    }

    JOYNR_LOG_TRACE(logger(), "runnable with handle {} unscheduled", runnableHandle);
}

void DelayedScheduler::shutdown()
{
    std::lock_guard<std::mutex> lock4(_writeLock);
    _stoppingDelayedScheduler = true;
    _delayedRunnables.clear();
}

} // namespace joynr
