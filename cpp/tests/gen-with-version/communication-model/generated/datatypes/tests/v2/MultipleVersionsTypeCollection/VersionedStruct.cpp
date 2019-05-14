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
#include "joynr/tests/v2/MultipleVersionsTypeCollection/VersionedStruct.h"

namespace joynr { namespace tests { namespace v2 { namespace MultipleVersionsTypeCollection { 

const std::uint32_t VersionedStruct::MAJOR_VERSION = 2;
const std::uint32_t VersionedStruct::MINOR_VERSION = 0;

VersionedStruct::VersionedStruct():
	flag2(false)
{
}

VersionedStruct::VersionedStruct(
		const bool &flag2
	):
		flag2(flag2)
{
}


std::size_t VersionedStruct::hashCode() const {
	std::size_t seed = 0;

	boost::hash_combine(seed, getFlag2());

	return seed;
}

std::string VersionedStruct::toString() const {
	std::ostringstream typeAsString;
	typeAsString << "VersionedStruct{";
	typeAsString << "flag2:" + std::to_string(getFlag2());
	typeAsString << "}";
	return typeAsString.str();
}

// printing VersionedStruct with google-test and google-mock
void PrintTo(const VersionedStruct& versionedStruct, ::std::ostream* os) {
	*os << "VersionedStruct::" << versionedStruct.toString();
}

std::size_t hash_value(const VersionedStruct& versionedStructValue)
{
	return versionedStructValue.hashCode();
}



} // namespace MultipleVersionsTypeCollection
} // namespace v2
} // namespace tests
} // namespace joynr
