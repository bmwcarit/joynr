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

namespace joynr
{

constexpr UdsFrameBufferV1::Cookie UdsFrameBufferV1::_initMagicCookie;
constexpr UdsFrameBufferV1::Cookie UdsFrameBufferV1::_msgMagicCookie;

UdsFrameBufferV1::UdsFrameBufferV1() noexcept : _isValid{false}, _buffer(empty())
{
}

UdsFrameBufferV1::UdsFrameBufferV1(const smrf::ByteArrayView& view) : UdsFrameBufferV1()
{
    if (_maxBodyLength < view.size()) {
        throw joynr::exceptions::JoynrRuntimeException("Frame payload size invalid " +
                                                       std::to_string(view.size()));
    } else {
        resizeBufferPayload(_buffer, view.size());
        writeMagicCookie(_msgMagicCookie);
        writeLength(view.size());
        std::memcpy(_buffer.data() + _headerSize, view.data(), view.size());
        _isValid = true;
    }
}

UdsFrameBufferV1::UdsFrameBufferV1(
        const joynr::system::RoutingTypes::UdsClientAddress& clientAddress)
        : UdsFrameBufferV1(smrf::ByteArrayView(serializeClientAddress(clientAddress)))
{
    writeMagicCookie(_initMagicCookie);
    _isValid = true;
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
    resizeBufferPayload(_buffer, readLength());
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
