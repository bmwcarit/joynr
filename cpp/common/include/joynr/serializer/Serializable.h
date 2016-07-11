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
#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include <type_traits>

#include <boost/variant/apply_visitor.hpp>
#include <muesli/Registry.h>

namespace joynr
{
namespace serializer
{

class ISerializable
{
public:
    virtual ~ISerializable() = default;

    template <typename Archive>
    void save(Archive& ar) const
    {
        saveImpl(muesli::OutputArchiveVariant(ar));
    }

protected:
    virtual void saveImpl(muesli::OutputArchiveVariant&& ar) const = 0;
};

template <typename T>
struct Serializable : ISerializable
{
    Serializable(T&& data) : storage(std::move(data))
    {
    }

    Serializable(const T& data) = delete;

    T getData() const
    {
        return storage;
    }

protected:
    void saveImpl(muesli::OutputArchiveVariant&& ar) const override
    {
        boost::apply_visitor([this](auto& archive) { archive(storage); }, ar);
    }

private:
    T storage;
};

} // namespace serializer
} // namespace joynr

#endif // SERIALIZABLE_H
