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

#ifndef GENERATED_INTERFACE_JOYNR_TESTS_IMULTIPLEVERSIONSINTERFACECONNECTOR_H
#define GENERATED_INTERFACE_JOYNR_TESTS_IMULTIPLEVERSIONSINTERFACECONNECTOR_H

#include "joynr/tests/AnonymousVersionedStruct.h"
#include <cstdint>
#include "joynr/tests/InterfaceVersionedStruct.h"
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct.h"

#include "joynr/tests/IMultipleVersionsInterface.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/SubscriptionCallback.h"
#include <memory>

namespace joynr {
	template <class ... Ts> class ISubscriptionListener;
	class ISubscriptionCallback;
	class SubscriptionQos;
	class OnChangeSubscriptionQos;
	class MulticastSubscriptionQos;
} // namespace joynr

namespace joynr { namespace tests { 


class  IMultipleVersionsInterfaceSubscription{
	/**
	  * in  - subscriptionListener      std::shared_ptr to a SubscriptionListener which will receive the updates.
	  * in  - subscriptionQos           SubscriptionQos parameters like interval and end date.
	  * out - assignedSubscriptionId    Buffer for the assigned subscriptionId.
	  */
public:
	virtual ~IMultipleVersionsInterfaceSubscription() = default;

	/**
	 * @brief creates a new subscription to attribute UInt8Attribute1
	 * @param subscriptionListener The listener callback providing methods to call on publication and failure
	 * @param subscriptionQos The subscription quality of service settings
	 * @return a future representing the result (subscription id) as string. It provides methods to wait for
	 * completion, to get the subscription id or the request status object. The subscription id will be available
	 * when the subscription is successfully registered at the provider.
	 */
	virtual std::shared_ptr<joynr::Future<std::string>> subscribeToUInt8Attribute1(
				std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
				std::shared_ptr<joynr::SubscriptionQos> subscriptionQos)
	 = 0;
	
	/**
	 * @brief updates an existing subscription to attribute UInt8Attribute1
	 * @param subscriptionListener The listener callback providing methods to call on publication and failure
	 * @param subscriptionQos The subscription quality of service settings
	 * @param subscriptionId The subscription id returned earlier on creation of the subscription
	 * @return a future representing the result (subscription id) as string. It provides methods to wait for
	 * completion, to get the subscription id or the request status object. The subscription id will be available
	 * when the subscription is successfully registered at the provider.
	 */
	virtual std::shared_ptr<joynr::Future<std::string>> subscribeToUInt8Attribute1(
				std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
				std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
				const std::string& subscriptionId)
	 = 0;
	
	/**
	 * @brief unsubscribes from attribute UInt8Attribute1
	 * @param subscriptionId The subscription id returned earlier on creation of the subscription
	 */
	virtual void unsubscribeFromUInt8Attribute1(const std::string& subscriptionId)
	 = 0;
	
	/**
	 * @brief creates a new subscription to attribute UInt8Attribute2
	 * @param subscriptionListener The listener callback providing methods to call on publication and failure
	 * @param subscriptionQos The subscription quality of service settings
	 * @return a future representing the result (subscription id) as string. It provides methods to wait for
	 * completion, to get the subscription id or the request status object. The subscription id will be available
	 * when the subscription is successfully registered at the provider.
	 */
	virtual std::shared_ptr<joynr::Future<std::string>> subscribeToUInt8Attribute2(
				std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
				std::shared_ptr<joynr::SubscriptionQos> subscriptionQos)
	 = 0;
	
	/**
	 * @brief updates an existing subscription to attribute UInt8Attribute2
	 * @param subscriptionListener The listener callback providing methods to call on publication and failure
	 * @param subscriptionQos The subscription quality of service settings
	 * @param subscriptionId The subscription id returned earlier on creation of the subscription
	 * @return a future representing the result (subscription id) as string. It provides methods to wait for
	 * completion, to get the subscription id or the request status object. The subscription id will be available
	 * when the subscription is successfully registered at the provider.
	 */
	virtual std::shared_ptr<joynr::Future<std::string>> subscribeToUInt8Attribute2(
				std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
				std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
				const std::string& subscriptionId)
	 = 0;
	
	/**
	 * @brief unsubscribes from attribute UInt8Attribute2
	 * @param subscriptionId The subscription id returned earlier on creation of the subscription
	 */
	virtual void unsubscribeFromUInt8Attribute2(const std::string& subscriptionId)
	 = 0;
	
};

class  IMultipleVersionsInterfaceConnector: virtual public IMultipleVersionsInterface, virtual public IMultipleVersionsInterfaceSubscription{

public:
	~IMultipleVersionsInterfaceConnector() override = default;
};


} // namespace tests
} // namespace joynr
#endif // GENERATED_INTERFACE_JOYNR_TESTS_IMULTIPLEVERSIONSINTERFACECONNECTOR_H
