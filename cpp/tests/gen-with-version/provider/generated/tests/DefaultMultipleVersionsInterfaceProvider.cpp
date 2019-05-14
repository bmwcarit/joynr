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
#include "joynr/tests/DefaultMultipleVersionsInterfaceProvider.h"

#include <chrono>
#include <cstdint>
#include <tuple>


namespace joynr { namespace tests { 

DefaultMultipleVersionsInterfaceProvider::DefaultMultipleVersionsInterfaceProvider() :
		MultipleVersionsInterfaceAbstractProvider()
		,
		uInt8Attribute1(),
		uInt8Attribute2()
{
}

DefaultMultipleVersionsInterfaceProvider::~DefaultMultipleVersionsInterfaceProvider() = default;

// attributes
void DefaultMultipleVersionsInterfaceProvider::getUInt8Attribute1(
		std::function<void(
				const std::uint8_t&
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	onSuccess(uInt8Attribute1);
}

void DefaultMultipleVersionsInterfaceProvider::setUInt8Attribute1(
		const std::uint8_t& uInt8Attribute1,
		std::function<void()> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	this->uInt8Attribute1 = uInt8Attribute1;
	uInt8Attribute1Changed(uInt8Attribute1);
	onSuccess();
}

void DefaultMultipleVersionsInterfaceProvider::getUInt8Attribute2(
		std::function<void(
				const std::uint8_t&
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	onSuccess(uInt8Attribute2);
}

void DefaultMultipleVersionsInterfaceProvider::setUInt8Attribute2(
		const std::uint8_t& uInt8Attribute2,
		std::function<void()> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	this->uInt8Attribute2 = uInt8Attribute2;
	uInt8Attribute2Changed(uInt8Attribute2);
	onSuccess();
}

// methods
void DefaultMultipleVersionsInterfaceProvider::getTrue(
		std::function<void(
				const bool& result
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	bool result = false;
	JOYNR_LOG_WARN(logger(), "**********************************************");
	JOYNR_LOG_WARN(logger(), "* DefaultMultipleVersionsInterfaceProvider::getTrue called");
	JOYNR_LOG_WARN(logger(), "**********************************************");
	onSuccess(
			result
	);
}

void DefaultMultipleVersionsInterfaceProvider::getVersionedStruct(
		const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct& input,
		std::function<void(
				const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct& result
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	std::ignore = input;
	joynr::tests::MultipleVersionsTypeCollection::VersionedStruct result;
	JOYNR_LOG_WARN(logger(), "**********************************************");
	JOYNR_LOG_WARN(logger(), "* DefaultMultipleVersionsInterfaceProvider::getVersionedStruct called");
	JOYNR_LOG_WARN(logger(), "**********************************************");
	onSuccess(
			result
	);
}

void DefaultMultipleVersionsInterfaceProvider::getAnonymousVersionedStruct(
		const joynr::tests::AnonymousVersionedStruct& input,
		std::function<void(
				const joynr::tests::AnonymousVersionedStruct& result
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	std::ignore = input;
	joynr::tests::AnonymousVersionedStruct result;
	JOYNR_LOG_WARN(logger(), "**********************************************");
	JOYNR_LOG_WARN(logger(), "* DefaultMultipleVersionsInterfaceProvider::getAnonymousVersionedStruct called");
	JOYNR_LOG_WARN(logger(), "**********************************************");
	onSuccess(
			result
	);
}

void DefaultMultipleVersionsInterfaceProvider::getInterfaceVersionedStruct(
		const joynr::tests::InterfaceVersionedStruct& input,
		std::function<void(
				const joynr::tests::InterfaceVersionedStruct& result
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	std::ignore = input;
	joynr::tests::InterfaceVersionedStruct result;
	JOYNR_LOG_WARN(logger(), "**********************************************");
	JOYNR_LOG_WARN(logger(), "* DefaultMultipleVersionsInterfaceProvider::getInterfaceVersionedStruct called");
	JOYNR_LOG_WARN(logger(), "**********************************************");
	onSuccess(
			result
	);
}


} // namespace tests
} // namespace joynr
