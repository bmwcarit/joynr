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
#ifndef UDSFRAME_H
#define UDSFRAME_H

#include <cstdint>
#include <limits>

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>

#include <smrf/ByteArrayView.h>
#include <smrf/ByteVector.h>

#include "joynr/Logger.h"
#include "joynr/UdsSettings.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"

namespace joynr
{

/** Frame (de)serializer for UDS frame format specification version 1 (MJI1/MJM1). */
class UdsFrameBufferV1
{
    static constexpr std::size_t _commonMagicBytes =
            2; // Number of bytes all magic numbers have in common
public:
    using Cookie = std::array<smrf::Byte, 4>;
    /** Magic cookie precedes every init-frame */
    static constexpr Cookie _initMagicCookie = {'M', 'J', 'I', '1'};

    /** Magic cookie precedes every message-frame */
    static constexpr Cookie _msgMagicCookie = {'M', 'J', 'M', '1'};

    /** Body length field follows the magic cookie (though UDS is used, the encoding is network
     * byte-order!) */
    using BodyLength = uint32_t;

    /** Constructs empty message- or init-buffer for reading from stream */
    UdsFrameBufferV1() noexcept;

    /** Constructs message buffer from view (bytes from view are copied, empty view results in empty
     * message) */
    explicit UdsFrameBufferV1(const smrf::ByteArrayView& view);

    /** Constructs init-frame buffer */
    explicit UdsFrameBufferV1(const joynr::system::RoutingTypes::UdsClientAddress& clientAddress);

    /** @return Get view on the raw buffer content. */
    boost::asio::const_buffers_1 raw() const noexcept;

    /** @return Get view on the header buffer content for writing. */
    boost::asio::mutable_buffers_1 header() noexcept;

    /**
     * @return Get view on the body buffer content for writing.
     * @throws JoynrRuntimeException if buffer for view cannot be allocated.
     * @throws JoynrRuntimeException if header is unexpected.
     */
    boost::asio::mutable_buffers_1 body();

    /**
     * Read message-body from frame buffer and resets the buffer for the next frame.
     * @return Message in frame
     * @throws JoynrRuntimeException if message cannot be decoded from frame.
     */
    smrf::ByteVector readMessage();

    /**
     * Read init-body from buffer and resets the buffer for the next frame.
     * @return Address in frame
     * @throws JoynrRuntimeException if address cannot be decoded from frame.
     */
    joynr::system::RoutingTypes::UdsClientAddress readInit();

private:
    smrf::ByteVector _buffer;
    static constexpr std::size_t _cookieSize = sizeof(Cookie);

    inline void writeMagicCookie(const Cookie& cookie)
    {
        std::memcpy(_buffer.data(), cookie.data(), _cookieSize);
    }

    inline void checkMagicCookie(const Cookie& cookie,
                                 std::size_t numberOfBytesToCheck = _cookieSize)
    {
        if (0 != std::memcmp(_buffer.data(), cookie.data(), numberOfBytesToCheck)) {
            std::string expected(
                    reinterpret_cast<const char*>(cookie.data()), numberOfBytesToCheck);
            throw joynr::exceptions::JoynrRuntimeException(
                    "UDS frame header does not start with '" + expected + "' magic cookie.");
        }
    }

    static constexpr std::size_t _bodyLengthSize = sizeof(BodyLength);

    inline BodyLength readLength() const
    {
        BodyLength networkByteOrder;
        std::memcpy(&networkByteOrder, _buffer.data() + _cookieSize, _bodyLengthSize);
        return boost::endian::big_to_native(networkByteOrder);
    }

    inline void writeLength(const BodyLength& size)
    {
        BodyLength networkByteOrder = boost::endian::native_to_big(size);
        std::memcpy(_buffer.data() + _cookieSize, &networkByteOrder, _bodyLengthSize);
    }

    static constexpr std::size_t _headerSize = _cookieSize + _bodyLengthSize;
    static constexpr std::size_t _maxBodyLength = std::numeric_limits<BodyLength>::max();
    static_assert(
            sizeof(smrf::ByteVector::size_type) > _bodyLengthSize,
            "Number of bytes transported within one UDS frame do not fit into SMRF byte vector.");
    static inline smrf::ByteVector empty()
    {
        return smrf::ByteVector(_headerSize, 0);
    }

    ADD_LOGGER(UdsFrameBufferV1)
};

} // namespace joynr

#endif // UDSFRAME_H
