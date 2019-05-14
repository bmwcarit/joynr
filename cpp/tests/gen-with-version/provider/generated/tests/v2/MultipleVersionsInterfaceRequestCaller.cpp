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

#include "joynr/tests/v2/MultipleVersionsInterfaceRequestCaller.h"
#include <cstdint>
#include "joynr/tests/v2/AnonymousVersionedStruct.h"
#include "joynr/tests/v2/MultipleVersionsTypeCollection/VersionedStruct.h"
#include "joynr/tests/v2/InterfaceVersionedStruct.h"
#include "joynr/tests/v2/MultipleVersionsInterfaceProvider.h"

namespace joynr { namespace tests { namespace v2 { 
MultipleVersionsInterfaceRequestCaller::MultipleVersionsInterfaceRequestCaller(std::shared_ptr<joynr::tests::v2::MultipleVersionsInterfaceProvider> provider)
	: joynr::RequestCaller(MultipleVersionsInterfaceProvider::INTERFACE_NAME(), joynr::types::Version(provider->MAJOR_VERSION, provider->MINOR_VERSION)),
	  provider(std::move(provider))
{
}

// attributes
void MultipleVersionsInterfaceRequestCaller::getUInt8Attribute1(
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
		std::string message = "Could not perform MultipleVersionsInterfaceRequestCaller::getUInt8Attribute1, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}
void MultipleVersionsInterfaceRequestCaller::setUInt8Attribute1(
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
		std::string message = "Could not perform MultipleVersionsInterfaceRequestCaller::setUInt8Attribute1, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
	} catch (const exceptions::JoynrException& e) {
		std::string message = "Could not perform MultipleVersionsInterfaceRequestCaller::setUInt8Attribute1, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}

void MultipleVersionsInterfaceRequestCaller::getUInt8Attribute2(
		std::function<void(
				const std::uint8_t& uInt8Attribute2
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
		provider->getUInt8Attribute2(std::move(onSuccess), std::move(onErrorWrapper));
	} catch (const exceptions::ProviderRuntimeException& e) {
		onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
	} catch (const exceptions::JoynrException& e) {
		std::string message = "Could not perform MultipleVersionsInterfaceRequestCaller::getUInt8Attribute2, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}
void MultipleVersionsInterfaceRequestCaller::setUInt8Attribute2(
		const std::uint8_t& uInt8Attribute2,
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
		provider->setUInt8Attribute2(uInt8Attribute2, std::move(onSuccess), std::move(onErrorWrapper));
	} catch (const exceptions::ProviderRuntimeException& e) {
		std::string message = "Could not perform MultipleVersionsInterfaceRequestCaller::setUInt8Attribute2, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
	} catch (const exceptions::JoynrException& e) {
		std::string message = "Could not perform MultipleVersionsInterfaceRequestCaller::setUInt8Attribute2, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}

// methods
void MultipleVersionsInterfaceRequestCaller::getTrue(
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
		std::string message = "Could not perform MultipleVersionsInterfaceRequestCaller::GetTrue, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}

void MultipleVersionsInterfaceRequestCaller::getVersionedStruct(
		const joynr::tests::v2::MultipleVersionsTypeCollection::VersionedStruct& input,
		std::function<void(
				const joynr::tests::v2::MultipleVersionsTypeCollection::VersionedStruct& result
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
		std::string message = "Could not perform MultipleVersionsInterfaceRequestCaller::GetVersionedStruct, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}

void MultipleVersionsInterfaceRequestCaller::getAnonymousVersionedStruct(
		const joynr::tests::v2::AnonymousVersionedStruct& input,
		std::function<void(
				const joynr::tests::v2::AnonymousVersionedStruct& result
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
		std::string message = "Could not perform MultipleVersionsInterfaceRequestCaller::GetAnonymousVersionedStruct, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}

void MultipleVersionsInterfaceRequestCaller::getInterfaceVersionedStruct(
		const joynr::tests::v2::InterfaceVersionedStruct& input,
		std::function<void(
				const joynr::tests::v2::InterfaceVersionedStruct& result
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
		std::string message = "Could not perform MultipleVersionsInterfaceRequestCaller::GetInterfaceVersionedStruct, caught exception: " +
							e.getTypeName() + ":" + e.getMessage();
		onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
	}
}


std::shared_ptr<IJoynrProvider> MultipleVersionsInterfaceRequestCaller::getProvider()
{
	return provider;
}


} // namespace v2
} // namespace tests
} // namespace joynr
