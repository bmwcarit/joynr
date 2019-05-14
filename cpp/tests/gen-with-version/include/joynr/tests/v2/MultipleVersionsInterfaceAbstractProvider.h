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
#ifndef GENERATED_INTERFACE_JOYNR_TESTS_V2_MULTIPLEVERSIONSINTERFACEABSTRACTPROVIDER_H
#define GENERATED_INTERFACE_JOYNR_TESTS_V2_MULTIPLEVERSIONSINTERFACEABSTRACTPROVIDER_H

#include <string>
#include <vector>
#include <memory>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/AbstractJoynrProvider.h"
#include "joynr/tests/v2/MultipleVersionsInterfaceProvider.h"

#include <cstdint>
#include "joynr/tests/v2/AnonymousVersionedStruct.h"
#include "joynr/tests/v2/MultipleVersionsTypeCollection/VersionedStruct.h"
#include "joynr/tests/v2/InterfaceVersionedStruct.h"


namespace joynr { namespace tests { namespace v2 { 


/** @brief Abstract provider class for interface MultipleVersionsInterface */
class  MultipleVersionsInterfaceAbstractProvider :
		public joynr::tests::v2::MultipleVersionsInterfaceProvider,
		public joynr::AbstractJoynrProvider
{

public:
	/** @brief Default constructor */
	MultipleVersionsInterfaceAbstractProvider();

	/** @brief Destructor */
	~MultipleVersionsInterfaceAbstractProvider() override;

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
	DISALLOW_COPY_AND_ASSIGN(MultipleVersionsInterfaceAbstractProvider);

};

} // namespace v2
} // namespace tests
} // namespace joynr

#endif // GENERATED_INTERFACE_JOYNR_TESTS_V2_MULTIPLEVERSIONSINTERFACEABSTRACTPROVIDER_H
