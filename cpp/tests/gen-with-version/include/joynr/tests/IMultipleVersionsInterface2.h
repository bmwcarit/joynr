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

#ifndef GENERATED_INTERFACE_JOYNR_TESTS_IMULTIPLEVERSIONSINTERFACE2_H
#define GENERATED_INTERFACE_JOYNR_TESTS_IMULTIPLEVERSIONSINTERFACE2_H

namespace joynr { namespace tests { 
class InterfaceVersionedStruct2;

} // namespace tests
} // namespace joynr
namespace joynr { namespace tests { namespace MultipleVersionsTypeCollection { 
class VersionedStruct2;

} // namespace MultipleVersionsTypeCollection
} // namespace tests
} // namespace joynr
namespace joynr { namespace tests { 
class AnonymousVersionedStruct2;

} // namespace tests
} // namespace joynr

#include <cstdint>
#include <vector>
#include <string>


#include <memory>
#include <functional>

#include <boost/none.hpp>
#include <boost/optional.hpp>

#include "joynr/MessagingQos.h"

namespace joynr
{
	template <class ... Ts> class Future;

namespace exceptions
{
	class JoynrException;
	class JoynrRuntimeException;
} // namespace exceptions

} // namespace joynr

namespace joynr { namespace tests { 

/**
 * @brief Base interface.
 *
 * @version 2.0
 */
class  IMultipleVersionsInterface2Base {
public:
	IMultipleVersionsInterface2Base() = default;
	virtual ~IMultipleVersionsInterface2Base() = default;

