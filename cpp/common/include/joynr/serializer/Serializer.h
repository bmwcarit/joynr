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
#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <iostream>
#include <stdexcept>

#include <boost/mpl/transform.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/variant.hpp>

#include "joynr/Util.h"
#include "joynr/serializer/JsonDeserializable.h"

/*
namespace joynr
{
namespace serializer
{

template <typename Serializer>
struct ExtractInputArchive
{
    using type = typename Serializer::InputArchive;
};

template <typename Serializer>
struct ExtractOutputArchive
{
    using type = typename Serializer::OutputArchive;
};

template <typename Serializer>
struct ExtractDeserializable
{
    using type = typename Serializer::Deserializable;
};

template <typename T>
struct AddUniquePtr
{
    using type = std::unique_ptr<T>;
};

template <template <typename> class Extractor,
          template <typename> class Postprocess = boost::mpl::identity>
using MakeSerializerVariant = typename boost::make_variant_over<
        typename boost::mpl::transform<RegisteredSerializers,
                                       Postprocess<Extractor<boost::mpl::_1>>>::type>::type;

template <typename Variant>
class ArchiveVariantWrapper
{
public:
    ArchiveVariantWrapper(Variant&& archiveVariant) : archiveVariant(std::move(archiveVariant))
    {
    }

    template <typename T>
    void operator()(T&& arg)
    {
        boost::apply_visitor(
                [&arg](auto& archive) { (*archive)(std::forward<T>(arg)); }, archiveVariant);
    }

private:
    Variant archiveVariant;
};

using OutputArchiveVariant = MakeSerializerVariant<ExtractOutputArchive, AddUniquePtr>;
using InputArchiveVariant = MakeSerializerVariant<ExtractInputArchive, AddUniquePtr>;
using DeserializableVariant = MakeSerializerVariant<ExtractDeserializable>;

template <typename ArchiveVariant, template <typename> class Extractor, typename Stream>
auto getArchive(const std::string& id, Stream& stream)
{
    ArchiveVariant archive;

    auto fun = [&id, &archive, &stream](auto serializer) {
        using Serializer = decltype(serializer);
        if (id == Serializer::id()) {
            using Archive = typename Extractor<Serializer>::type;
            archive = std::make_unique<Archive>(stream);
            return false;
        }
        return true;
    };

    bool foundSerializer = util::InvokeOn<RegisteredSerializers>(fun);
    if (!foundSerializer) {
        throw std::invalid_argument("no serializer registered for id " + id);
    }

    return ArchiveVariantWrapper<ArchiveVariant>(std::move(archive));
}

inline auto getOutputArchive(const std::string& id, std::ostream& stream)
{
    return getArchive<OutputArchiveVariant, ExtractOutputArchive>(id, stream);
}

inline auto getInputArchive(const std::string& id, std::istream& stream)
{
    return getArchive<InputArchiveVariant, ExtractInputArchive>(id, stream);
}

} // namespace serializer
} // namespace joynr
*/
#endif // SERIALIZER_H
