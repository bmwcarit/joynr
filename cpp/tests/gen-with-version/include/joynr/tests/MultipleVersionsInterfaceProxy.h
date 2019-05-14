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

#ifndef GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACEPROXY_H
#define GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACEPROXY_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/tests/AnonymousVersionedStruct.h"
#include <cstdint>
#include "joynr/tests/InterfaceVersionedStruct.h"
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct.h"
#include <string>
#include <memory>

#include "joynr/tests/MultipleVersionsInterfaceSyncProxy.h"
#include "joynr/tests/MultipleVersionsInterfaceAsyncProxy.h"
#include "joynr/tests/IMultipleVersionsInterface.h"

#ifdef _MSC_VER
	// Visual C++ gives a warning which is caused by diamond inheritance, but this is
	// not relevant when using pure virtual methods:
	// http://msdn.microsoft.com/en-us/library/6b3sy7ae(v=vs.80).aspx
	#pragma warning( disable : 4250 )
#endif

namespace joynr { namespace tests { 
/**
 * @brief Proxy class for interface MultipleVersionsInterface
 *
 * @version 2.0
 */
class  MultipleVersionsInterfaceProxy : virtual public IMultipleVersionsInterface, virtual public MultipleVersionsInterfaceSyncProxy, virtual public MultipleVersionsInterfaceAsyncProxy {
public:
	/**
	 * @brief Parameterized constructor
	 * @param connectorFactory The connector factory
	 * @param domain The provider domain
	 * @param qosSettings The quality of service settings
	 */
	MultipleVersionsInterfaceProxy(
			std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
			std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
			const std::string& domain,
			const joynr::MessagingQos& qosSettings
	);

	/**
	 * @brief unsubscribes from attribute UInt8Attribute1
	 * @param subscriptionId The subscription id returned earlier on creation of the subscription
	 */
	void unsubscribeFromUInt8Attribute1(const std::string &subscriptionId) override {
		MultipleVersionsInterfaceProxyBase::unsubscribeFromUInt8Attribute1(subscriptionId);
	}

	/**
	 * @brief creates a new subscription to attribute UInt8Attribute1
	 * @param subscriptionListener The listener callback providing methods to call on publication and failure
	 * @param subscriptionQos The subscription quality of service settings
	 * @return a future representing the result (subscription id) as string. It provides methods to wait for
	 * completion, to get the subscription id or the request status object. The subscription id will be available
	 * when the subscription is successfully registered at the provider.
	 */
	std::shared_ptr<joynr::Future<std::string>> subscribeToUInt8Attribute1(
				std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
				std::shared_ptr<joynr::SubscriptionQos> subscriptionQos)
	 override {
		return MultipleVersionsInterfaceProxyBase::subscribeToUInt8Attribute1(
					subscriptionListener,
					subscriptionQos);
	}

	/**
	 * @brief updates an existing subscription to attribute UInt8Attribute1
	 * @param subscriptionListener The listener callback providing methods to call on publication and failure
	 * @param subscriptionQos The subscription quality of service settings
	 * @param subscriptionId The subscription id returned earlier on creation of the subscription
	 * @return a future representing the result (subscription id) as string. It provides methods to wait for
	 * completion, to get the subscription id or the request status object. The subscription id will be available
	 * when the subscription is successfully registered at the provider.
	 */
	std::shared_ptr<joynr::Future<std::string>> subscribeToUInt8Attribute1(
				std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
				std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
				const std::string& subscriptionId)
	 override{
		return MultipleVersionsInterfaceProxyBase::subscribeToUInt8Attribute1(
					subscriptionListener,
					subscriptionQos,
					subscriptionId);
	}

	/**
	 * @brief unsubscribes from attribute UInt8Attribute2
	 * @param subscriptionId The subscription id returned earlier on creation of the subscription
	 */
	void unsubscribeFromUInt8Attribute2(const std::string &subscriptionId) override {
		MultipleVersionsInterfaceProxyBase::unsubscribeFromUInt8Attribute2(subscriptionId);
	}

	/**
	 * @brief creates a new subscription to attribute UInt8Attribute2
	 * @param subscriptionListener The listener callback providing methods to call on publication and failure
	 * @param subscriptionQos The subscription quality of service settings
	 * @return a future representing the result (subscription id) as string. It provides methods to wait for
	 * completion, to get the subscription id or the request status object. The subscription id will be available
	 * when the subscription is successfully registered at the provider.
	 */
	std::shared_ptr<joynr::Future<std::string>> subscribeToUInt8Attribute2(
				std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
				std::shared_ptr<joynr::SubscriptionQos> subscriptionQos)
	 override {
		return MultipleVersionsInterfaceProxyBase::subscribeToUInt8Attribute2(
					subscriptionListener,
					subscriptionQos);
	}

	/**
	 * @brief updates an existing subscription to attribute UInt8Attribute2
	 * @param subscriptionListener The listener callback providing methods to call on publication and failure
	 * @param subscriptionQos The subscription quality of service settings
	 * @param subscriptionId The subscription id returned earlier on creation of the subscription
	 * @return a future representing the result (subscription id) as string. It provides methods to wait for
	 * completion, to get the subscription id or the request status object. The subscription id will be available
	 * when the subscription is successfully registered at the provider.
	 */
	std::shared_ptr<joynr::Future<std::string>> subscribeToUInt8Attribute2(
				std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
				std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
				const std::string& subscriptionId)
	 override{
		return MultipleVersionsInterfaceProxyBase::subscribeToUInt8Attribute2(
					subscriptionListener,
					subscriptionQos,
					subscriptionId);
	}


	/** @brief Destructor */
	~MultipleVersionsInterfaceProxy() override = default;

	// attributes
	using MultipleVersionsInterfaceAsyncProxy::getUInt8Attribute1Async;
	using MultipleVersionsInterfaceSyncProxy::getUInt8Attribute1;
	using MultipleVersionsInterfaceAsyncProxy::setUInt8Attribute1Async;
	using MultipleVersionsInterfaceSyncProxy::setUInt8Attribute1;
	using MultipleVersionsInterfaceAsyncProxy::getUInt8Attribute2Async;
	using MultipleVersionsInterfaceSyncProxy::getUInt8Attribute2;
	using MultipleVersionsInterfaceAsyncProxy::setUInt8Attribute2Async;
	using MultipleVersionsInterfaceSyncProxy::setUInt8Attribute2;

	using IMultipleVersionsInterfaceSync::getTrue;
	using IMultipleVersionsInterfaceAsync::getTrueAsync;
	using IMultipleVersionsInterfaceSync::getAnonymousVersionedStruct;
	using IMultipleVersionsInterfaceAsync::getAnonymousVersionedStructAsync;
	using IMultipleVersionsInterfaceSync::getInterfaceVersionedStruct;
	using IMultipleVersionsInterfaceAsync::getInterfaceVersionedStructAsync;
	using IMultipleVersionsInterfaceSync::getVersionedStruct;
	using IMultipleVersionsInterfaceAsync::getVersionedStructAsync;
private:
	DISALLOW_COPY_AND_ASSIGN(MultipleVersionsInterfaceProxy);
};


} // namespace tests
} // namespace joynr

#endif // GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACEPROXY_H
