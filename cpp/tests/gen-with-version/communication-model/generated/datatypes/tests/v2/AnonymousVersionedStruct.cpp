/*
 *
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
 *
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
 */

// #####################################################
//#######################################################
//###                                                 ###
//##    WARNING: This file is generated. DO NOT EDIT   ##
//##             All changes will be lost!             ##
//###                                                 ###
//#######################################################
// #####################################################

#include <sstream>
#include <string>

#include <boost/functional/hash.hpp>
#include "joynr/HashUtil.h"
#include "joynr/tests/v2/AnonymousVersionedStruct.h"

namespace joynr { namespace tests { namespace v2 { 

const std::uint32_t AnonymousVersionedStruct::MAJOR_VERSION = 2;
const std::uint32_t AnonymousVersionedStruct::MINOR_VERSION = 0;

AnonymousVersionedStruct::AnonymousVersionedStruct():
	flag2(false)
{
}

AnonymousVersionedStruct::AnonymousVersionedStruct(
		const bool &flag2
	):
		flag2(flag2)
{
}


std::size_t AnonymousVersionedStruct::hashCode() const {
	std::size_t seed = 0;

	boost::hash_combine(seed, getFlag2());

	return seed;
}

std::string AnonymousVersionedStruct::toString() const {
	std::ostringstream typeAsString;
	typeAsString << "AnonymousVersionedStruct{";
	typeAsString << "flag2:" + std::to_string(getFlag2());
	typeAsString << "}";
	return typeAsString.str();
}

// printing AnonymousVersionedStruct with google-test and google-mock
void PrintTo(const AnonymousVersionedStruct& anonymousVersionedStruct, ::std::ostream* os) {
	*os << "AnonymousVersionedStruct::" << anonymousVersionedStruct.toString();
}

std::size_t hash_value(const AnonymousVersionedStruct& anonymousVersionedStructValue)
{
	return anonymousVersionedStructValue.hashCode();
}



} // namespace v2
} // namespace tests
} // namespace joynr
