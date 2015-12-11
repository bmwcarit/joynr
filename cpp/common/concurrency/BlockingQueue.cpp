/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "joynr/Runnable.h"

#include <cassert>

#include "joynr/joynrlogging.h"

namespace joynr
{

using namespace joynr_logging;

Logger* BlockingQueue::logger = Logging::getInstance()->getLogger("MSG", "FifoScheduler");

BlockingQueue::BlockingQueue() : stoppingScheduler(false), queue(), condition(), conditionMutex()
{
}

BlockingQueue::~BlockingQueue()
{
    shutdown();
}

void BlockingQueue::add(Runnable* work)
{
    {
        std::lock_guard<std::mutex> lock(conditionMutex);
        queue.push_front(work);
    }

    // Notify a waiting thread
    condition.notify_one();
}

Runnable* BlockingQueue::take()
{
    if (stoppingScheduler) {
        LOG_TRACE(logger, "Shuting down and returning NULL");
        return NULL;
    }

    std::unique_lock<std::mutex> lock(
            conditionMutex); // std::condition_variable works only with unique_lock

    LOG_TRACE(logger, FormatString("Wait for condition (queuelen=%0)").arg(queue.size()).str());
    // Wait for work or shutdown
    condition.wait(lock, [this] { return (stoppingScheduler || !queue.empty()); });
    if (stoppingScheduler) {
        LOG_TRACE(logger, "Shuting down and returning NULL");
        return NULL;
    }

    LOG_TRACE(logger, "Condition released");

    // Get the item
    Runnable* item = queue.back();
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
    LOG_TRACE(logger, "Shutdown called");
    {
        std::lock_guard<std::mutex> lock(conditionMutex);
        stoppingScheduler = true;

        // delete queued elements if necessary
        for (Runnable* i : queue) {
            if (i->isDeleteOnExit()) {
                delete i;
            }
        }
        queue.clear();
    }

    // unblock waiting threads
    LOG_TRACE(logger, "Shutdown, notifying all.");
    condition.notify_all();
}

} // namespace joynr
