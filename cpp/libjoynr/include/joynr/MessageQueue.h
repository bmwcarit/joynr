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
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ImmutableMessage.h"

namespace joynr
{
using MessageQueueItem = ContentWithDecayTime<std::shared_ptr<ImmutableMessage>>;

template <typename T>
class JOYNR_EXPORT MessageQueue
{
public:
    MessageQueue(std::uint64_t messageLimit = 0)
            : queue(), queueMutex(), messageQueueLimit(messageLimit)
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

        if (messageQueueLimit > 0)
            if (getQueueLength() == messageQueueLimit) {
                removeMessageWithLeastTtl();
            }

        std::lock_guard<std::mutex> lock(queueMutex);
        queue.insert(std::make_pair(std::move(key), std::move(item)));

        return queue.size();
    }

    std::shared_ptr<ImmutableMessage> getNextMessageFor(const T& key)
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        auto queueElement = queue.find(key);
        if (queueElement != queue.end()) {
            auto message = std::move(queueElement->second->getContent());
            queue.erase(queueElement);
            return message;
        }
        return nullptr;
    }

    void removeOutdatedMessages()
    {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (queue.empty()) {
            return;
        }

        JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now());

        for (auto queueIterator = queue.begin(); queueIterator != queue.end();) {
            if (queueIterator->second->getDecayTime() < now) {
                queueIterator = queue.erase(queueIterator);
            } else {
                ++queueIterator;
            }
        }
    }

protected:
    DISALLOW_COPY_AND_ASSIGN(MessageQueue);
    ADD_LOGGER(MessageQueue);

    // TODO should we replace by std::unordered_multimap?
    // or a boost multi_index table because we need to store the TTL
    std::multimap<T, std::unique_ptr<MessageQueueItem>> queue;
    mutable std::mutex queueMutex;

private:
    std::uint64_t messageQueueLimit;

    void removeMessageWithLeastTtl()
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        assert(!queue.empty());

        auto queueIteratorHold = queue.begin();
        for (auto queueIterator = queue.begin(); queueIterator != queue.end(); queueIterator++) {
            if (queueIterator->second->getDecayTime() < queueIteratorHold->second->getDecayTime()) {
                queueIteratorHold = queueIterator;
            }
        }

        auto message = std::move(queueIteratorHold->second->getContent());
        JOYNR_LOG_WARN(logger(),
                       "erasing message with id {} since queue limit of {} was reached",
                       message->getId(),
                       messageQueueLimit);

        queue.erase(queueIteratorHold);
    }
};
} // namespace joynr

#endif // MESSAGEQUEUE_H
