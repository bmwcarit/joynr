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
#include "joynr/BlockingQueue.h"

#include <utility>

namespace joynr
{

BlockingQueue::BlockingQueue() : stoppingScheduler(false), queue(), condition(), conditionMutex()
{
}

BlockingQueue::~BlockingQueue()
{
    shutdown();
}

void BlockingQueue::add(std::shared_ptr<Runnable> work)
{
    {
        std::lock_guard<std::mutex> lock(conditionMutex);
        queue.push_front(std::move(work));
    }

    // Notify a waiting thread
    condition.notify_one();
}

std::shared_ptr<Runnable> BlockingQueue::take()
{
    if (stoppingScheduler) {
        JOYNR_LOG_TRACE(logger(), "Shutting down and returning NULL");
        return nullptr;
    }

    std::unique_lock<std::mutex> lock(
            conditionMutex); // std::condition_variable works only with unique_lock

    JOYNR_LOG_TRACE(logger(), "Wait for condition (queuelen={})", queue.size());
    // Wait for work or shutdown
    condition.wait(lock, [this] { return (stoppingScheduler || !queue.empty()); });
    if (stoppingScheduler) {
        JOYNR_LOG_TRACE(logger(), "Shutting down and returning NULL");
        return nullptr;
    }

    JOYNR_LOG_TRACE(logger(), "Condition released");

    // Get the item
    std::shared_ptr<Runnable> item = queue.back();
    queue.pop_back();
    return item;
}

int BlockingQueue::getQueueLength() const
{
    std::lock_guard<std::mutex> lock(conditionMutex);
    return queue.size();
}

void BlockingQueue::shutdown()
{
    JOYNR_LOG_TRACE(logger(), "Shutdown called");
    {
        std::lock_guard<std::mutex> lock(conditionMutex);
        stoppingScheduler = true;

        // delete queued elements if necessary
        // for (std::shared_ptr<Runnable> i : queue) {
        // if (i->isDeleteOnExit()) {
        //    delete i;
        //}
        //}
        queue.clear();
    }

    // unblock waiting threads
    JOYNR_LOG_TRACE(logger(), "Shutdown, notifying all.");
    condition.notify_all();
}

} // namespace joynr
