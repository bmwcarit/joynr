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
#include "joynr/ReadWriteLock.h"
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

    struct TaskWithExpiryDate {
        Task _task;
        TimePoint _expiryDate;
        std::function<void()> _timeout;
    };

    /**
     * @brief Creates queue processor.
     * @param maxTimeToWait the maximum time to wait until the queue is checked for expired tasks
     * again even if the current task has not completed yet. This should be set to the minimum ttl
     * of tasks added to the sequencer. The actual time to wait is calculated from the remaining ttl
     * of the task with the minimum expiry date in the queue.
     */
    TaskSequencer(std::chrono::milliseconds maxTimeToWait = std::chrono::milliseconds(60000))
            : _isRunning{true}, _future{nothingToDo()}, _maxTimeToWait{maxTimeToWait}
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

        ReadLocker futureReadLock(_futureReadWriteLock);
        if (std::future_status::ready !=
            _future->_resultFuture.wait_for(std::chrono::milliseconds(0))) {
            _future->onError(std::make_shared<exceptions::JoynrRuntimeException>(
                    "All tasks have been canceled."));
        }
        futureReadLock.unlock();

        {
            std::unique_lock<std::mutex> lock(_tasksMutex);
            std::vector<TaskWithExpiryDate> releasePendingTasksMemory;
            _tasks.swap(releasePendingTasksMemory);
            _tasksChanged.notify_all();
        }
        _worker.join();

        auto releaseCurrentFutureMemory = nothingToDo();
        WriteLocker futureWriteLock(_futureReadWriteLock);
        _future.swap(releaseCurrentFutureMemory);
        // futureWriteLock will be auto unlocked when leaving section
    }

private:
    std::atomic_bool _isRunning;
    FutureSP _future;
    std::mutex _tasksMutex;
    ReadWriteLock _futureReadWriteLock;
    std::condition_variable _tasksChanged;
    std::vector<TaskWithExpiryDate> _tasks;
    std::thread _worker;
    const std::chrono::milliseconds _maxTimeToWait;

    void run()
    {
        while (_isRunning.load()) {
            std::chrono::milliseconds timeToWaitMs = _maxTimeToWait;
            {
                std::unique_lock<std::mutex> lock(_tasksMutex);
                if (!_tasks.empty()) {
                    auto taskWithMinExpiryDate = std::min_element(
                            _tasks.begin(),
                            _tasks.end(),
                            [](const TaskWithExpiryDate& first, const TaskWithExpiryDate& second) {
                                return first._expiryDate < second._expiryDate;
                            });
                    // use _maxTimeToWait as maximum time to wait
                    if (taskWithMinExpiryDate->_expiryDate.relativeFromNow() < _maxTimeToWait) {
                        timeToWaitMs = taskWithMinExpiryDate->_expiryDate.relativeFromNow();
                    }
                    if (taskWithMinExpiryDate->_expiryDate.toMilliseconds() <=
                        TimePoint::now().toMilliseconds()) {
                        // cancel expired task
                        taskWithMinExpiryDate->_timeout();
                        _tasks.erase(taskWithMinExpiryDate);
                        continue;
                    }
                }
            }

            ReadLocker futureReadLock(_futureReadWriteLock);
            auto futureStatus = _future->_resultFuture.wait_for(timeToWaitMs);
            futureReadLock.unlock();

            if (futureStatus != std::future_status::ready) {
                // cancel expired tasks
                std::unique_lock<std::mutex> lock(_tasksMutex);
                for (auto it = _tasks.begin(); it != _tasks.end();) {
                    if (it->_expiryDate.toMilliseconds() <= TimePoint::now().toMilliseconds()) {
                        it->_timeout();
                        it = _tasks.erase(it);
                    } else {
                        ++it;
                    }
                }
            } else {
                // task finished, get and execute next task
                WriteLocker futureWriteLock1(_futureReadWriteLock);
                _future = nothingToDo();
                futureWriteLock1.unlock();
                try {
                    // if something throws in this block we must make sure
                    // that any locks are auto unlocked before reaching the
                    // catch block. Thus we need to use different locks.
                    if (_isRunning.load()) {
                        std::unique_lock<std::mutex> lock(_tasksMutex);
                        if (_tasks.empty()) {
                            _tasksChanged.wait(lock);
                        }
                        if (!_tasks.empty()) {
                            TaskWithExpiryDate nextTask = std::move(_tasks.front());
                            if (nextTask._expiryDate.toMilliseconds() <=
                                TimePoint::now().toMilliseconds()) {
                                _tasks.erase(_tasks.begin());
                                lock.unlock();
                                nextTask._timeout();
                                continue;
                            } else {
                                _tasks.erase(_tasks.begin());
                                if (!nextTask._task) {
                                    throw std::runtime_error("Dropping null-task.");
                                }
                                lock.unlock();
                                WriteLocker futureWriteLock2(_futureReadWriteLock);
                                _future = nextTask._task();
                                // futureWriteLock2 is auto unlocked when leaving section
                            }
                        }
                    }
                    ReadLocker futureReadLock3(_futureReadWriteLock);
                    if (!_future) {
                        throw std::runtime_error("Future factory created empty task.");
                    }
                } catch (const std::exception& e) {
                    JOYNR_LOG_ERROR(logger(),
                                    "Task creation failed, continue with next task: {}",
                                    e.what());
                    WriteLocker futureWriteLock3(_futureReadWriteLock);
                    _future = nothingToDo();
                    // futureWriteLock3 is auto unlocked when leaving section
                } catch (...) {
                    JOYNR_LOG_ERROR(
                            logger(),
                            "Task creation failed for unknown reasons, continue with next task.");
                    WriteLocker futureWriteLock4(_futureReadWriteLock);
                    _future = nothingToDo();
                    // futureWriteLock4 is auto unlocked when leaving section
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
