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

#include "UdsFrameBufferV1.h"

#include <cstring>
#include <stdexcept>
#include <new>
#include <string>

namespace joynr
{

constexpr UdsFrameBufferV1::Cookie UdsFrameBufferV1::_initMagicCookie;
constexpr UdsFrameBufferV1::Cookie UdsFrameBufferV1::_msgMagicCookie;

UdsFrameBufferV1::UdsFrameBufferV1() noexcept : _buffer(empty())
{
}

UdsFrameBufferV1::UdsFrameBufferV1(const smrf::ByteArrayView& view) : UdsFrameBufferV1()
{
    if (_maxBodyLength < view.size()) {
        // Connection will be closed by server when receiving message with invalid header.
        JOYNR_LOG_ERROR(logger(), "Dropping invalid message of size {}.", view.size());
    } else {
        writeMagicCookie(_msgMagicCookie);
        _buffer.resize(view.size() + _headerSize);
        writeLength(view.size());
        std::memcpy(_buffer.data() + _headerSize, view.data(), view.size());
    }
}

UdsFrameBufferV1::UdsFrameBufferV1(
        const joynr::system::RoutingTypes::UdsClientAddress& clientAddress)
        : UdsFrameBufferV1()
{
    const auto serialized = serializer::serializeToJson(clientAddress);
    const smrf::ByteVector bytes(serialized.begin(), serialized.end());

    if (_maxBodyLength < bytes.size()) {
        // Connection will be closed by server when receiving message with invalid header.
        JOYNR_LOG_ERROR(
                logger(), "Dropping invalid ID due to its serialized size {}.", bytes.size());
    } else {
        writeMagicCookie(_initMagicCookie);
        writeLength(bytes.size());
        _buffer.insert(_buffer.end(), bytes.begin(), bytes.end());
    }
}

boost::asio::const_buffers_1 UdsFrameBufferV1::raw() const noexcept
{
    return boost::asio::const_buffers_1(_buffer.data(), _buffer.size());
}

boost::asio::mutable_buffers_1 UdsFrameBufferV1::header() noexcept
{
    return boost::asio::mutable_buffers_1(_buffer.data(), _headerSize);
}

boost::asio::mutable_buffers_1 UdsFrameBufferV1::body()
{
    checkMagicCookie(_initMagicCookie, _commonMagicBytes);
    try {
        _buffer.resize(readLength() + _headerSize);
    } catch (const std::bad_alloc&) {
        const auto requestedLength = readLength();
        _buffer = empty(); // Assure valid state of buffer
        throw joynr::exceptions::JoynrRuntimeException(
                "Failed to reserve UDS frame buffer of size [bytes]: " +
                std::to_string(requestedLength));
    }
    return boost::asio::mutable_buffers_1(
            _buffer.data() + _headerSize, _buffer.size() - _headerSize);
}

smrf::ByteVector UdsFrameBufferV1::readMessage()
{
    checkMagicCookie(_msgMagicCookie);
    auto tmp = empty();
    _buffer.swap(tmp);
    tmp.erase(tmp.begin(), tmp.begin() + _headerSize);
    return tmp;
}

joynr::system::RoutingTypes::UdsClientAddress UdsFrameBufferV1::readInit()
{
    checkMagicCookie(_initMagicCookie);
    _buffer.erase(_buffer.begin(), _buffer.begin() + _headerSize);
    std::shared_ptr<joynr::system::RoutingTypes::UdsClientAddress> clientAddress;
    const smrf::ByteArrayView initMessage(_buffer);
    try {
        joynr::serializer::deserializeFromJson(clientAddress, initMessage);
    } catch (const std::invalid_argument& e) {
        _buffer = empty(); // Assure valid state of buffer
        throw joynr::exceptions::JoynrRuntimeException(
                std::string("Failed to decode UDS init-message body: ") + e.what());
    }
    _buffer = empty();
    return *clientAddress;
}

} // namespace joynr
