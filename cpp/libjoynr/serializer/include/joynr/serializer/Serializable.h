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
#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include <string>
#include <tuple>
#include <utility>

#include <boost/type_index.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace joynr
{
namespace serializer
{

template <typename Variant>
class ISerializable
{
public:
    virtual ~ISerializable() = default;

    template <typename Archive>
    void save(Archive& ar) const
    {
        saveImpl(Variant(ar));
    }

    virtual std::string typeName() = 0;

protected:
    virtual void saveImpl(Variant&& ar) const = 0;
};

template <typename Variant, typename... Ts>
struct Serializable : ISerializable<Variant>
{
    template <typename... Xs>
    explicit Serializable(Xs&&... data)
            : storage(std::forward<Xs>(data)...)
    {
    }

    const std::tuple<Ts...>& getData() const
    {
        return storage;
    }

    std::string typeName() override
    {
        return boost::typeindex::type_id<std::tuple<Ts...>>().pretty_name();
    }

protected:
    void saveImpl(Variant&& ar) const override
    {
        boost::apply_visitor([this](auto& archive) { archive(storage); }, ar);
    }

private:
    std::tuple<Ts...> storage;
};

} // namespace serializer
} // namespace joynr

#endif // SERIALIZABLE_H
