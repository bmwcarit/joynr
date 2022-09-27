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

#ifndef ENUM_H
#define ENUM_H

#include <boost/bimap.hpp>
#include <boost/preprocessor.hpp>

template <typename EnumType>
struct EnumStringMapHolder;

template <typename EnumType>
using BiMap = boost::bimap<std::string, EnumType>;

template <typename EnumType>
BiMap<EnumType> makeBiMap(const std::initializer_list<typename BiMap<EnumType>::value_type>& list)
{
    return BiMap<EnumType>(list.begin(), list.end());
}

#define STRINGIFY(x) #x
#define EXPAND_AND_STRINGIFY(x) STRINGIFY(x)
#define MAKE_ENUM_STRING_MAP(r, data, elem) (value_type(EXPAND_AND_STRINGIFY(elem), data::elem))

#define JOYNR_ENUM(enum_name, types)                                                               \
    enum class enum_name { BOOST_PP_SEQ_ENUM(types) };                                             \
    template <>                                                                                    \
    struct EnumStringMapHolder<enum_name> {                                                        \
        static auto map()                                                                          \
        {                                                                                          \
            using value_type = typename BiMap<enum_name>::value_type;                              \
            return makeBiMap<enum_name>({BOOST_PP_SEQ_ENUM(                                        \
                    BOOST_PP_SEQ_FOR_EACH(MAKE_ENUM_STRING_MAP, enum_name, types))});              \
        }                                                                                          \
    }

template <typename EnumType, typename = std::enable_if_t<std::is_enum<EnumType>::value>>
std::istream& operator>>(std::istream& in, EnumType& e)
{
    std::string token;
    in >> token;

    if (EnumStringMapHolder<EnumType>::map().left.count(token)) {
        e = EnumStringMapHolder<EnumType>::map().left.at(token);
    } else {
        throw boost::program_options::invalid_option_value("Invalid option");
    }
    return in;
}

template <typename EnumType, typename = std::enable_if_t<std::is_enum<EnumType>::value>>
std::ostream& operator<<(std::ostream& os, EnumType e)
{
    os << EnumStringMapHolder<EnumType>::map().right.at(e);
    return os;
}

#endif // ENUM_H
