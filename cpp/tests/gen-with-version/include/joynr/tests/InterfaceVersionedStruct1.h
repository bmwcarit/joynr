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
#ifndef GENERATED_TYPE_JOYNR_TESTS_INTERFACEVERSIONEDSTRUCT1_H
#define GENERATED_TYPE_JOYNR_TESTS_INTERFACEVERSIONEDSTRUCT1_H


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
class  InterfaceVersionedStruct1 {

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
	InterfaceVersionedStruct1();

	// constructor setting all fields
	/**
	 * @brief Parameterized constructor
	 */
	explicit InterfaceVersionedStruct1(
			const bool &flag
	);

	/** @brief Copy constructor */
	InterfaceVersionedStruct1(const InterfaceVersionedStruct1&) = default;

	/** @brief Move constructor */
	InterfaceVersionedStruct1(InterfaceVersionedStruct1&&) = default;

	/** @brief Destructor */
	~InterfaceVersionedStruct1() = default;

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
	InterfaceVersionedStruct1& operator=(const InterfaceVersionedStruct1&) = default;

	/**
	 * @brief move assigns an object
	 * @return reference to the object assigned to
	 */
	InterfaceVersionedStruct1& operator=(InterfaceVersionedStruct1&&) = default;

	/**
	 * @brief "equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	bool operator==(const InterfaceVersionedStruct1& other) const
	{
	    return this->equals(other, joynr::util::MAX_ULPS);
	}

	/**
	 * @brief "not equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are not equal, false otherwise
	 */
	bool operator!=(const InterfaceVersionedStruct1& other) const
	{
	    return !(*this == other);
	}


	// getters
	/**
	 * @brief Gets Flag
	 * @return 
	 */
	inline const bool& getFlag() const { return flag; }

	// setters
	/**
	 * @brief Sets Flag
	 */
	inline void setFlag(const bool& flag) { this->flag = flag; }

	/**
	 * @brief equals method
	 * @param other reference to the object to compare to
	 * @param maxUlps maximum number of ULPs (Units in the Last Place) that are tolerated when comparing to floating point values
	 * @return true if objects are equal, false otherwise
	 */
	bool equals(const InterfaceVersionedStruct1& other, std::size_t maxUlps) const
	{
		return this->equalsInternal(other, maxUlps);
	}
protected:
	// printing InterfaceVersionedStruct1 with google-test and google-mock
	/**
	 * @brief Print values of a InterfaceVersionedStruct1 object
	 * @param interfaceVersionedStruct1 The current object instance
	 * @param os The output stream to send the output to
	 */
	friend void PrintTo(const InterfaceVersionedStruct1& interfaceVersionedStruct1, ::std::ostream* os);

	/**
	 * @brief equals method
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	bool equalsInternal(const InterfaceVersionedStruct1& other, std::size_t maxUlps) const
	{
		return
		joynr::util::compareValues(this->flag, other.flag, maxUlps)
		;
	}


private:
	// serialize InterfaceVersionedStruct1 with muesli
	template <typename Archive>
	friend void serialize(Archive& archive, InterfaceVersionedStruct1& interfaceversionedstruct1Obj);

	// members
	bool flag;
};

std::size_t hash_value(const InterfaceVersionedStruct1& interfaceVersionedStruct1Value);

// serialize InterfaceVersionedStruct1 with muesli
template <typename Archive>
void serialize(Archive& archive, InterfaceVersionedStruct1& interfaceversionedstruct1Obj)
{
archive(
		muesli::make_nvp("flag", interfaceversionedstruct1Obj.flag)
);
}


} // namespace tests
} // namespace joynr

namespace std {

/**
 * @brief Function object that implements a hash function for joynr::tests::InterfaceVersionedStruct1.
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template<>
struct hash<joynr::tests::InterfaceVersionedStruct1> {

	/**
	 * @brief method overriding default implementation of operator ()
	 * @param interfaceVersionedStruct1Value the operators argument
	 * @return the ordinal number representing the enum value
	 */
	std::size_t operator()(const joynr::tests::InterfaceVersionedStruct1& interfaceVersionedStruct1Value) const {
		return joynr::tests::hash_value(interfaceVersionedStruct1Value);
	}
};
} // namespace std

MUESLI_REGISTER_TYPE(joynr::tests::InterfaceVersionedStruct1, "joynr.tests.InterfaceVersionedStruct1")

#endif // GENERATED_TYPE_JOYNR_TESTS_INTERFACEVERSIONEDSTRUCT1_H
