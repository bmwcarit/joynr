/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

    using DeserializableVariant = muesli::MakeArchiveVariant<muesli::RegisteredInputArchives,
                                                             muesli::RegisteredInputStreams,
                                                             GetDeserializable>;

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

    template <typename T>
    void getData(T&& arg)
    {
        assert(containsOutboundData() || containsInboundData());
        if (containsOutboundData()) {
            arg = static_cast<const Serializable<std::decay_t<T>>*>(serializable.get())->getData();
        } else if (containsInboundData()) {
            boost::apply_visitor(
                    [&arg](auto& x) { x.template get<T>(std::forward<T>(arg)); }, *deserializable);
        }
    }

    template <typename T>
    void setData(T&& arg)
    {
        serializable = std::make_unique<Serializable<std::decay_t<T>>>(std::forward<T>(arg));
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
    std::unique_ptr<ISerializable> serializable;
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
