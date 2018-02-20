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

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
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
struct key_and_ttlAbsolute;
}

template <typename T>
class JOYNR_EXPORT MessageQueue
{
public:
    MessageQueue(std::uint64_t messageQueueLimit = 0,
                 std::uint64_t perKeyMessageQueueLimit = 0,
                 std::uint64_t messageQueueLimitBytes = 0)
            : queue(),
              queueMutex(),
              messageQueueLimit(messageQueueLimit),
              messageQueueLimitBytes(messageQueueLimitBytes),
              perKeyMessageQueueLimit(perKeyMessageQueueLimit),
              queueSizeBytes(0)
    {
    }

    std::size_t getQueueLength() const
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        return getQueueLengthUnlocked();
    }

    std::size_t getQueueSizeBytes() const
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        return queueSizeBytes;
    }

    void queueMessage(const T key, std::shared_ptr<ImmutableMessage> message)
    {

        MessageQueueItem item;
        item.key = std::move(key);
        item.ttlAbsolute = message->getExpiryDate();
        std::string recipient = message->getRecipient();
        item.message = std::move(message);

        std::lock_guard<std::mutex> lock(queueMutex);
        ensureFreeQueueSlot(item.key);
        if (!ensureFreeQueueBytes(item.message->getMessageSize())) {
            JOYNR_LOG_WARN(logger(),
                           "queueMessage: messageSize {} exceeds messageQueueLimitBytes {}, "
                           "discarding message with id {}. recipient {}; queueSize(bytes) = {}, "
                           "#msgs = {}",
                           item.message->getMessageSize(),
                           messageQueueLimitBytes,
                           item.message->getId(),
                           recipient,
                           queueSizeBytes,
                           getQueueLengthUnlocked());
            return;
        }
        queueSizeBytes += item.message->getMessageSize();
        queue.insert(std::move(item));
        JOYNR_LOG_TRACE(logger(),
                        "queueMessage: recipient {}, new queueSize(bytes) = {}, #msgs = {}",
                        recipient,
                        queueSizeBytes,
                        getQueueLengthUnlocked());
    }

    std::shared_ptr<ImmutableMessage> getNextMessageFor(const T& key)
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        auto& keyIndex = boost::multi_index::get<messagequeuetags::key>(queue);

        auto queueElement = keyIndex.find(key);
        if (queueElement != keyIndex.cend()) {
            auto message = std::move(queueElement->message);
            queueSizeBytes -= message->getMessageSize();
            queue.erase(queueElement);
            JOYNR_LOG_TRACE(logger(),
                            "getNextMessageFor: message recipient {}, size {}, new "
                            "queueSize(bytes) = {}, #msgs = {}",
                            message->getRecipient(),
                            message->getMessageSize(),
                            queueSizeBytes,
                            getQueueLengthUnlocked());
            return message;
        }
        return nullptr;
    }

    void removeOutdatedMessages()
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        int numberOfErasedMessages = 0;
        std::size_t erasedBytes = 0;

        if (queue.empty()) {
            return;
        }

        auto& ttlIndex = boost::multi_index::get<messagequeuetags::ttlAbsolute>(queue);
        auto onePastOutdatedMsgIt = ttlIndex.lower_bound(TimePoint::now());

        for (auto it = ttlIndex.begin(); it != onePastOutdatedMsgIt; ++it) {
            std::size_t msgSize = it->message->getMessageSize();
            JOYNR_LOG_TRACE(logger(),
                            "removeOutdatedMessages: Erasing expired message with id {} of size {}",
                            it->message->getId(),
                            msgSize);
            queueSizeBytes -= msgSize;
            erasedBytes += msgSize;
            numberOfErasedMessages++;
        }
        ttlIndex.erase(ttlIndex.begin(), onePastOutdatedMsgIt);
        if (numberOfErasedMessages) {
            JOYNR_LOG_TRACE(logger(),
                            "removeOutdatedMessages: Erased {} messages of size {}, new "
                            "queueSize(bytes) = {}, #msgs = {}",
                            numberOfErasedMessages,
                            erasedBytes,
                            queueSizeBytes,
                            getQueueLengthUnlocked());
        }
    }

