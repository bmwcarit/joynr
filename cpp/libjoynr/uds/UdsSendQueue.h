/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#ifndef UDSSENDQUEUE_H
#define UDSSENDQUEUE_H

#include <deque>
#include <utility>

#include <boost/asio.hpp>
#include <boost/format.hpp>

#include "joynr/IUdsSender.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

/**
 * @brief Size limited FIFO queue for sending UDS frames using state machine
 *
 * The boolean return values are e.g. true if a new state shall be inserted to
 * corresponding user state machine.
 */
template <typename FRAME>
class UdsSendQueue
{
public:
    explicit UdsSendQueue(const std::size_t& maxSize) noexcept : _maxSize{maxSize}
    {
    }

    /**
     * Adds a new entry to the end of the queue.
     * If the maximum size is reached, the current entries are removed, and for each entry the send
     * failure callback is executed.
     * @param frame Frame to send, byte array will be consumed by call
     * @param callback Callback executed if frame has not been sent and the queue limit is reached.
     * @return True if the queue was empty before the insertion of the new entry.
     */
    bool pushBack(FRAME&& frame,
                  const IUdsSender::SendFailed& callback =
                          [](const joynr::exceptions::JoynrRuntimeException&) {})
    {
        const auto previousSize = _buffer.size();
        if (_maxSize <= previousSize) {
            const auto errorMsg =
                    boost::format(
                            "Sending queue size %d exceeded. Rescheduling all queued messages.") %
                    _maxSize;
            emptyQueueAndNotify(errorMsg.str());
        }
        _buffer.push_back(Entry(std::move(frame), callback));
        return previousSize == 0;
    }

    /**
     * Provides an ASIO buffer view on the first entry in the queue
     * @return View on first message (might be empty if queue is empty)
     */
    boost::asio::const_buffers_1 showFront() noexcept
    {
        if (_buffer.empty()) {
            return boost::asio::const_buffers_1(boost::asio::const_buffer());
        }
        const auto& frameBuffer = _buffer.front().first;
        return frameBuffer.raw();
    }

    /**
     * Removes first entry in queue if the queue is not empty and the sending has been successful.
     * @param sentFailed Error code signalling whe sucess or failure of sending the entry
     * @return True if the queue is not empty after removal and no error occured.
     */
    bool popFrontOnSuccess(const boost::system::error_code& sentFailed) noexcept
    {
        if (_buffer.empty() || sentFailed) {
            return false;
        }
        _buffer.pop_front();
        return !_buffer.empty();
    }

    /**
     * Removes all items from the queue and triggers the send-failed callback.
     * @param errorMessage Human readable reason
     */
    void emptyQueueAndNotify(const std::string& errorMessage)
    {
        const joynr::exceptions::JoynrDelayMessageException error(errorMessage);
        for (const auto& entry : _buffer) {
            entry.second(error);
        }
        _buffer.clear();
    }

private:
    using Entry = std::pair<FRAME, IUdsSender::SendFailed>;
    std::deque<Entry> _buffer;
    std::size_t _maxSize;
};

} // namespace joynr

#endif // UDSSENDQUEUE_H
