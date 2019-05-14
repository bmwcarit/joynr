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
#include "joynr/tests/AnonymousVersionedStruct1.h"

namespace joynr { namespace tests { 

const std::uint32_t AnonymousVersionedStruct1::MAJOR_VERSION = 1;
const std::uint32_t AnonymousVersionedStruct1::MINOR_VERSION = 0;

AnonymousVersionedStruct1::AnonymousVersionedStruct1():
	flag1(false)
{
}

AnonymousVersionedStruct1::AnonymousVersionedStruct1(
		const bool &flag1
	):
		flag1(flag1)
{
}


std::size_t AnonymousVersionedStruct1::hashCode() const {
	std::size_t seed = 0;

	boost::hash_combine(seed, getFlag1());

	return seed;
}

std::string AnonymousVersionedStruct1::toString() const {
	std::ostringstream typeAsString;
	typeAsString << "AnonymousVersionedStruct1{";
	typeAsString << "flag1:" + std::to_string(getFlag1());
	typeAsString << "}";
	return typeAsString.str();
}

// printing AnonymousVersionedStruct1 with google-test and google-mock
void PrintTo(const AnonymousVersionedStruct1& anonymousVersionedStruct1, ::std::ostream* os) {
	*os << "AnonymousVersionedStruct1::" << anonymousVersionedStruct1.toString();
}

std::size_t hash_value(const AnonymousVersionedStruct1& anonymousVersionedStruct1Value)
{
	return anonymousVersionedStruct1Value.hashCode();
}



} // namespace tests
} // namespace joynr
