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

#ifndef GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE1SYNCPROXY_H
#define GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE1SYNCPROXY_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/tests/MultipleVersionsInterface1ProxyBase.h"

#include "joynr/tests/InterfaceVersionedStruct1.h"
#include <cstdint>
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct1.h"
#include <string>
#include "joynr/tests/AnonymousVersionedStruct1.h"

#include <memory>

namespace joynr { namespace tests { 
/**
 * @brief Synchronous proxy for interface MultipleVersionsInterface1
 *
 * @version 1.0
 */
class  MultipleVersionsInterface1SyncProxy :
		virtual public MultipleVersionsInterface1ProxyBase,
		virtual public IMultipleVersionsInterface1Sync
{
public:
	/**
	 * @brief Parameterized constructor
	 * @param connectorFactory The connector factory
	 * @param domain The provider domain
	 * @param qosSettings The quality of service settings
	 */
	MultipleVersionsInterface1SyncProxy(
			std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
			std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
			const std::string& domain,
			const joynr::MessagingQos& qosSettings
	);

	
	/**
	* @brief Synchronous getter for the uInt8Attribute1 attribute.
	*
	* @param result The result that will be returned to the caller.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void getUInt8Attribute1(std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos = boost::none)
	override;
	
	
	/**
	* @brief Synchronous setter for the uInt8Attribute1 attribute.
	*
	* @param uInt8Attribute1 The value to set.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void setUInt8Attribute1(const std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos = boost::none)
	override;
	/**
	* @brief Synchronous operation getTrue.
	*
	* @param bool result this is an output parameter
	*        and will be set within function getTrue
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void getTrue(
				bool& result,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	override;
	/**
	* @brief Synchronous operation getVersionedStruct.
	*
	* @param joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1 result this is an output parameter
	*        and will be set within function getVersionedStruct
	* @param joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1 input
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void getVersionedStruct(
				joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1& result,
				const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1& input,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	override;
	/**
	* @brief Synchronous operation getAnonymousVersionedStruct.
	*
	* @param joynr::tests::AnonymousVersionedStruct1 result this is an output parameter
	*        and will be set within function getAnonymousVersionedStruct
	* @param joynr::tests::AnonymousVersionedStruct1 input
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void getAnonymousVersionedStruct(
				joynr::tests::AnonymousVersionedStruct1& result,
				const joynr::tests::AnonymousVersionedStruct1& input,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	override;
	/**
	* @brief Synchronous operation getInterfaceVersionedStruct.
	*
	* @param joynr::tests::InterfaceVersionedStruct1 result this is an output parameter
	*        and will be set within function getInterfaceVersionedStruct
	* @param joynr::tests::InterfaceVersionedStruct1 input
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void getInterfaceVersionedStruct(
				joynr::tests::InterfaceVersionedStruct1& result,
				const joynr::tests::InterfaceVersionedStruct1& input,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	override;

	friend class MultipleVersionsInterface1Proxy;

private:
	DISALLOW_COPY_AND_ASSIGN(MultipleVersionsInterface1SyncProxy);
};

} // namespace tests
} // namespace joynr
#endif // GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE1SYNCPROXY_H