	static const std::string& INTERFACE_NAME();
	/**
	 * @brief MAJOR_VERSION The major version of this provider interface as specified in the
	 * Franca model.
	 */
	static const std::uint32_t MAJOR_VERSION;
	/**
	 * @brief MINOR_VERSION The minor version of this provider interface as specified in the
	 * Franca model.
	 */
	static const std::uint32_t MINOR_VERSION;
};


/**
 * @brief This is the MultipleVersionsInterface2 synchronous interface.
 *
 * @version 2.0
 */
class  IMultipleVersionsInterface2Sync :
		virtual public IMultipleVersionsInterface2Base
{
public:
	~IMultipleVersionsInterface2Sync() override = default;
	
	/**
	* @brief Synchronous getter for the uInt8Attribute1 attribute.
	*
	* @param result The result that will be returned to the caller.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	virtual 
	void getUInt8Attribute1(std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos = boost::none)
	= 0;
	
	
	/**
	* @brief Synchronous getter for the uInt8Attribute2 attribute.
	*
	* @param result The result that will be returned to the caller.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	virtual 
	void getUInt8Attribute2(std::uint8_t& uInt8Attribute2, boost::optional<joynr::MessagingQos> qos = boost::none)
	= 0;
	
	
	/**
	* @brief Synchronous setter for the uInt8Attribute1 attribute.
	*
	* @param uInt8Attribute1 The value to set.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	virtual 
	void setUInt8Attribute1(const std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos = boost::none)
	= 0;
	
	/**
	* @brief Synchronous setter for the uInt8Attribute2 attribute.
	*
	* @param uInt8Attribute2 The value to set.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	virtual 
	void setUInt8Attribute2(const std::uint8_t& uInt8Attribute2, boost::optional<joynr::MessagingQos> qos = boost::none)
	= 0;
	/**
	* @brief Synchronous operation getTrue.
	*
	* @param bool result this is an output parameter
	*        and will be set within function getTrue
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	virtual 
	void getTrue(
				bool& result,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	= 0;
	/**
	* @brief Synchronous operation getVersionedStruct.
	*
	* @param joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2 result this is an output parameter
	*        and will be set within function getVersionedStruct
	* @param joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2 input
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	virtual 
	void getVersionedStruct(
				joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& result,
				const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& input,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	= 0;
	/**
	* @brief Synchronous operation getAnonymousVersionedStruct.
	*
	* @param joynr::tests::AnonymousVersionedStruct2 result this is an output parameter
	*        and will be set within function getAnonymousVersionedStruct
	* @param joynr::tests::AnonymousVersionedStruct2 input
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	virtual 
	void getAnonymousVersionedStruct(
				joynr::tests::AnonymousVersionedStruct2& result,
				const joynr::tests::AnonymousVersionedStruct2& input,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	= 0;
	/**
	* @brief Synchronous operation getInterfaceVersionedStruct.
	*
	* @param joynr::tests::InterfaceVersionedStruct2 result this is an output parameter
	*        and will be set within function getInterfaceVersionedStruct
	* @param joynr::tests::InterfaceVersionedStruct2 input
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	virtual 
	void getInterfaceVersionedStruct(
				joynr::tests::InterfaceVersionedStruct2& result,
				const joynr::tests::InterfaceVersionedStruct2& input,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	= 0;
};

/**
 * @brief This is the MultipleVersionsInterface2 asynchronous interface.
 *
 * @version 2.0
 */
class  IMultipleVersionsInterface2Async :
		virtual public IMultipleVersionsInterface2Base
{
public:
	~IMultipleVersionsInterface2Async() override = default;
	
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
	virtual 
	std::shared_ptr<joynr::Future<std::uint8_t> > getUInt8Attribute1Async(
				std::function<void(const std::uint8_t& uInt8Attribute1)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none)
				noexcept
	= 0;
	
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
	virtual 
	std::shared_ptr<joynr::Future<std::uint8_t> > getUInt8Attribute2Async(
				std::function<void(const std::uint8_t& uInt8Attribute2)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none)
				noexcept
	= 0;
	
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
	virtual 
	std::shared_ptr<joynr::Future<void> > setUInt8Attribute1Async(
				std::uint8_t uInt8Attribute1,
				std::function<void(void)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none)
				noexcept
	= 0;
	
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
	virtual 
	std::shared_ptr<joynr::Future<void> > setUInt8Attribute2Async(
				std::uint8_t uInt8Attribute2,
				std::function<void(void)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none)
				noexcept
	= 0;
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
	virtual 
	std::shared_ptr<joynr::Future<bool>>  getTrueAsync(
				std::function<void(const bool& result)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none
	) noexcept
	= 0;
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
	virtual 
	std::shared_ptr<joynr::Future<joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2>>  getVersionedStructAsync(
				const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& input,
				std::function<void(const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& result)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none
	) noexcept
	= 0;
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
	virtual 
	std::shared_ptr<joynr::Future<joynr::tests::AnonymousVersionedStruct2>>  getAnonymousVersionedStructAsync(
				const joynr::tests::AnonymousVersionedStruct2& input,
				std::function<void(const joynr::tests::AnonymousVersionedStruct2& result)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none
	) noexcept
	= 0;
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
	virtual 
	std::shared_ptr<joynr::Future<joynr::tests::InterfaceVersionedStruct2>>  getInterfaceVersionedStructAsync(
				const joynr::tests::InterfaceVersionedStruct2& input,
				std::function<void(const joynr::tests::InterfaceVersionedStruct2& result)> onSuccess = nullptr,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError = nullptr,
				boost::optional<joynr::MessagingQos> qos = boost::none
	) noexcept
	= 0;
};

/**
 * @brief This is the MultipleVersionsInterface2 interface.
 *
 * @version 2.0
 */
class  IMultipleVersionsInterface2 : virtual public IMultipleVersionsInterface2Sync, virtual public IMultipleVersionsInterface2Async {
public:
	~IMultipleVersionsInterface2() override = default;
	using IMultipleVersionsInterface2Sync::getUInt8Attribute1;
	using IMultipleVersionsInterface2Async::getUInt8Attribute1Async;
	using IMultipleVersionsInterface2Sync::setUInt8Attribute1;
	using IMultipleVersionsInterface2Async::setUInt8Attribute1Async;
	using IMultipleVersionsInterface2Sync::getUInt8Attribute2;
	using IMultipleVersionsInterface2Async::getUInt8Attribute2Async;
	using IMultipleVersionsInterface2Sync::setUInt8Attribute2;
	using IMultipleVersionsInterface2Async::setUInt8Attribute2Async;
	using IMultipleVersionsInterface2Sync::getTrue;
	using IMultipleVersionsInterface2Async::getTrueAsync;
	using IMultipleVersionsInterface2Sync::getAnonymousVersionedStruct;
	using IMultipleVersionsInterface2Async::getAnonymousVersionedStructAsync;
	using IMultipleVersionsInterface2Sync::getInterfaceVersionedStruct;
	using IMultipleVersionsInterface2Async::getInterfaceVersionedStructAsync;
	using IMultipleVersionsInterface2Sync::getVersionedStruct;
	using IMultipleVersionsInterface2Async::getVersionedStructAsync;
};


} // namespace tests
} // namespace joynr
#endif // GENERATED_INTERFACE_JOYNR_TESTS_IMULTIPLEVERSIONSINTERFACE2_H
