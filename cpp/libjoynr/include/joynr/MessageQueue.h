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
#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "joynr/ContentWithDecayTime.h"
#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ImmutableMessage.h"

namespace joynr
{
using MessageQueueItem = ContentWithDecayTime<std::shared_ptr<ImmutableMessage>>;

template <typename T>
class JOYNR_EXPORT MessageQueue
{
public:
    MessageQueue() : queue(), queueMutex()
    {
    }

    std::size_t getQueueLength() const
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        return queue.size();
    }

    std::size_t queueMessage(T key, std::shared_ptr<ImmutableMessage> message)
    {
        JoynrTimePoint absTtl = message->getExpiryDate();
        auto item = std::make_unique<MessageQueueItem>(std::move(message), absTtl);

        std::lock_guard<std::mutex> lock(queueMutex);
        queue.insert(std::make_pair(std::move(key), std::move(item)));

        return queue.size();
    }

    std::unique_ptr<MessageQueueItem> getNextMessageFor(const T& key)
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        auto queueElement = queue.find(key);
        if (queueElement != queue.end()) {
            auto item = std::move(queueElement->second);
            queue.erase(queueElement);
            return item;
        }
        return nullptr;
    }

    std::int64_t removeOutdatedMessages()
    {
        std::lock_guard<std::mutex> lock(queueMutex);

        std::int64_t counter = 0;
        if (queue.empty()) {
            return counter;
        }

        JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now());

        for (auto queueIterator = queue.begin(); queueIterator != queue.end();) {
            if (queueIterator->second->getDecayTime() < now) {
                queueIterator = queue.erase(queueIterator);
                counter++;
            } else {
                ++queueIterator;
            }
        }

        return counter;
    }

protected:
    DISALLOW_COPY_AND_ASSIGN(MessageQueue);

    // TODO should we replace by std::unordered_multimap?
    // or a boost multi_index table because we need to store the TTL
    std::multimap<T, std::unique_ptr<MessageQueueItem>> queue;
    mutable std::mutex queueMutex;
};
} // namespace joynr

#endif // MESSAGEQUEUE_H