protected:
    DISALLOW_COPY_AND_ASSIGN(MessageQueue);
    ADD_LOGGER(MessageQueue);

    struct MessageQueueItem
    {
        T key;
        TimePoint ttlAbsolute;
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
                            BOOST_MULTI_INDEX_MEMBER(MessageQueueItem, TimePoint, ttlAbsolute)>,
                    boost::multi_index::ordered_non_unique<
                            boost::multi_index::tag<messagequeuetags::key_and_ttlAbsolute>,
                            boost::multi_index::composite_key<
                                    MessageQueueItem,
                                    boost::multi_index::
                                            member<MessageQueueItem, T, &MessageQueueItem::key>,
                                    BOOST_MULTI_INDEX_MEMBER(MessageQueueItem,
                                                             TimePoint,
                                                             ttlAbsolute)>>>>;

    QueueMultiIndexContainer queue;
    mutable std::mutex queueMutex;

private:
    const std::uint64_t messageQueueLimit;
    const std::uint64_t messageQueueLimitBytes;
    const std::uint64_t perKeyMessageQueueLimit;
    std::uint64_t queueSizeBytes;

    std::size_t getQueueLengthUnlocked() const
    {
        return boost::multi_index::get<messagequeuetags::key>(queue).size();
    }

    bool ensureFreeQueueBytes(const std::uint64_t messageLength)
    {
        // queueMutex must have been acquired earlier
        const bool queueLimitBytesActive = messageQueueLimitBytes > 0;
        if (!queueLimitBytesActive) {
            return true;
        }

        if (messageLength > messageQueueLimitBytes) {
            return false;
        }

        while (queueSizeBytes + messageLength > messageQueueLimitBytes) {
            removeMessageWithLeastTtl();
        }
        return true;
    }

    void ensureFreeQueueSlot(const T& key)
    {
        // queueMutex must have been acquired earlier
        const bool queueLimitActive = messageQueueLimit > 0;
        if (!queueLimitActive) {
            return;
        }

        const bool perKeyQueueLimitActive = perKeyMessageQueueLimit > 0;
        if (perKeyQueueLimitActive) {
            ensureFreePerKeyQueueSlot(key);
        }

        while (getQueueLengthUnlocked() >= messageQueueLimit) {
            removeMessageWithLeastTtl();
        }
    }

    void ensureFreePerKeyQueueSlot(const T& key)
    {
        // queueMutex must have been locked already
        assert(perKeyMessageQueueLimit > 0);

        auto& keyAndTtlIndex =
                boost::multi_index::get<messagequeuetags::key_and_ttlAbsolute>(queue);
        auto range = keyAndTtlIndex.equal_range(key);
        const std::size_t numEntriesForKey = std::distance(range.first, range.second);

        JOYNR_LOG_TRACE(logger(),
                        "ensureFreePerKeyQueueSlot: numEntriesForKey = {}, perKeyMessageQueueLimit "
                        "= {}",
                        numEntriesForKey,
                        perKeyMessageQueueLimit);

        if (numEntriesForKey >= perKeyMessageQueueLimit) {
            JOYNR_LOG_WARN(logger(),
                           "Erasing message with id {} of size {} since key based queue limit of "
                           "{} was reached",
                           range.first->message->getId(),
                           range.first->message->getMessageSize(),
                           perKeyMessageQueueLimit);
            queueSizeBytes -= range.first->message->getMessageSize();
            keyAndTtlIndex.erase(range.first);
        }
    }

    void removeMessageWithLeastTtl()
    {
        // queueMutex must have been locked already
        if (queue.empty()) {
            return;
        }

        const std::size_t queueLength = getQueueLengthUnlocked();
        auto& ttlIndex = boost::multi_index::get<messagequeuetags::ttlAbsolute>(queue);
        auto msgWithLowestTtl = ttlIndex.cbegin();
        assert(msgWithLowestTtl != ttlIndex.cend());

        JOYNR_LOG_WARN(logger(),
                       "Erasing message with id {} of size {} since either generic queue limit of "
                       "{} messages or {} bytes was reached, #msgs = {}, queueSize(bytes) = {}",
                       msgWithLowestTtl->message->getId(),
                       msgWithLowestTtl->message->getMessageSize(),
                       messageQueueLimit,
                       messageQueueLimitBytes,
                       queueLength,
                       queueSizeBytes);

        queueSizeBytes -= msgWithLowestTtl->message->getMessageSize();
        ttlIndex.erase(msgWithLowestTtl);
    }
};
} // namespace joynr

#endif // MESSAGEQUEUE_H
