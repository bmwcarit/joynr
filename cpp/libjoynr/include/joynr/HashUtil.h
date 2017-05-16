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
#ifndef HASHUTIL_H
#define HASHUTIL_H

#include <functional>
#include <map>
#include <type_traits>

#include "joynr/Util.h"

namespace std
{

inline std::size_t hash_value(const std::vector<bool>::const_reference& r)
{
    return std::hash<bool>()(r);
}

} // namespace std

namespace boost
{

template <class T, class S>
std::size_t hash_value(const std::map<T, S>& v);

template <class T>
std::enable_if_t<joynr::util::IsDerivedFromTemplate<std::map, T>::value, std::size_t> hash_value(
        const T& v);

} // namespace boost

#include <boost/functional/hash.hpp>

namespace boost
{
template <class T, class S>
std::size_t hash_value(const std::map<T, S>& v)
{
    return boost::hash_range(v.begin(), v.end());
}

template <class T>
std::enable_if_t<joynr::util::IsDerivedFromTemplate<std::map, T>::value, std::size_t> hash_value(
        const T& v)
{
    return boost::hash_range(v.begin(), v.end());
}

} // namespace boost
#endif // HASHUTIL_H
