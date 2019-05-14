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

#ifndef GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE2ASYNCPROXY_H
#define GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE2ASYNCPROXY_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/tests/MultipleVersionsInterface2ProxyBase.h"

namespace joynr
{
	template <class ... Ts> class Future;

namespace exceptions
{
	class JoynrException;
	class JoynrRuntimeException;
} // namespace exceptions
} // namespace joynr

#include "joynr/tests/InterfaceVersionedStruct2.h"
#include <cstdint>
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct2.h"
#include <string>
#include "joynr/tests/AnonymousVersionedStruct2.h"

#include <memory>

namespace joynr { namespace tests { 
/** @brief proxy class for asynchronous calls of interface MultipleVersionsInterface2
 *
 * @version 2.0
 */
class  MultipleVersionsInterface2AsyncProxy :
		virtual public MultipleVersionsInterface2ProxyBase,
		virtual public IMultipleVersionsInterface2Async
{
public:

	/**
	 * @brief Parameterized constructor
	 * @param connectorFactory The connector factory
	 * @param domain The provider domain
	 * @param qosSettings The quality of service settings
	 */
	MultipleVersionsInterface2AsyncProxy(
			std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
			std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
			const std::string& domain,
			const joynr::MessagingQos& qosSettings
	);

	
	/**
	* @brief Asynchronous getter for the uInt8Attribute1 attribute.
	*
	* @param onSuccess A callback function to be called once the asynchronous computation has
	* finished successfully. It must expect the method out parameters.
	* @param onRuntimeError A callback function to be called once the asynchronous computation has
	* failed with an unexpected non-modeled exception. It must expect a JoynrRuntimeException object.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @returns A future representing the result of the asynchronous method call. It provides methods
	* to wait for completion, to get the result or the request status object.
	*/
	std::shared_ptr<joynr::Future<std::uint8_t> > getUInt8Attribute1Async(
				std::function<void(const std::uint8_t& uInt8Attribute1)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none)
				noexcept
	override;
	
	/**
	* @brief Asynchronous getter for the uInt8Attribute2 attribute.
	*
	* @param onSuccess A callback function to be called once the asynchronous computation has
	* finished successfully. It must expect the method out parameters.
	* @param onRuntimeError A callback function to be called once the asynchronous computation has
	* failed with an unexpected non-modeled exception. It must expect a JoynrRuntimeException object.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @returns A future representing the result of the asynchronous method call. It provides methods
	* to wait for completion, to get the result or the request status object.
	*/
	std::shared_ptr<joynr::Future<std::uint8_t> > getUInt8Attribute2Async(
				std::function<void(const std::uint8_t& uInt8Attribute2)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none)
				noexcept
	override;
	
	/**
	* @brief Asynchronous setter for the uInt8Attribute1 attribute.
	*
	* @param uInt8Attribute1 The value to set.
	* @param onSuccess A callback function to be called once the asynchronous computation has
	* finished successfully. It must expect the method out parameters.
	* @param onRuntimeError A callback function to be called once the asynchronous computation has
	* failed with an unexpected non-modeled exception. It must expect a JoynrRuntimeException object.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @returns A future representing the result of the asynchronous method call. It provides methods
	* to wait for completion, to get the result or the request status object.
	*/
	std::shared_ptr<joynr::Future<void> > setUInt8Attribute1Async(
				std::uint8_t uInt8Attribute1,
				std::function<void(void)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none)
				noexcept
	override;
	
	/**
	* @brief Asynchronous setter for the uInt8Attribute2 attribute.
	*
	* @param uInt8Attribute2 The value to set.
	* @param onSuccess A callback function to be called once the asynchronous computation has
	* finished successfully. It must expect the method out parameters.
	* @param onRuntimeError A callback function to be called once the asynchronous computation has
	* failed with an unexpected non-modeled exception. It must expect a JoynrRuntimeException object.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @returns A future representing the result of the asynchronous method call. It provides methods
	* to wait for completion, to get the result or the request status object.
	*/
	std::shared_ptr<joynr::Future<void> > setUInt8Attribute2Async(
				std::uint8_t uInt8Attribute2,
				std::function<void(void)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none)
				noexcept
	override;
	/**
	* @brief Asynchronous operation getTrue.
	*
	* @param onSuccess A callback function to be called once the asynchronous computation has
	* finished successfully. It must expect the method out parameters.
	* @param onRuntimeError A callback function to be called once the asynchronous computation has
	* failed with an unexpected non-modeled exception. It must expect a JoynrRuntimeException object.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @returns A future representing the result of the asynchronous method call. It provides methods
	* to wait for completion, to get the result or the request status object.
	*/
	std::shared_ptr<joynr::Future<bool>>  getTrueAsync(
				std::function<void(const bool& result)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none
	) noexcept
	override;
	/**
	* @brief Asynchronous operation getVersionedStruct.
	*
	* @param onSuccess A callback function to be called once the asynchronous computation has
	* finished successfully. It must expect the method out parameters.
	* @param onRuntimeError A callback function to be called once the asynchronous computation has
	* failed with an unexpected non-modeled exception. It must expect a JoynrRuntimeException object.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @returns A future representing the result of the asynchronous method call. It provides methods
	* to wait for completion, to get the result or the request status object.
	*/
	std::shared_ptr<joynr::Future<joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2>>  getVersionedStructAsync(
				const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& input,
				std::function<void(const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& result)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none
	) noexcept
	override;
	/**
	* @brief Asynchronous operation getAnonymousVersionedStruct.
	*
	* @param onSuccess A callback function to be called once the asynchronous computation has
	* finished successfully. It must expect the method out parameters.
	* @param onRuntimeError A callback function to be called once the asynchronous computation has
	* failed with an unexpected non-modeled exception. It must expect a JoynrRuntimeException object.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @returns A future representing the result of the asynchronous method call. It provides methods
	* to wait for completion, to get the result or the request status object.
	*/
	std::shared_ptr<joynr::Future<joynr::tests::AnonymousVersionedStruct2>>  getAnonymousVersionedStructAsync(
				const joynr::tests::AnonymousVersionedStruct2& input,
				std::function<void(const joynr::tests::AnonymousVersionedStruct2& result)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none
	) noexcept
	override;
	/**
	* @brief Asynchronous operation getInterfaceVersionedStruct.
	*
	* @param onSuccess A callback function to be called once the asynchronous computation has
	* finished successfully. It must expect the method out parameters.
	* @param onRuntimeError A callback function to be called once the asynchronous computation has
	* failed with an unexpected non-modeled exception. It must expect a JoynrRuntimeException object.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @returns A future representing the result of the asynchronous method call. It provides methods
	* to wait for completion, to get the result or the request status object.
	*/
	std::shared_ptr<joynr::Future<joynr::tests::InterfaceVersionedStruct2>>  getInterfaceVersionedStructAsync(
				const joynr::tests::InterfaceVersionedStruct2& input,
				std::function<void(const joynr::tests::InterfaceVersionedStruct2& result)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none
	) noexcept
	override;

	friend class MultipleVersionsInterface2Proxy;

private:
	DISALLOW_COPY_AND_ASSIGN(MultipleVersionsInterface2AsyncProxy);
};

} // namespace tests
} // namespace joynr
#endif // GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE2ASYNCPROXY_H

