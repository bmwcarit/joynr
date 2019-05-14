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
#ifndef GENERATED_TYPE_JOYNR_TESTS_ANONYMOUSVERSIONEDSTRUCT1_H
#define GENERATED_TYPE_JOYNR_TESTS_ANONYMOUSVERSIONEDSTRUCT1_H


#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <typeinfo>

#include "joynr/Util.h"
#include "joynr/ByteBuffer.h"

// include complex Datatype headers.

#include "joynr/serializer/Serializer.h"

namespace joynr { namespace tests { 

/**
 * @version 1.0
 */
class  AnonymousVersionedStruct1 {

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
	AnonymousVersionedStruct1();

	// constructor setting all fields
	/**
	 * @brief Parameterized constructor
	 */
	explicit AnonymousVersionedStruct1(
			const bool &flag1
	);

	/** @brief Copy constructor */
	AnonymousVersionedStruct1(const AnonymousVersionedStruct1&) = default;

	/** @brief Move constructor */
	AnonymousVersionedStruct1(AnonymousVersionedStruct1&&) = default;

	/** @brief Destructor */
	~AnonymousVersionedStruct1() = default;

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
	AnonymousVersionedStruct1& operator=(const AnonymousVersionedStruct1&) = default;

	/**
	 * @brief move assigns an object
	 * @return reference to the object assigned to
	 */
	AnonymousVersionedStruct1& operator=(AnonymousVersionedStruct1&&) = default;

	/**
	 * @brief "equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	bool operator==(const AnonymousVersionedStruct1& other) const
	{
	    return this->equals(other, joynr::util::MAX_ULPS);
	}

	/**
	 * @brief "not equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are not equal, false otherwise
	 */
	bool operator!=(const AnonymousVersionedStruct1& other) const
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
	bool equals(const AnonymousVersionedStruct1& other, std::size_t maxUlps) const
	{
		return this->equalsInternal(other, maxUlps);
	}
protected:
	// printing AnonymousVersionedStruct1 with google-test and google-mock
	/**
	 * @brief Print values of a AnonymousVersionedStruct1 object
	 * @param anonymousVersionedStruct1 The current object instance
	 * @param os The output stream to send the output to
	 */
	friend void PrintTo(const AnonymousVersionedStruct1& anonymousVersionedStruct1, ::std::ostream* os);

	/**
	 * @brief equals method
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	bool equalsInternal(const AnonymousVersionedStruct1& other, std::size_t maxUlps) const
	{
		return
		joynr::util::compareValues(this->flag1, other.flag1, maxUlps)
		;
	}


private:
	// serialize AnonymousVersionedStruct1 with muesli
	template <typename Archive>
	friend void serialize(Archive& archive, AnonymousVersionedStruct1& anonymousversionedstruct1Obj);

	// members
	bool flag1;
};

std::size_t hash_value(const AnonymousVersionedStruct1& anonymousVersionedStruct1Value);

// serialize AnonymousVersionedStruct1 with muesli
template <typename Archive>
void serialize(Archive& archive, AnonymousVersionedStruct1& anonymousversionedstruct1Obj)
{
archive(
		muesli::make_nvp("flag1", anonymousversionedstruct1Obj.flag1)
);
}


} // namespace tests
} // namespace joynr

namespace std {

/**
 * @brief Function object that implements a hash function for joynr::tests::AnonymousVersionedStruct1.
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template<>
struct hash<joynr::tests::AnonymousVersionedStruct1> {

	/**
	 * @brief method overriding default implementation of operator ()
	 * @param anonymousVersionedStruct1Value the operators argument
	 * @return the ordinal number representing the enum value
	 */
	std::size_t operator()(const joynr::tests::AnonymousVersionedStruct1& anonymousVersionedStruct1Value) const {
		return joynr::tests::hash_value(anonymousVersionedStruct1Value);
	}
};
} // namespace std

MUESLI_REGISTER_TYPE(joynr::tests::AnonymousVersionedStruct1, "joynr.tests.AnonymousVersionedStruct1")

#endif // GENERATED_TYPE_JOYNR_TESTS_ANONYMOUSVERSIONEDSTRUCT1_H
