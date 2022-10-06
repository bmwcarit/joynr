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
#ifndef JSONDESERIALIZABLE_H
#define JSONDESERIALIZABLE_H

#include <memory>
#include <utility>

#include <muesli/SkipIntroOutroWrapper.h>

#include "joynr/serializer/SerializerTraits.h"

namespace joynr
{
namespace serializer
{

template <typename Archive>
class JsonDeserializable
{
public:
    explicit JsonDeserializable(Archive& archive) : jsonInputArchive()
    {
        archive.pushNullableNode();
        if (!archive.currentValueIsNull()) {
            jsonInputArchive = archive.shared_from_this();
            jsonInputArchive->pushState();
        }
        archive.popNode();
    }

    template <typename Tuple>
    void get(Tuple&& value)
    {
        assert(jsonInputArchive);
        jsonInputArchive->popState();
        (*jsonInputArchive)(muesli::SkipIntroOutroWrapper<std::decay_t<Tuple>>(&value));
        jsonInputArchive.reset();
    }

private:
    std::shared_ptr<Archive> jsonInputArchive;
};

template <>
struct SerializerTraits<muesli::tags::json> {
    static constexpr const char* id()
    {
        return "json";
    }
    template <typename Archive>
    using Deserializable = JsonDeserializable<Archive>;
};

} // namespace serializer
} // namespace joynr

#endif // JSONDESERIALIZABLE_H
