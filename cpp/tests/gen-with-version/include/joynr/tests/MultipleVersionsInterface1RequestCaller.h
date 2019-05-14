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
#ifndef GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE1REQUESTCALLER_H
#define GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE1REQUESTCALLER_H

#include <functional>
#include <memory>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/RequestCaller.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/Version.h"
#include "joynr/tests/IMultipleVersionsInterface1.h"

#include "joynr/tests/InterfaceVersionedStruct1.h"
#include <cstdint>
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct1.h"
#include <string>
#include "joynr/tests/AnonymousVersionedStruct1.h"
#include "joynr/Logger.h"

namespace joynr
{
class UnicastBroadcastListener;
class SubscriptionAttributeListener;
} // namespace joynr

namespace joynr { namespace tests { 

class MultipleVersionsInterface1Provider;

/** @brief RequestCaller for interface MultipleVersionsInterface1 */
class  MultipleVersionsInterface1RequestCaller : public joynr::RequestCaller {
public:
	/**
	 * @brief parameterized constructor
	 * @param provider The provider instance
	 */
	explicit MultipleVersionsInterface1RequestCaller(std::shared_ptr<MultipleVersionsInterface1Provider> provider);

	/** @brief Destructor */
	~MultipleVersionsInterface1RequestCaller() override = default;

	// attributes
	/**
	 * @brief Gets the value of the Franca attribute UInt8Attribute1
	 * @param onSuccess A callback function to be called once the asynchronous computation has
	 * finished with success. It must expect a request status object as well as the return value.
	 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
	 */
	virtual void getUInt8Attribute1(
			std::function<void(
					const std::uint8_t&
			)>&& onSuccess,
			std::function<void(
					const std::shared_ptr<exceptions::ProviderRuntimeException>&
			)> onError
	);
	/**
	 * @brief Sets the value of the Franca attribute UInt8Attribute1
	 * @param uInt8Attribute1 The new value of the attribute
	 * @param onSuccess A callback function to be called once the asynchronous computation has
	 * finished with success. It must expect a request status object.
	 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
	 */
	virtual void setUInt8Attribute1(
			const std::uint8_t& uInt8Attribute1,
			std::function<void()>&& onSuccess,
			std::function<void(
					const std::shared_ptr<exceptions::ProviderRuntimeException>&
			)> onError
	);

	// methods
	/**
	 * @brief Implementation of Franca method getTrue
	 * @param onSuccess A callback function to be called once the asynchronous computation has
	 * finished with success. It must expect the output parameter list, if parameters are present.
	 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
	 */
	virtual void getTrue(
			std::function<void(
					const bool& result
			)>&& onSuccess,
			std::function<void(
					const std::shared_ptr<exceptions::JoynrException>&
			)> onError
	);

	/**
	 * @brief Implementation of Franca method getVersionedStruct
	 * @param input Method input parameter input
	 * @param onSuccess A callback function to be called once the asynchronous computation has
	 * finished with success. It must expect the output parameter list, if parameters are present.
	 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
	 */
	virtual void getVersionedStruct(
			const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1& input,
			std::function<void(
					const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1& result
			)>&& onSuccess,
			std::function<void(
					const std::shared_ptr<exceptions::JoynrException>&
			)> onError
	);

	/**
	 * @brief Implementation of Franca method getAnonymousVersionedStruct
	 * @param input Method input parameter input
	 * @param onSuccess A callback function to be called once the asynchronous computation has
	 * finished with success. It must expect the output parameter list, if parameters are present.
	 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
	 */
	virtual void getAnonymousVersionedStruct(
			const joynr::tests::AnonymousVersionedStruct1& input,
			std::function<void(
					const joynr::tests::AnonymousVersionedStruct1& result
			)>&& onSuccess,
			std::function<void(
					const std::shared_ptr<exceptions::JoynrException>&
			)> onError
	);

	/**
	 * @brief Implementation of Franca method getInterfaceVersionedStruct
	 * @param input Method input parameter input
	 * @param onSuccess A callback function to be called once the asynchronous computation has
	 * finished with success. It must expect the output parameter list, if parameters are present.
	 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
	 */
	virtual void getInterfaceVersionedStruct(
			const joynr::tests::InterfaceVersionedStruct1& input,
			std::function<void(
					const joynr::tests::InterfaceVersionedStruct1& result
			)>&& onSuccess,
			std::function<void(
					const std::shared_ptr<exceptions::JoynrException>&
			)> onError
	);


protected:
	std::shared_ptr<IJoynrProvider> getProvider() override;

private:
	DISALLOW_COPY_AND_ASSIGN(MultipleVersionsInterface1RequestCaller);
	std::shared_ptr<joynr::tests::MultipleVersionsInterface1Provider> provider;
	ADD_LOGGER(MultipleVersionsInterface1RequestCaller)
};


} // namespace tests
} // namespace joynr
#endif // GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE1REQUESTCALLER_H
