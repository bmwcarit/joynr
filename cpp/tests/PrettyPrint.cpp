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
#include <gtest/gtest.h>
#include "PrettyPrint.h"

#include "joynr/serializer/Serializer.h"
#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/types/Localisation/Trip.h"
#include "joynr/JoynrMessage.h"
#include "joynr/types/TestTypes/TStruct.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"

using namespace joynr;

namespace joynr {
namespace types {

void PrintTo(const joynr::types::TestTypes::TStruct& value, ::std::ostream* os) {
    *os << joynr::serializer::serializeToJson(value);
}

void PrintTo(const joynr::types::Localisation::GpsLocation& value, ::std::ostream* os) {
    *os << joynr::serializer::serializeToJson(value);
}

void PrintTo(const joynr::types::Localisation::Trip& value, ::std::ostream* os) {
    *os << joynr::serializer::serializeToJson(value) << std::endl;
}
void PrintTo(const joynr::types::DiscoveryEntry& value, ::std::ostream* os) {
    *os << joynr::serializer::serializeToJson(value) << std::endl;
}

} // namespace types

namespace system {

void PrintTo(const joynr::system::RoutingTypes::WebSocketAddress& value, ::std::ostream* os) {
    *os << joynr::serializer::serializeToJson(value) << std::endl;
}

} // namespace system

void PrintTo(const JoynrMessage& value, ::std::ostream* os)
{
    *os << joynr::serializer::serializeToJson(value);
}

} // namespace joynr

void PrintTo(const StatusCodeEnum& value, ::std::ostream* os)
{
    *os << StatusCode::toString(value) << std::endl;
}

