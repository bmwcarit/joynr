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
#ifndef CONCURRENTQUEUE_H
#define CONCURRENTQUEUE_H

#include <mutex>
#include <queue>

namespace joynr
{

/**
 * @class ConcurrentQueue
 * @brief The ConcurentQueue is a synchronized FIFO queue
 */
template <typename T>
class ConcurrentQueue
{
public:
    /**
     * @brief Constructor
     */
    ConcurrentQueue() : mutex(), queue(){};

    /**
     * @brief Push element to queue
     * @param value Element to be pushed
     */
    void push(T value)
    {
        std::lock_guard<std::mutex> guard(mutex);
        queue.push(value);
    }

    /**
     * @brief Pop first element from queue
     */
    void pop()
    {
        std::lock_guard<std::mutex> guard(mutex);
        queue.pop();
    }

    /**
     * @brief Get first element in the queue
     * @return First element in the queue
     */
    T& front()
    {
        std::lock_guard<std::mutex> guard(mutex);
        return queue.front();
    }

    /**
     * @brief Get first element in the queue
     * @return First element in the queue
     */
    const T& front() const
    {
        std::lock_guard<std::mutex> guard(mutex);
        return queue.front();
    }

    /**
     * @brief Returns whether the queue is empty or not
     * @return If @c true is returned the queue is empty
     */
    bool empty() const
    {
        std::lock_guard<std::mutex> guard(mutex);
        return queue.empty();
    }

    /**
     * @brief Returns the current size of the queue
     * @return Current size of the queue
     */
    uint32_t size() const
    {
        return queue.size();
    }

private:
    mutable std::mutex mutex;
    std::queue<T> queue;
};

} // namespace joynr
#endif // CONCURRENTQUEUE_H
