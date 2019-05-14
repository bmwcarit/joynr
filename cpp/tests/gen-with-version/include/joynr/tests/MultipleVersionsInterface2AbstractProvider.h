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
#ifndef GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE2ABSTRACTPROVIDER_H
#define GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE2ABSTRACTPROVIDER_H

#include <string>
#include <vector>
#include <memory>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/AbstractJoynrProvider.h"
#include "joynr/tests/MultipleVersionsInterface2Provider.h"

#include "joynr/tests/InterfaceVersionedStruct2.h"
#include <cstdint>
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct2.h"
#include "joynr/tests/AnonymousVersionedStruct2.h"


namespace joynr { namespace tests { 


/** @brief Abstract provider class for interface MultipleVersionsInterface2 */
class  MultipleVersionsInterface2AbstractProvider :
		public joynr::tests::MultipleVersionsInterface2Provider,
		public joynr::AbstractJoynrProvider
{

public:
	/** @brief Default constructor */
	MultipleVersionsInterface2AbstractProvider();

	/** @brief Destructor */
	~MultipleVersionsInterface2AbstractProvider() override;

	/**
	 * @brief Get the interface name
	 * @return The name of the interface
	 */
	const std::string& getInterfaceName() const override;


protected:

	// attributes
	/**
	 * @brief uInt8Attribute1Changed must be called by a concrete provider to signal attribute
	 * modifications. It is used to implement onchange subscriptions.
	 * @param uInt8Attribute1 the new attribute value
	 */
	void uInt8Attribute1Changed(
			const std::uint8_t& uInt8Attribute1
	) override;
	/**
	 * @brief uInt8Attribute2Changed must be called by a concrete provider to signal attribute
	 * modifications. It is used to implement onchange subscriptions.
	 * @param uInt8Attribute2 the new attribute value
	 */
	void uInt8Attribute2Changed(
			const std::uint8_t& uInt8Attribute2
	) override;

private:
	DISALLOW_COPY_AND_ASSIGN(MultipleVersionsInterface2AbstractProvider);

};

} // namespace tests
} // namespace joynr

#endif // GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE2ABSTRACTPROVIDER_H
