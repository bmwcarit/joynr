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
#ifndef SERIALIZATIONPLACEHOLDER_H
#define SERIALIZATIONPLACEHOLDER_H

#include <type_traits>
#include <memory>

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/type_index.hpp>

#include "joynr/serializer/Serializer.h"
#include "joynr/serializer/Serializable.h"
#include "joynr/serializer/SerializerTraits.h"
#include "joynr/Util.h"

namespace joynr
{
namespace serializer
{

class SerializationPlaceholder
{
    template <typename Archive>
    struct GetDeserializable
    {
        using Tag = typename muesli::TagTraits<Archive>::type;
        using type = typename SerializerTraits<Tag>::template Deserializable<Archive>;
    };

    using DeserializableVariant =
            MakeArchiveVariant<muesli::InputArchiveTypeVector, GetDeserializable>;

public:
    SerializationPlaceholder() = default;
    SerializationPlaceholder(SerializationPlaceholder&&) = default;
    ~SerializationPlaceholder() = default;

    // movable only type
    SerializationPlaceholder(const SerializationPlaceholder&) = delete;
    SerializationPlaceholder& operator=(const SerializationPlaceholder&) = delete;
    SerializationPlaceholder& operator=(SerializationPlaceholder&&) = default;

    template <typename Archive>
    void save(Archive& ar) const
    {
        if (containsOutboundData()) {
            serializable->save(ar);
        } else {
            ar(std::nullptr_t{});
        }
    }

    template <typename Archive>
    void load(Archive& ar)
    {
        using Deserializable = typename GetDeserializable<std::decay_t<Archive>>::type;
        deserializable = Deserializable(ar);
    }

    template <typename... Ts>
    void getData(Ts&... args)
    {
        assert(containsOutboundData() || containsInboundData());
        if (containsOutboundData()) {
            using TypedSerializable = Serializable<OutputArchiveRefVariant, Ts...>;
            const TypedSerializable* typedSerializable =
                    dynamic_cast<const TypedSerializable*>(serializable.get());
            if (typedSerializable != nullptr) {
                std::tie(args...) = typedSerializable->getData();
            } else {
                using RequestedType = std::tuple<Ts...>;
                throw std::invalid_argument(
                        "Serializable mismatch: contains " + serializable->typeName() +
                        " requested" + boost::typeindex::type_id<RequestedType>().pretty_name());
            }
        } else if (containsInboundData()) {
            boost::apply_visitor(
                    [&args...](auto& x) { x.template get<std::tuple<Ts&...>>(std::tie(args...)); },
                    *deserializable);
        }
    }

    template <typename... Ts>
    void setData(Ts&&... arg)
    {
        serializable = std::make_unique<Serializable<OutputArchiveRefVariant, std::decay_t<Ts>...>>(
                std::forward<Ts>(arg)...);
    }

    bool containsOutboundData() const
    {
        return serializable != nullptr;
    }

    bool containsInboundData() const
    {
        return deserializable.is_initialized();
    }

private:
    std::unique_ptr<ISerializable<OutputArchiveRefVariant>> serializable;
    boost::optional<DeserializableVariant> deserializable;
};

} // namespace serializer
} // namespace joynr

namespace muesli
{
template <>
struct SkipIntroOutroTraits<joynr::serializer::SerializationPlaceholder> : std::true_type
{
};
} // namespace muesli

#endif // SERIALIZATIONPLACEHOLDER_H
