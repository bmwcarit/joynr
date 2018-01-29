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
#include <memory>
#include <mutex>
#include <string>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include "joynr/ContentWithDecayTime.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ImmutableMessage.h"

namespace joynr
{
using MessageQueueItem = ContentWithDecayTime<std::shared_ptr<ImmutableMessage>>;

namespace messagequeuetags
{
struct key;
struct ttlAbsolute;
}

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
        return boost::multi_index::get<messagequeuetags::key>(queue).size();
    }

    void queueMessage(T key, std::shared_ptr<ImmutableMessage> message)
    {
        MessageQueueItem item;
        item.key = std::move(key);
        item.ttlAbsolute = message->getExpiryDate();
        item.message = std::move(message);

        if (messageQueueLimit > 0)
            if (getQueueLength() == messageQueueLimit) {
                removeMessageWithLeastTtl();
            }

        std::lock_guard<std::mutex> lock(queueMutex);
        queue.insert(std::move(item));
    }

    std::shared_ptr<ImmutableMessage> getNextMessageFor(const T& key)
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        auto& keyIndex = boost::multi_index::get<messagequeuetags::key>(queue);

        auto queueElement = keyIndex.find(key);
        if (queueElement != keyIndex.cend()) {
            auto message = std::move(queueElement->message);
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

        auto& ttlIndex = boost::multi_index::get<messagequeuetags::ttlAbsolute>(queue);

        const JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now());
        auto onePastOutdatedMsgIt = ttlIndex.lower_bound(now);

        ttlIndex.erase(ttlIndex.begin(), onePastOutdatedMsgIt);
    }

protected:
    DISALLOW_COPY_AND_ASSIGN(MessageQueue);
    ADD_LOGGER(MessageQueue);

    struct MessageQueueItem
    {
        T key;
        JoynrTimePoint ttlAbsolute;
        std::shared_ptr<ImmutableMessage> message;
    };

    using QueueMultiIndexContainer = boost::multi_index_container<
            MessageQueueItem,
            boost::multi_index::indexed_by<
                    boost::multi_index::hashed_non_unique<
                            boost::multi_index::tag<messagequeuetags::key>,
                            BOOST_MULTI_INDEX_MEMBER(MessageQueueItem, T, key)>,
                    boost::multi_index::ordered_non_unique<
                            boost::multi_index::tag<messagequeuetags::ttlAbsolute>,
                            BOOST_MULTI_INDEX_MEMBER(MessageQueueItem,
                                                     JoynrTimePoint,
                                                     ttlAbsolute)>>>;

    QueueMultiIndexContainer queue;
    mutable std::mutex queueMutex;

private:
    std::uint64_t messageQueueLimit;

    void removeMessageWithLeastTtl()
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (queue.empty()) {
            return;
        }

        auto& ttlIndex = boost::multi_index::get<messagequeuetags::ttlAbsolute>(queue);
        auto msgWithLowestTtl = ttlIndex.cbegin();
        assert(msgWithLowestTtl != ttlIndex.cend());

        JOYNR_LOG_WARN(logger(),
                       "erasing message with id {} since queue limit of {} was reached",
                       msgWithLowestTtl->message->getId(),
                       messageQueueLimit);

        ttlIndex.erase(msgWithLowestTtl);
    }
};
} // namespace joynr

#endif // MESSAGEQUEUE_H
