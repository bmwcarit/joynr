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
#include <functional>

#include "joynr/tests/MultipleVersionsInterface1RequestCaller.h"
#include "joynr/tests/InterfaceVersionedStruct1.h"
#include <cstdint>
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct1.h"
#include "joynr/tests/AnonymousVersionedStruct1.h"
#include "joynr/tests/MultipleVersionsInterface1Provider.h"

namespace joynr { namespace tests { 
MultipleVersionsInterface1RequestCaller::MultipleVersionsInterface1RequestCaller(std::shared_ptr<joynr::tests::MultipleVersionsInterface1Provider> provider)
	: joynr::RequestCaller(MultipleVersionsInterface1Provider::INTERFACE_NAME(), joynr::types::Version(provider->MAJOR_VERSION, provider->MINOR_VERSION)),
	  provider(std::move(provider))
{
}

// attributes
void MultipleVersionsInterface1RequestCaller::getUInt8Attribute1(
		std::function<void(
				const std::uint8_t& uInt8Attribute1
		)>&& onSuccess,
		std::function<void(
				const std::shared_ptr<exceptions::ProviderRuntimeException>&
		)> onError
) {
	std::function<void(const exceptions::ProviderRuntimeException&)> onErrorWrapper =
	[onError] (const exceptions::ProviderRuntimeException& error) {
		onError(std::make_shared<exceptions::ProviderRuntimeException>(error));
	};
	try {
		provider->getUInt8Attribute1(std::move(onSuccess), std::move(onErrorWrapper));
	} catch (const exceptions::ProviderRuntimeException& e) {
		onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
	} catch (const exceptions::JoynrException& e) {
		std::string message = "Could not perform MultipleVersionsInterface1RequestCaller::getUInt8Attribute1, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}
void MultipleVersionsInterface1RequestCaller::setUInt8Attribute1(
		const std::uint8_t& uInt8Attribute1,
		std::function<void()>&& onSuccess,
		std::function<void(
				const std::shared_ptr<exceptions::ProviderRuntimeException>&
		)> onError
) {
	std::function<void(const exceptions::ProviderRuntimeException&)> onErrorWrapper =
	[onError] (const exceptions::ProviderRuntimeException& error) {
		onError(std::make_shared<exceptions::ProviderRuntimeException>(error));
	};
	try {
		provider->setUInt8Attribute1(uInt8Attribute1, std::move(onSuccess), std::move(onErrorWrapper));
	} catch (const exceptions::ProviderRuntimeException& e) {
		std::string message = "Could not perform MultipleVersionsInterface1RequestCaller::setUInt8Attribute1, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
	} catch (const exceptions::JoynrException& e) {
		std::string message = "Could not perform MultipleVersionsInterface1RequestCaller::setUInt8Attribute1, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}

// methods
void MultipleVersionsInterface1RequestCaller::getTrue(
		std::function<void(
				const bool& result
		)>&& onSuccess,
		std::function<void(
				const std::shared_ptr<exceptions::JoynrException>&
		)> onError
) {
	std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onErrorWrapper =
			[onError] (const joynr::exceptions::ProviderRuntimeException& error) {
				onError(std::make_shared<exceptions::ProviderRuntimeException>(error));
			};
	try {
		provider->getTrue(
				std::move(onSuccess),
				std::move(onErrorWrapper)
		);
	// ApplicationExceptions should not be created by the application itself to ensure
	// serializability. They are treated as JoynrExceptions. They can only be handled correctly
	// if the constructor is used properly (with the appropriate literal of the reported error
	// enumeration).
	} catch (const exceptions::ProviderRuntimeException& e) {
		onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
	} catch (const exceptions::JoynrException& e) {
		std::string message = "Could not perform MultipleVersionsInterface1RequestCaller::GetTrue, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}

void MultipleVersionsInterface1RequestCaller::getVersionedStruct(
		const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1& input,
		std::function<void(
				const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1& result
		)>&& onSuccess,
		std::function<void(
				const std::shared_ptr<exceptions::JoynrException>&
		)> onError
) {
	std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onErrorWrapper =
			[onError] (const joynr::exceptions::ProviderRuntimeException& error) {
				onError(std::make_shared<exceptions::ProviderRuntimeException>(error));
			};
	try {
		provider->getVersionedStruct(
				input,
				std::move(onSuccess),
				std::move(onErrorWrapper)
		);
	// ApplicationExceptions should not be created by the application itself to ensure
	// serializability. They are treated as JoynrExceptions. They can only be handled correctly
	// if the constructor is used properly (with the appropriate literal of the reported error
	// enumeration).
	} catch (const exceptions::ProviderRuntimeException& e) {
		onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
	} catch (const exceptions::JoynrException& e) {
		std::string message = "Could not perform MultipleVersionsInterface1RequestCaller::GetVersionedStruct, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}

void MultipleVersionsInterface1RequestCaller::getAnonymousVersionedStruct(
		const joynr::tests::AnonymousVersionedStruct1& input,
		std::function<void(
				const joynr::tests::AnonymousVersionedStruct1& result
		)>&& onSuccess,
		std::function<void(
				const std::shared_ptr<exceptions::JoynrException>&
		)> onError
) {
	std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onErrorWrapper =
			[onError] (const joynr::exceptions::ProviderRuntimeException& error) {
				onError(std::make_shared<exceptions::ProviderRuntimeException>(error));
			};
	try {
		provider->getAnonymousVersionedStruct(
				input,
				std::move(onSuccess),
				std::move(onErrorWrapper)
		);
	// ApplicationExceptions should not be created by the application itself to ensure
	// serializability. They are treated as JoynrExceptions. They can only be handled correctly
	// if the constructor is used properly (with the appropriate literal of the reported error
	// enumeration).
	} catch (const exceptions::ProviderRuntimeException& e) {
		onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
	} catch (const exceptions::JoynrException& e) {
		std::string message = "Could not perform MultipleVersionsInterface1RequestCaller::GetAnonymousVersionedStruct, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}

void MultipleVersionsInterface1RequestCaller::getInterfaceVersionedStruct(
		const joynr::tests::InterfaceVersionedStruct1& input,
		std::function<void(
				const joynr::tests::InterfaceVersionedStruct1& result
		)>&& onSuccess,
		std::function<void(
				const std::shared_ptr<exceptions::JoynrException>&
		)> onError
) {
	std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onErrorWrapper =
			[onError] (const joynr::exceptions::ProviderRuntimeException& error) {
				onError(std::make_shared<exceptions::ProviderRuntimeException>(error));
			};
	try {
		provider->getInterfaceVersionedStruct(
				input,
				std::move(onSuccess),
				std::move(onErrorWrapper)
		);
	// ApplicationExceptions should not be created by the application itself to ensure
	// serializability. They are treated as JoynrExceptions. They can only be handled correctly
	// if the constructor is used properly (with the appropriate literal of the reported error
	// enumeration).
	} catch (const exceptions::ProviderRuntimeException& e) {
		onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
	} catch (const exceptions::JoynrException& e) {
		std::string message = "Could not perform MultipleVersionsInterface1RequestCaller::GetInterfaceVersionedStruct, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}


std::shared_ptr<IJoynrProvider> MultipleVersionsInterface1RequestCaller::getProvider()
{
	return provider;
}


} // namespace tests
} // namespace joynr
