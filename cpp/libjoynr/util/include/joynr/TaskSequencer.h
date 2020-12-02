/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

#ifndef TASK_SEQUENCER
#define TASK_SEQUENCER

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "joynr/exceptions/JoynrException.h"

#include "joynr/Future.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/TimePoint.h"

namespace joynr
{

/**
 * @brief Queues async-tasks creating JOYNR Futures and processes them in sequence
 *
 * This class allows the sequential creation and processing of asynchronous tasks using JOYNR Future
 * for monitoring task status.
 *
 * @note
 * To avoid implicit runtime changes in the sequence of DTOR calls on shutdown, the provided cancel
 * method releases ownership of all shared memory captured by pending tasks or futures.
 *
 * @note
 * The TaskSequencer cancels queued tasks as soon as they are expired: It calls the timeout()
 * function provided in TaskWithExpiryDate and removes the expired tasks from the queue.
 * TaskSequencer assumes that the tasks which do not have an unlimited expiry
 * (= TimePoint::max() ) date are ordered, i.e. a new task that is added should
 * have an expiryDate that is not less than the expiryDate of any previously
 * added task with not unlimited expiry date.
 *
 * @tparam Ts Future return types
 */
template <class... Ts>
class TaskSequencer
{
public:
    /** Shared JOYNR future */
    using FutureSP = std::shared_ptr<Future<Ts...>>;

    /**
     * @brief Task function creating shared futures
     * @return Shared future
     */
    using Task = std::function<FutureSP()>;

    struct TaskWithExpiryDate
    {
        Task _task;
        TimePoint _expiryDate;
        std::function<void()> _timeout;
    };

    /**
     * @brief Creates queue processor.
     * @param defaultTimeToWait - if there are no further tasks in the queue,
     *        this time in milliseconds is used to wait for the current task to
     *        complete until the queue is checked for expired tasks again even if
     *        the current task has not completed yet. If the queue is not empty,
     *        the wait time is calculated from the task with the minimum expiry date.
     */
    TaskSequencer(std::chrono::milliseconds defaultTimeToWait = std::chrono::milliseconds::max())
            : _isRunning{true}, _future{nothingToDo()}, _defaultTimeToWait{defaultTimeToWait}
    {
        _worker = std::thread(&TaskSequencer::run, this);
    }

    /** Cancel processing of tasks. Drop all enqueued tasks silently. */
    virtual ~TaskSequencer()
    {
        cancel();
    }

    DISALLOW_COPY_AND_ASSIGN(TaskSequencer);
    DISALLOW_MOVE_AND_ASSIGN(TaskSequencer);

    /**
     * @brief add tasks to processor FIFO queue.
     * @param taskWithExpiryDate New task with expiry date
     */
    virtual void add(const TaskWithExpiryDate& taskWithExpiryDate)
    {
        std::unique_lock<std::mutex> lock(_tasksMutex);
        _tasks.push_back(taskWithExpiryDate);
        _tasksChanged.notify_all();
    }

    /**
     * @brief Cancel all task processing
     */
    virtual void cancel()
    {
        if (!_isRunning.exchange(false)) {
            return;
        }

        if (std::future_status::ready !=
            _future->_resultFuture.wait_for(std::chrono::milliseconds(0))) {
            _future->onError(std::make_shared<exceptions::JoynrRuntimeException>(
                    "All tasks have been canceled."));
        }

        {
            std::unique_lock<std::mutex> lock(_tasksMutex);
            std::vector<TaskWithExpiryDate> releasePendingTasksMemory;
            _tasks.swap(releasePendingTasksMemory);
            _tasksChanged.notify_all();
        }
        _worker.join();

        auto releaseCurrentFutureMemory = nothingToDo();
        _future.swap(releaseCurrentFutureMemory);
    }

private:
    std::atomic_bool _isRunning;
    FutureSP _future;
    std::mutex _tasksMutex;
    std::condition_variable _tasksChanged;
    std::vector<TaskWithExpiryDate> _tasks;
    std::thread _worker;
    const std::chrono::milliseconds _defaultTimeToWait;

    void run()
    {
        while (_isRunning.load()) {
            std::chrono::milliseconds timeToWaitMs = _defaultTimeToWait;
            {
                std::unique_lock<std::mutex> lock(_tasksMutex);
                if (!_tasks.empty()) {
                    auto taskWithMinExpiryDate = std::min_element(
                            _tasks.begin(),
                            _tasks.end(),
                            [](const TaskWithExpiryDate& first, const TaskWithExpiryDate& second) {
                                return first._expiryDate < second._expiryDate;
                            });

                    timeToWaitMs = taskWithMinExpiryDate->_expiryDate.relativeFromNow();

                    if (taskWithMinExpiryDate->_expiryDate.toMilliseconds() <=
                        TimePoint::now().toMilliseconds()) {
                        taskWithMinExpiryDate->_timeout();
                        _tasks.erase(taskWithMinExpiryDate);
                        continue;
                    }
                }
            }
            auto futureStatus = _future->_resultFuture.wait_for(timeToWaitMs);

            if (futureStatus != std::future_status::ready) {
                std::unique_lock<std::mutex> lock(_tasksMutex);
                for (auto it = _tasks.begin(); it != _tasks.end();) {
                    if (it->_expiryDate.toMilliseconds() <= TimePoint::now().toMilliseconds()) {
                        it->_timeout();
                        it = _tasks.erase(it);
                    } else if (it->_expiryDate != TimePoint::max()) {
                        break;
                    } else {
                        ++it;
                    }
                }
            } else {
                _future = nothingToDo();
                try {
                    if (_isRunning.load()) {
                        std::unique_lock<std::mutex> lock(_tasksMutex);
                        if (_tasks.empty()) {
                            _tasksChanged.wait(lock);
                        }
                        if (!_tasks.empty()) {
                            TaskWithExpiryDate nextTask = std::move(_tasks.front());
                            if (nextTask._expiryDate.toMilliseconds() <=
                                TimePoint::now().toMilliseconds()) {
                                nextTask._timeout();
                                _tasks.erase(_tasks.begin());
                                continue;
                            } else {
                                _tasks.erase(_tasks.begin());
                                if (!nextTask._task) {
                                    throw std::runtime_error("Dropping null-task.");
                                }
                                _future = nextTask._task();
                            }
                        }
                    }
                    if (!_future) {
                        throw std::runtime_error("Future factory created empty task.");
                    }
                } catch (const std::exception& e) {
                    JOYNR_LOG_ERROR(logger(),
                                    "Task creation failed, continue with next task: {}",
                                    e.what());
                    _future = nothingToDo();
                } catch (...) {
                    JOYNR_LOG_ERROR(
                            logger(),
                            "Task creation failed for unknown reasons, continue with next task.");
                    _future = nothingToDo();
                }
            }
        }
    }

    inline static FutureSP nothingToDo()
    {
        auto alreadCompleted = std::make_shared<Future<Ts...>>();
        alreadCompleted->onError(
                std::make_shared<exceptions::JoynrRuntimeException>("No task available."));
        return alreadCompleted;
    }

    ADD_LOGGER(TaskSequencer<Ts...>);
};

} // namespace joynr

#endif // TASK_SEQUENCER
