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
#ifndef GENERATED_TYPE_JOYNR_TESTS_V1_ANONYMOUSVERSIONEDSTRUCT_H
#define GENERATED_TYPE_JOYNR_TESTS_V1_ANONYMOUSVERSIONEDSTRUCT_H


#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <typeinfo>

#include "joynr/Util.h"
#include "joynr/ByteBuffer.h"

// include complex Datatype headers.

#include "joynr/serializer/Serializer.h"

namespace joynr { namespace tests { namespace v1 { 

/**
 * @version 1.0
 */
class  AnonymousVersionedStruct {

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
	AnonymousVersionedStruct();

	// constructor setting all fields
	/**
	 * @brief Parameterized constructor
	 */
	explicit AnonymousVersionedStruct(
			const bool &flag1
	);

	/** @brief Copy constructor */
	AnonymousVersionedStruct(const AnonymousVersionedStruct&) = default;

	/** @brief Move constructor */
	AnonymousVersionedStruct(AnonymousVersionedStruct&&) = default;

	/** @brief Destructor */
	~AnonymousVersionedStruct() = default;

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
	AnonymousVersionedStruct& operator=(const AnonymousVersionedStruct&) = default;

	/**
	 * @brief move assigns an object
	 * @return reference to the object assigned to
	 */
	AnonymousVersionedStruct& operator=(AnonymousVersionedStruct&&) = default;

	/**
	 * @brief "equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	bool operator==(const AnonymousVersionedStruct& other) const
	{
	    return this->equals(other, joynr::util::MAX_ULPS);
	}

	/**
	 * @brief "not equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are not equal, false otherwise
	 */
	bool operator!=(const AnonymousVersionedStruct& other) const
	{
	    return !(*this == other);
	}


	// getters
	/**
	 * @brief Gets Flag1
	 * @return 
	 */
	inline const bool& getFlag1() const { return flag1; }

	// setters
	/**
	 * @brief Sets Flag1
	 */
	inline void setFlag1(const bool& flag1) { this->flag1 = flag1; }

	/**
	 * @brief equals method
	 * @param other reference to the object to compare to
	 * @param maxUlps maximum number of ULPs (Units in the Last Place) that are tolerated when comparing to floating point values
	 * @return true if objects are equal, false otherwise
	 */
	bool equals(const AnonymousVersionedStruct& other, std::size_t maxUlps) const
	{
		return this->equalsInternal(other, maxUlps);
	}
protected:
	// printing AnonymousVersionedStruct with google-test and google-mock
	/**
	 * @brief Print values of a AnonymousVersionedStruct object
	 * @param anonymousVersionedStruct The current object instance
	 * @param os The output stream to send the output to
	 */
	friend void PrintTo(const AnonymousVersionedStruct& anonymousVersionedStruct, ::std::ostream* os);

	/**
	 * @brief equals method
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	bool equalsInternal(const AnonymousVersionedStruct& other, std::size_t maxUlps) const
	{
		return
		joynr::util::compareValues(this->flag1, other.flag1, maxUlps)
		;
	}


private:
	// serialize AnonymousVersionedStruct with muesli
	template <typename Archive>
	friend void serialize(Archive& archive, AnonymousVersionedStruct& anonymousversionedstructObj);

	// members
	bool flag1;
};

std::size_t hash_value(const AnonymousVersionedStruct& anonymousVersionedStructValue);

// serialize AnonymousVersionedStruct with muesli
template <typename Archive>
void serialize(Archive& archive, AnonymousVersionedStruct& anonymousversionedstructObj)
{
archive(
		muesli::make_nvp("flag1", anonymousversionedstructObj.flag1)
);
}


} // namespace v1
} // namespace tests
} // namespace joynr

namespace std {

/**
 * @brief Function object that implements a hash function for joynr::tests::v1::AnonymousVersionedStruct.
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template<>
struct hash<joynr::tests::v1::AnonymousVersionedStruct> {

	/**
	 * @brief method overriding default implementation of operator ()
	 * @param anonymousVersionedStructValue the operators argument
	 * @return the ordinal number representing the enum value
	 */
	std::size_t operator()(const joynr::tests::v1::AnonymousVersionedStruct& anonymousVersionedStructValue) const {
		return joynr::tests::v1::hash_value(anonymousVersionedStructValue);
	}
};
} // namespace std

MUESLI_REGISTER_TYPE(joynr::tests::v1::AnonymousVersionedStruct, "joynr.tests.v1.AnonymousVersionedStruct")

#endif // GENERATED_TYPE_JOYNR_TESTS_V1_ANONYMOUSVERSIONEDSTRUCT_H
