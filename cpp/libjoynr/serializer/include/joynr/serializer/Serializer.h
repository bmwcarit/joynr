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
#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/mpl/identity.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/variant.hpp>

// order of includes is relevant due to muesli's registration mechanism
// clang-format off
#include <muesli/archives/json/JsonInputArchive.h>
#include <muesli/archives/json/JsonOutputArchive.h>
#include <muesli/streams/StringIStream.h>
#include <muesli/streams/StringOStream.h>
#include <muesli/ArchiveRegistry.h>
#include <muesli/TypeRegistry.h>
#include <muesli/Registry.h>
// clang-format on

#include <smrf/ByteArrayView.h>

#include "joynr/Util.h"
#include "joynr/serializer/JsonDeserializable.h"

namespace joynr
{
namespace serializer
{

template <typename ArchiveTypeVector, template <typename> class Postprocess = boost::mpl::identity>
using MakeArchiveVariant = typename boost::make_variant_over<
        typename boost::mpl::transform<ArchiveTypeVector, Postprocess<boost::mpl::_1>>::type>::type;

using OutputArchiveRefVariant =
        MakeArchiveVariant<muesli::OutputArchiveTypeVector, std::add_lvalue_reference>;

template <typename T>
struct AddSharedPtr {
    using type = std::shared_ptr<T>;
};

template <typename Variant>
class ArchiveVariantWrapper
{
public:
    explicit ArchiveVariantWrapper(Variant&& archiveVariant)
            : _archiveVariant(std::move(archiveVariant))
    {
    }

    template <typename... Ts>
    void operator()(Ts&&... args)
    {
        boost::apply_visitor([&args...](auto&& archive) { (*archive)(std::forward<Ts>(args)...); },
                             _archiveVariant);
    }

private:
    Variant _archiveVariant;
};

template <typename ArchiveVariant, typename RegisteredArchives, typename Stream>
auto getArchive(const std::string& id, Stream& stream)
{
    ArchiveVariant archive;

    auto fun = [&id, &archive, &stream](auto&& archiveHolder) {
        using ArchiveInTemplateHolder = std::decay_t<decltype(archiveHolder)>;
        using ArchiveImpl = typename ArchiveInTemplateHolder::template type<Stream>;
        using ArchiveTag = typename muesli::TagTraits<ArchiveImpl>::type;
        if (id == SerializerTraits<ArchiveTag>::id()) {
            archive = std::make_shared<ArchiveImpl>(stream);
            return false;
        }
        return true;
    };

    bool foundSerializer = util::invokeOn<RegisteredArchives>(fun);
    if (!foundSerializer) {
        throw std::invalid_argument("no serializer registered for id " + id);
    }

    return ArchiveVariantWrapper<ArchiveVariant>(std::move(archive));
}

template <typename OutputStream>
inline auto getOutputArchive(const std::string& id, OutputStream& stream)
{
    using OutputArchiveVariant = MakeArchiveVariant<muesli::OutputArchiveTypeVector, AddSharedPtr>;
    return getArchive<OutputArchiveVariant, muesli::RegisteredOutputArchives>(id, stream);
}

template <typename InputStream>
inline auto getInputArchive(const std::string& id, InputStream& stream)
{
    using InputArchiveVariant = MakeArchiveVariant<muesli::InputArchiveTypeVector, AddSharedPtr>;
    return getArchive<InputArchiveVariant, muesli::RegisteredInputArchives>(id, stream);
}

namespace detail
{
template <typename T, typename InputStream>
void deserializeFromJson(T& value, InputStream& stream)
{
    using InputArchive = muesli::JsonInputArchive<InputStream>;
    auto iarchive = std::make_shared<InputArchive>(stream);
    (*iarchive)(value);
}
} // namespace detail

template <typename T>
void deserializeFromJson(T& value, const std::string& str)
{
    muesli::StringIStream stream(str);
    detail::deserializeFromJson(value, stream);
}

template <typename T>
void deserializeFromJson(T& value, std::string&& str)
{
    muesli::StringIStream stream(std::move(str));
    detail::deserializeFromJson(value, stream);
}

template <typename T>
void deserializeFromJson(T& value, const smrf::ByteArrayView& byteArrayView)
{
    // TODO we need to be able to directly parse from a smrf::ByteVector
    // without having to copy to a string
    std::string str(byteArrayView.data(), byteArrayView.data() + byteArrayView.size());
    deserializeFromJson(value, std::move(str));
}

template <typename T>
std::string serializeToJson(const T& value)
{
    using OutputStream = muesli::StringOStream;
    using OutputArchive = muesli::JsonOutputArchive<OutputStream>;
    OutputStream ostream;
    OutputArchive oarchive(ostream);
    oarchive(value);
    return ostream.getString();
}

} // namespace serializer
} // namespace joynr

#endif // SERIALIZER_H
