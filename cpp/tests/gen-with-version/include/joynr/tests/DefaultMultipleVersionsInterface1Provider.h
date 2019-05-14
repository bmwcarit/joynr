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
#ifndef GENERATED_INTERFACE_JOYNR_TESTS_DEFAULTMULTIPLEVERSIONSINTERFACE1PROVIDER_H
#define GENERATED_INTERFACE_JOYNR_TESTS_DEFAULTMULTIPLEVERSIONSINTERFACE1PROVIDER_H

#include <functional>

#include "joynr/tests/IMultipleVersionsInterface1.h"
#include "joynr/Logger.h"

#include "joynr/tests/InterfaceVersionedStruct1.h"
#include <cstdint>
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct1.h"
#include "joynr/tests/AnonymousVersionedStruct1.h"

#include "joynr/tests/MultipleVersionsInterface1AbstractProvider.h"

namespace joynr { namespace tests { 

class  DefaultMultipleVersionsInterface1Provider : public joynr::tests::MultipleVersionsInterface1AbstractProvider {

public:
	DefaultMultipleVersionsInterface1Provider();

	~DefaultMultipleVersionsInterface1Provider() override;

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

	// methods
	void getTrue(
			std::function<void(
					const bool& result
			)> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;

	void getVersionedStruct(
			const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1& input,
			std::function<void(
					const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1& result
			)> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;

	void getAnonymousVersionedStruct(
			const joynr::tests::AnonymousVersionedStruct1& input,
			std::function<void(
					const joynr::tests::AnonymousVersionedStruct1& result
			)> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;

	void getInterfaceVersionedStruct(
			const joynr::tests::InterfaceVersionedStruct1& input,
			std::function<void(
					const joynr::tests::InterfaceVersionedStruct1& result
			)> onSuccess,
			std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
	) override;

protected:
	std::uint8_t uInt8Attribute1;

private:
	ADD_LOGGER(DefaultMultipleVersionsInterface1Provider)

};


} // namespace tests
} // namespace joynr

#endif // GENERATED_INTERFACE_JOYNR_TESTS_DEFAULTMULTIPLEVERSIONSINTERFACE1PROVIDER_H
