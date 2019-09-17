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

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ImmutableMessage.h"

namespace joynr
{

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
            : _queue(),
              _queueMutex(),
              _messageQueueLimit(messageQueueLimit),
              _messageQueueLimitBytes(messageQueueLimitBytes),
              _perKeyMessageQueueLimit(perKeyMessageQueueLimit),
              _queueSizeBytes(0)
    {
    }

    std::size_t getQueueLength() const
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        return getQueueLengthUnlocked();
    }

    std::size_t getQueueSizeBytes() const
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        return _queueSizeBytes;
    }

    void queueMessage(const T key, std::shared_ptr<ImmutableMessage> message)
    {

        MessageQueueItem item;
        item._key = std::move(key);
        item._ttlAbsolute = message->getExpiryDate();
        item._message = std::move(message);

        std::lock_guard<std::mutex> lock(_queueMutex);
        ensureFreeQueueSlot(item._key);
        if (!ensureFreeQueueBytes(item._message->getMessageSize())) {
            JOYNR_LOG_WARN(logger(),
                           "queueMessage: messageSize exceeds messageQueueLimitBytes {}, "
                           "discarding message {}; queueSize(bytes) = {}, "
                           "#msgs = {}",
                           _messageQueueLimitBytes,
                           item._message->getTrackingInfo(),
                           _queueSizeBytes,
                           getQueueLengthUnlocked());
            return;
        }
        _queueSizeBytes += item._message->getMessageSize();
        std::string trackingInfo = item._message->getTrackingInfo();
        _queue.insert(std::move(item));
        JOYNR_LOG_TRACE(logger(),
                        "queueMessage: message {}, new queueSize(bytes) = {}, #msgs = {}",
                        trackingInfo,
                        _queueSizeBytes,
                        getQueueLengthUnlocked());
    }

    std::shared_ptr<ImmutableMessage> getNextMessageFor(const T& key)
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        auto& keyIndex = boost::multi_index::get<messagequeuetags::key>(_queue);

        auto queueElement = keyIndex.find(key);
        if (queueElement != keyIndex.cend()) {
            auto message = std::move(queueElement->_message);
            _queueSizeBytes -= message->getMessageSize();
            _queue.erase(queueElement);
            JOYNR_LOG_TRACE(logger(),
                            "getNextMessageFor: message {}, new "
                            "queueSize(bytes) = {}, #msgs = {}",
                            message->getTrackingInfo(),
                            _queueSizeBytes,
                            getQueueLengthUnlocked());
            return message;
        }
        return nullptr;
    }

    void removeOutdatedMessages()
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        int numberOfErasedMessages = 0;
        std::size_t erasedBytes = 0;

        if (_queue.empty()) {
            return;
        }

        auto& ttlIndex = boost::multi_index::get<messagequeuetags::ttlAbsolute>(_queue);
        auto onePastOutdatedMsgIt = ttlIndex.lower_bound(TimePoint::now());

        for (auto it = ttlIndex.begin(); it != onePastOutdatedMsgIt; ++it) {
            std::size_t msgSize = it->_message->getMessageSize();
            JOYNR_LOG_INFO(logger(),
                           "removeOutdatedMessages: Erasing expired message {}",
                           it->_message->getTrackingInfo());
            _queueSizeBytes -= msgSize;
            erasedBytes += msgSize;
            numberOfErasedMessages++;
        }
        ttlIndex.erase(ttlIndex.begin(), onePastOutdatedMsgIt);
        if (numberOfErasedMessages) {
            JOYNR_LOG_INFO(logger(),
                           "removeOutdatedMessages: Erased {} messages of size {}, new "
                           "queueSize(bytes) = {}, #msgs = {}",
                           numberOfErasedMessages,
                           erasedBytes,
                           _queueSizeBytes,
                           getQueueLengthUnlocked());
        }
    }

