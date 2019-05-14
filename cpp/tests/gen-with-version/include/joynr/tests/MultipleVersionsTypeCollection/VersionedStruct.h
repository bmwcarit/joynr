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
#ifndef GENERATED_TYPE_JOYNR_TESTS_MULTIPLEVERSIONSTYPECOLLECTION_VERSIONEDSTRUCT_H
#define GENERATED_TYPE_JOYNR_TESTS_MULTIPLEVERSIONSTYPECOLLECTION_VERSIONEDSTRUCT_H


#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <typeinfo>

#include "joynr/Util.h"
#include "joynr/ByteBuffer.h"

// include complex Datatype headers.

#include "joynr/serializer/Serializer.h"

namespace joynr { namespace tests { namespace MultipleVersionsTypeCollection { 

/**
 * @version 2.0
 */
class  VersionedStruct {

public:
	/**
	 * @brief MAJOR_VERSION The major version of this struct as specified in the
	 * type collection or interface in the Franca model.
	 */
	static const std::uint32_t MAJOR_VERSION;
	/**
	 * @brief MINOR_VERSION The minor version of this struct as specified in the
	 * type collection or interface in the Franca model.
	 */
	static const std::uint32_t MINOR_VERSION;

	// general methods

	// default constructor
	/** @brief Constructor */
	VersionedStruct();

	// constructor setting all fields
	/**
	 * @brief Parameterized constructor
	 */
	explicit VersionedStruct(
			const bool &flag2
	);

	/** @brief Copy constructor */
	VersionedStruct(const VersionedStruct&) = default;

	/** @brief Move constructor */
	VersionedStruct(VersionedStruct&&) = default;

	/** @brief Destructor */
	~VersionedStruct() = default;

	/**
	 * @brief Stringifies the class
	 * @return stringified class content
	 */
	std::string toString() const;

	/**
	 * @brief Returns a hash code value for this object
	 * @return a hash code value for this object.
	 */
	std::size_t hashCode() const;

	/**
	 * @brief assigns an object
	 * @return reference to the object assigned to
	 */
	VersionedStruct& operator=(const VersionedStruct&) = default;

	/**
	 * @brief move assigns an object
	 * @return reference to the object assigned to
	 */
	VersionedStruct& operator=(VersionedStruct&&) = default;

	/**
	 * @brief "equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	bool operator==(const VersionedStruct& other) const
	{
	    return this->equals(other, joynr::util::MAX_ULPS);
	}

	/**
	 * @brief "not equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are not equal, false otherwise
	 */
	bool operator!=(const VersionedStruct& other) const
	{
	    return !(*this == other);
	}


	// getters
	/**
	 * @brief Gets Flag2
	 * @return 
	 */
	inline const bool& getFlag2() const { return flag2; }

	// setters
	/**
	 * @brief Sets Flag2
	 */
	inline void setFlag2(const bool& flag2) { this->flag2 = flag2; }

	/**
	 * @brief equals method
	 * @param other reference to the object to compare to
	 * @param maxUlps maximum number of ULPs (Units in the Last Place) that are tolerated when comparing to floating point values
	 * @return true if objects are equal, false otherwise
	 */
	bool equals(const VersionedStruct& other, std::size_t maxUlps) const
	{
		return this->equalsInternal(other, maxUlps);
	}
protected:
	// printing VersionedStruct with google-test and google-mock
	/**
	 * @brief Print values of a VersionedStruct object
	 * @param versionedStruct The current object instance
	 * @param os The output stream to send the output to
	 */
	friend void PrintTo(const VersionedStruct& versionedStruct, ::std::ostream* os);

	/**
	 * @brief equals method
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	bool equalsInternal(const VersionedStruct& other, std::size_t maxUlps) const
	{
		return
		joynr::util::compareValues(this->flag2, other.flag2, maxUlps)
		;
	}


private:
	// serialize VersionedStruct with muesli
	template <typename Archive>
	friend void serialize(Archive& archive, VersionedStruct& versionedstructObj);

	// members
	bool flag2;
};

std::size_t hash_value(const VersionedStruct& versionedStructValue);

// serialize VersionedStruct with muesli
template <typename Archive>
void serialize(Archive& archive, VersionedStruct& versionedstructObj)
{
archive(
		muesli::make_nvp("flag2", versionedstructObj.flag2)
);
}


} // namespace MultipleVersionsTypeCollection
} // namespace tests
} // namespace joynr

namespace std {

/**
 * @brief Function object that implements a hash function for joynr::tests::MultipleVersionsTypeCollection::VersionedStruct.
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template<>
struct hash<joynr::tests::MultipleVersionsTypeCollection::VersionedStruct> {

	/**
	 * @brief method overriding default implementation of operator ()
	 * @param versionedStructValue the operators argument
	 * @return the ordinal number representing the enum value
	 */
	std::size_t operator()(const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct& versionedStructValue) const {
		return joynr::tests::MultipleVersionsTypeCollection::hash_value(versionedStructValue);
	}
};
} // namespace std

MUESLI_REGISTER_TYPE(joynr::tests::MultipleVersionsTypeCollection::VersionedStruct, "joynr.tests.MultipleVersionsTypeCollection.VersionedStruct")

#endif // GENERATED_TYPE_JOYNR_TESTS_MULTIPLEVERSIONSTYPECOLLECTION_VERSIONEDSTRUCT_H
