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

#ifndef GENERATED_INTERFACE_JOYNR_TESTS_V2_MULTIPLEVERSIONSINTERFACEASYNCPROXY_H
#define GENERATED_INTERFACE_JOYNR_TESTS_V2_MULTIPLEVERSIONSINTERFACEASYNCPROXY_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/tests/v2/MultipleVersionsInterfaceProxyBase.h"

namespace joynr
{
	template <class ... Ts> class Future;

namespace exceptions
{
	class JoynrException;
	class JoynrRuntimeException;
} // namespace exceptions
} // namespace joynr

#include <cstdint>
#include "joynr/tests/v2/AnonymousVersionedStruct.h"
#include "joynr/tests/v2/MultipleVersionsTypeCollection/VersionedStruct.h"
#include <string>
#include "joynr/tests/v2/InterfaceVersionedStruct.h"

#include <memory>

namespace joynr { namespace tests { namespace v2 { 
/** @brief proxy class for asynchronous calls of interface MultipleVersionsInterface
 *
 * @version 2.0
 */
class  MultipleVersionsInterfaceAsyncProxy :
		virtual public MultipleVersionsInterfaceProxyBase,
		virtual public IMultipleVersionsInterfaceAsync
{
public:

	/**
	 * @brief Parameterized constructor
	 * @param connectorFactory The connector factory
	 * @param domain The provider domain
	 * @param qosSettings The quality of service settings
	 */
	MultipleVersionsInterfaceAsyncProxy(
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
	std::shared_ptr<joynr::Future<joynr::tests::v2::MultipleVersionsTypeCollection::VersionedStruct>>  getVersionedStructAsync(
				const joynr::tests::v2::MultipleVersionsTypeCollection::VersionedStruct& input,
				std::function<void(const joynr::tests::v2::MultipleVersionsTypeCollection::VersionedStruct& result)> onSuccess = nullptr,
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
	std::shared_ptr<joynr::Future<joynr::tests::v2::AnonymousVersionedStruct>>  getAnonymousVersionedStructAsync(
				const joynr::tests::v2::AnonymousVersionedStruct& input,
				std::function<void(const joynr::tests::v2::AnonymousVersionedStruct& result)> onSuccess = nullptr,
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
	std::shared_ptr<joynr::Future<joynr::tests::v2::InterfaceVersionedStruct>>  getInterfaceVersionedStructAsync(
				const joynr::tests::v2::InterfaceVersionedStruct& input,
				std::function<void(const joynr::tests::v2::InterfaceVersionedStruct& result)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none
	) noexcept
	override;

	friend class MultipleVersionsInterfaceProxy;

private:
	DISALLOW_COPY_AND_ASSIGN(MultipleVersionsInterfaceAsyncProxy);
};

} // namespace v2
} // namespace tests
} // namespace joynr
#endif // GENERATED_INTERFACE_JOYNR_TESTS_V2_MULTIPLEVERSIONSINTERFACEASYNCPROXY_H

