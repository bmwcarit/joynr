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
#ifndef GENERATED_INTERFACE_JOYNR_TESTS_DEFAULTMULTIPLEVERSIONSINTERFACEPROVIDER_H
#define GENERATED_INTERFACE_JOYNR_TESTS_DEFAULTMULTIPLEVERSIONSINTERFACEPROVIDER_H

#include <functional>

#include "joynr/tests/IMultipleVersionsInterface.h"
#include "joynr/Logger.h"

#include "joynr/tests/AnonymousVersionedStruct.h"
#include <cstdint>
#include "joynr/tests/InterfaceVersionedStruct.h"
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct.h"

#include "joynr/tests/MultipleVersionsInterfaceAbstractProvider.h"

namespace joynr { namespace tests { 

class  DefaultMultipleVersionsInterfaceProvider : public joynr::tests::MultipleVersionsInterfaceAbstractProvider {

public:
	DefaultMultipleVersionsInterfaceProvider();

	~DefaultMultipleVersionsInterfaceProvider() override;

	// attributes
	void getUInt8Attribute1(
			std::function<void(
					const std::uint8_t&
			)> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;
	void setUInt8Attribute1(
			const std::uint8_t& uInt8Attribute1,
			std::function<void()> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;

	void getUInt8Attribute2(
			std::function<void(
					const std::uint8_t&
			)> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;
	void setUInt8Attribute2(
			const std::uint8_t& uInt8Attribute2,
			std::function<void()> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;

	// methods
	void getTrue(
			std::function<void(
					const bool& result
			)> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;

	void getVersionedStruct(
			const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct& input,
			std::function<void(
					const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct& result
			)> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;

	void getAnonymousVersionedStruct(
			const joynr::tests::AnonymousVersionedStruct& input,
			std::function<void(
					const joynr::tests::AnonymousVersionedStruct& result
			)> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;

	void getInterfaceVersionedStruct(
			const joynr::tests::InterfaceVersionedStruct& input,
			std::function<void(
					const joynr::tests::InterfaceVersionedStruct& result
			)> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;

protected:
	std::uint8_t uInt8Attribute1;
	std::uint8_t uInt8Attribute2;

private:
	ADD_LOGGER(DefaultMultipleVersionsInterfaceProvider)

};


} // namespace tests
} // namespace joynr

#endif // GENERATED_INTERFACE_JOYNR_TESTS_DEFAULTMULTIPLEVERSIONSINTERFACEPROVIDER_H
