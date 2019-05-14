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
#include "joynr/tests/v1/InterfaceVersionedStruct.h"

namespace joynr { namespace tests { namespace v1 { 

const std::uint32_t InterfaceVersionedStruct::MAJOR_VERSION = 1;
const std::uint32_t InterfaceVersionedStruct::MINOR_VERSION = 0;

InterfaceVersionedStruct::InterfaceVersionedStruct():
	flag(false)
{
}

InterfaceVersionedStruct::InterfaceVersionedStruct(
		const bool &flag
	):
		flag(flag)
{
}


std::size_t InterfaceVersionedStruct::hashCode() const {
	std::size_t seed = 0;

	boost::hash_combine(seed, getFlag());

	return seed;
}

std::string InterfaceVersionedStruct::toString() const {
	std::ostringstream typeAsString;
	typeAsString << "InterfaceVersionedStruct{";
	typeAsString << "flag:" + std::to_string(getFlag());
	typeAsString << "}";
	return typeAsString.str();
}

// printing InterfaceVersionedStruct with google-test and google-mock
void PrintTo(const InterfaceVersionedStruct& interfaceVersionedStruct, ::std::ostream* os) {
	*os << "InterfaceVersionedStruct::" << interfaceVersionedStruct.toString();
}

std::size_t hash_value(const InterfaceVersionedStruct& interfaceVersionedStructValue)
{
	return interfaceVersionedStructValue.hashCode();
}



} // namespace v1
} // namespace tests
} // namespace joynr
