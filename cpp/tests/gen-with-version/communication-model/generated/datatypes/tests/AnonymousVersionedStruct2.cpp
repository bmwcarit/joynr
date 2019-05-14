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
#include "joynr/tests/AnonymousVersionedStruct2.h"

namespace joynr { namespace tests { 

const std::uint32_t AnonymousVersionedStruct2::MAJOR_VERSION = 2;
const std::uint32_t AnonymousVersionedStruct2::MINOR_VERSION = 0;

AnonymousVersionedStruct2::AnonymousVersionedStruct2():
	flag2(false)
{
}

AnonymousVersionedStruct2::AnonymousVersionedStruct2(
		const bool &flag2
	):
		flag2(flag2)
{
}


std::size_t AnonymousVersionedStruct2::hashCode() const {
	std::size_t seed = 0;

	boost::hash_combine(seed, getFlag2());

	return seed;
}

std::string AnonymousVersionedStruct2::toString() const {
	std::ostringstream typeAsString;
	typeAsString << "AnonymousVersionedStruct2{";
	typeAsString << "flag2:" + std::to_string(getFlag2());
	typeAsString << "}";
	return typeAsString.str();
}

// printing AnonymousVersionedStruct2 with google-test and google-mock
void PrintTo(const AnonymousVersionedStruct2& anonymousVersionedStruct2, ::std::ostream* os) {
	*os << "AnonymousVersionedStruct2::" << anonymousVersionedStruct2.toString();
}

std::size_t hash_value(const AnonymousVersionedStruct2& anonymousVersionedStruct2Value)
{
	return anonymousVersionedStruct2Value.hashCode();
}



} // namespace tests
} // namespace joynr
