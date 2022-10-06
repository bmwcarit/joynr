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
#ifndef BYTEBUFFER_H
#define BYTEBUFFER_H

#include <cstdint>
#include <cstring>
#include <vector>

#include <muesli/Traits.h>

namespace joynr
{

namespace detail
{
using InternalByte = std::uint8_t;
using ExternalByte = std::int8_t;

using InternalByteBuffer = std::vector<InternalByte>;
using ExternalByteBuffer = std::vector<ExternalByte>;
} // namespace detail

class ByteBuffer : public detail::InternalByteBuffer
{
public:
    using detail::InternalByteBuffer::InternalByteBuffer;
    using detail::InternalByteBuffer::operator=;

    ByteBuffer();
    ByteBuffer(const ByteBuffer&);
    ByteBuffer(ByteBuffer&&);
    ByteBuffer(const detail::InternalByteBuffer& bb);
    ByteBuffer(detail::InternalByteBuffer&& bb);

    ByteBuffer& operator=(const ByteBuffer&);
    ByteBuffer& operator=(ByteBuffer&&);

    template <typename Archive>
    void save(Archive& archive)
    {
        detail::ExternalByteBuffer externalBuffer;
        copyBuffer(*this, externalBuffer);
        archive(externalBuffer);
    }

    template <typename Archive>
    void load(Archive& archive)
    {
        detail::ExternalByteBuffer externalBuffer;
        archive(externalBuffer);
        copyBuffer(externalBuffer, *this);
    }

private:
    template <typename Source, typename Dest>
    static void copyBuffer(const Source& source, Dest& dest)
    {
        if (!source.data()) {
            dest.resize(0);
            return;
        }
        dest.resize(source.size());
        std::memcpy(dest.data(), source.data(), source.size());
    }
};

} // namespace joynr

namespace muesli
{

template <>
struct SkipIntroOutroTraits<joynr::ByteBuffer> : std::true_type {
};

} // namespace muesli

#endif // BYTEBUFFER_H