protected:
    DISALLOW_COPY_AND_ASSIGN(MessageQueue);
    ADD_LOGGER(MessageQueue);

    struct MessageQueueItem
    {
        T _key;
        TimePoint _ttlAbsolute;
        std::shared_ptr<ImmutableMessage> _message;
    };

    using QueueMultiIndexContainer = boost::multi_index_container<
            MessageQueueItem,
            boost::multi_index::indexed_by<
                    boost::multi_index::hashed_non_unique<
                            boost::multi_index::tag<messagequeuetags::key>,
                            BOOST_MULTI_INDEX_MEMBER(MessageQueueItem, T, _key)>,
                    boost::multi_index::ordered_non_unique<
                            boost::multi_index::tag<messagequeuetags::ttlAbsolute>,
                            BOOST_MULTI_INDEX_MEMBER(MessageQueueItem, TimePoint, _ttlAbsolute)>,
                    boost::multi_index::ordered_non_unique<
                            boost::multi_index::tag<messagequeuetags::key_and_ttlAbsolute>,
                            boost::multi_index::composite_key<
                                    MessageQueueItem,
                                    boost::multi_index::
                                            member<MessageQueueItem, T, &MessageQueueItem::_key>,
                                    BOOST_MULTI_INDEX_MEMBER(MessageQueueItem,
                                                             TimePoint,
                                                             _ttlAbsolute)>>>>;

    QueueMultiIndexContainer _queue;
    mutable std::mutex _queueMutex;

private:
    const std::uint64_t _messageQueueLimit;
    const std::uint64_t _messageQueueLimitBytes;
    const std::uint64_t _perKeyMessageQueueLimit;
    std::uint64_t _queueSizeBytes;

    std::size_t getQueueLengthUnlocked() const
    {
        return boost::multi_index::get<messagequeuetags::key>(_queue).size();
    }

    bool ensureFreeQueueBytes(const std::uint64_t messageLength)
    {
        // queueMutex must have been acquired earlier
        const bool queueLimitBytesActive = _messageQueueLimitBytes > 0;
        if (!queueLimitBytesActive) {
            return true;
        }

        if (messageLength > _messageQueueLimitBytes) {
            return false;
        }

        while (_queueSizeBytes + messageLength > _messageQueueLimitBytes) {
            removeMessageWithLeastTtl();
        }
        return true;
    }

    void ensureFreeQueueSlot(const T& key)
    {
        // queueMutex must have been acquired earlier
        const bool queueLimitActive = _messageQueueLimit > 0;
        if (!queueLimitActive) {
            return;
        }

        const bool perKeyQueueLimitActive = _perKeyMessageQueueLimit > 0;
        if (perKeyQueueLimitActive) {
            ensureFreePerKeyQueueSlot(key);
        }

        while (getQueueLengthUnlocked() >= _messageQueueLimit) {
            removeMessageWithLeastTtl();
        }
    }

    void ensureFreePerKeyQueueSlot(const T& key)
    {
        // queueMutex must have been locked already
        assert(_perKeyMessageQueueLimit > 0);

        auto& keyAndTtlIndex =
                boost::multi_index::get<messagequeuetags::key_and_ttlAbsolute>(_queue);
        auto range = keyAndTtlIndex.equal_range(key);
        const std::size_t numEntriesForKey =
                static_cast<std::size_t>(std::distance(range.first, range.second));

        JOYNR_LOG_TRACE(logger(),
                        "ensureFreePerKeyQueueSlot: numEntriesForKey = {}, perKeyMessageQueueLimit "
                        "= {}",
                        numEntriesForKey,
                        _perKeyMessageQueueLimit);

        if (numEntriesForKey >= _perKeyMessageQueueLimit) {
            JOYNR_LOG_WARN(logger(),
                           "Erasing message {} since key based queue limit of "
                           "{} was reached",
                           range.first->_message->getTrackingInfo(),
                           _perKeyMessageQueueLimit);
            _queueSizeBytes -= range.first->_message->getMessageSize();
            keyAndTtlIndex.erase(range.first);
        }
    }

    void removeMessageWithLeastTtl()
    {
        // queueMutex must have been locked already
        if (_queue.empty()) {
            return;
        }

        const std::size_t queueLength = getQueueLengthUnlocked();
        auto& ttlIndex = boost::multi_index::get<messagequeuetags::ttlAbsolute>(_queue);
        auto msgWithLowestTtl = ttlIndex.cbegin();
        assert(msgWithLowestTtl != ttlIndex.cend());

        JOYNR_LOG_WARN(logger(),
                       "Erasing message {} since either generic queue limit of "
                       "{} messages or {} bytes was reached, #msgs = {}, queueSize(bytes) = {}",
                       msgWithLowestTtl->_message->getTrackingInfo(),
                       _messageQueueLimit,
                       _messageQueueLimitBytes,
                       queueLength,
                       _queueSizeBytes);

        _queueSizeBytes -= msgWithLowestTtl->_message->getMessageSize();
        ttlIndex.erase(msgWithLowestTtl);
    }
};
} // namespace joynr

#endif // MESSAGEQUEUE_H
