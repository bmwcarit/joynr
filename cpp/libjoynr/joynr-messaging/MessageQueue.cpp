/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#include "joynr/MessageQueue.h"
#include "joynr/DispatcherUtils.h"

#include <chrono>

namespace joynr
{

using namespace std::chrono;

MessageQueue::MessageQueue()
        : queue(new std::multimap<std::string, MessageQueueItem*>()), queueMutex()
{
}

MessageQueue::~MessageQueue()
{
    delete queue;
}

std::size_t MessageQueue::getQueueLength()
{
    return queue->size();
}

std::size_t MessageQueue::queueMessage(const JoynrMessage& message)
{
    JoynrTimePoint absTtl = message.getHeaderExpiryDate();
    MessageQueueItem* item = new MessageQueueItem(message, absTtl);
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue->insert(std::make_pair(message.getHeaderTo(), item));
    }
    return queue->size();
}

MessageQueueItem* MessageQueue::getNextMessageForParticipant(const std::string destinationPartId)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    auto queueElement = queue->find(destinationPartId);
    if (queueElement != queue->end()) {
        MessageQueueItem* item = queueElement->second;
        queue->erase(queueElement);
        return item;
    }
    return NULL;
}

int64_t MessageQueue::removeOutdatedMessages()
{
    int64_t counter = 0;
    if (queue->empty()) {
        return counter;
    }

    JoynrTimePoint now = time_point_cast<milliseconds>(system_clock::now());
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto queueIterator = queue->begin(); queueIterator != queue->end();) {
            MessageQueueItem* value = queueIterator->second;
            if (value->getDecayTime() < now) {
                queueIterator = queue->erase(queueIterator);
                delete value;
                counter++;
            } else {
                ++queueIterator;
            }
        }
    }
    return counter;
}
}
