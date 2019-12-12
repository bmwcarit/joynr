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
#ifndef BLOCKINGQUEUE_H
#define BLOCKINGQUEUE_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class Runnable;

/**
 * @class BlockingQueue
 * @brief A thread safe queue for submitting tasks
 *
 * This class provides a FIFO queue to add and take tasks of type
 * @ref Runnable. In case of an empty queue, calling @ref take will
 * block until a task is available.
 */
class JOYNR_EXPORT BlockingQueue
{
public:
    /**
     * @brief Constructor
     */
    BlockingQueue();

    /**
     * @brief Destructor
     * @note Be sure to call @ref shutdown and wait for return before
     *      destroying this object
     */
    virtual ~BlockingQueue();

    /**
     * @brief Submit task to be done
     * @param task Task to be added to the queue
     */
    void add(std::shared_ptr<Runnable> task);

    /**
     * @brief Does an ordinary shutdown of @ref BlockingQueue
     * @note Must be called before destructor is called
     */
    void shutdown();

    /**
     * @brief Take some work
     * @return Work to be done or @c nullptr if scheduler is shutting down
     *
     * @note This method will block until work is available or the scheduler is
     *      going to shutdown. If so, this method will return @c nullptr.
     */
    std::shared_ptr<Runnable> take();

    /**
     * @brief Returns the current size of the queue
     * @return Number of pending @ref Runnable objects
     */
    int getQueueLength() const;

private:
    /*! Not allowed to copy @ref BlockingQueue */
    DISALLOW_COPY_AND_ASSIGN(BlockingQueue);

private:
    /*! Logger */
    ADD_LOGGER(BlockingQueue)

    /*! Flag indicating scheduler is shutting down */
    std::atomic_bool stoppingScheduler;

    /*! Queue of waiting work */
    std::deque<std::shared_ptr<Runnable>> queue;

    /*! Cond to wait for task on calling @ref take */
    std::condition_variable condition;

    /*! Mutual exclusion of the @ref queue and for @ref condition */
    mutable std::mutex conditionMutex;
};
} // namespace joynr
#endif // BLOCKINGQUEUE_H
