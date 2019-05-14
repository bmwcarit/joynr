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

#include "joynr/tests/MultipleVersionsInterface1SyncProxy.h"

#include "joynr/tests/InterfaceVersionedStruct1.h"
#include <cstdint>
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct1.h"
#include "joynr/tests/AnonymousVersionedStruct1.h"

namespace joynr { namespace tests { 
// The proxies will contain all arbitration checks
// the connectors will contain the JSON related code

MultipleVersionsInterface1SyncProxy::MultipleVersionsInterface1SyncProxy(
		std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
		std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
		const std::string &domain,
		const joynr::MessagingQos &qosSettings
) :
		joynr::ProxyBase(runtime, connectorFactory, domain, qosSettings),
		MultipleVersionsInterface1ProxyBase(runtime, connectorFactory, domain, qosSettings)
{
}

void MultipleVersionsInterface1SyncProxy::getUInt8Attribute1(std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke getUInt8Attribute1 because the required runtime has been already destroyed.";
		}
		else {
			errorMsg = "proxy cannot invoke getUInt8Attribute1 because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->getUInt8Attribute1(uInt8Attribute1, std::move(qos));
	}
}
void MultipleVersionsInterface1SyncProxy::setUInt8Attribute1(const std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke setUInt8Attribute1 because the required runtime has been already destroyed.";
		}
		else {
			errorMsg = "proxy cannot invoke setUInt8Attribute1 because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->setUInt8Attribute1(uInt8Attribute1, std::move(qos));
	}
}

/*
 * getTrue
 */
void MultipleVersionsInterface1SyncProxy::getTrue(
			bool& result,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke getTrue because the required runtime has been already destroyed.";
		}
		if (connector==nullptr){
			errorMsg = "proxy cannot invoke getTrue because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->getTrue(result,  std::move(qos));
	}
}
/*
 * getVersionedStruct
 */
void MultipleVersionsInterface1SyncProxy::getVersionedStruct(
			joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1& result,
			const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1& input,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke getVersionedStruct because the required runtime has been already destroyed.";
		}
		if (connector==nullptr){
			errorMsg = "proxy cannot invoke getVersionedStruct because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->getVersionedStruct(result, input, std::move(qos));
	}
}
/*
 * getAnonymousVersionedStruct
 */
void MultipleVersionsInterface1SyncProxy::getAnonymousVersionedStruct(
			joynr::tests::AnonymousVersionedStruct1& result,
			const joynr::tests::AnonymousVersionedStruct1& input,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke getAnonymousVersionedStruct because the required runtime has been already destroyed.";
		}
		if (connector==nullptr){
			errorMsg = "proxy cannot invoke getAnonymousVersionedStruct because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->getAnonymousVersionedStruct(result, input, std::move(qos));
	}
}
/*
 * getInterfaceVersionedStruct
 */
void MultipleVersionsInterface1SyncProxy::getInterfaceVersionedStruct(
			joynr::tests::InterfaceVersionedStruct1& result,
			const joynr::tests::InterfaceVersionedStruct1& input,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke getInterfaceVersionedStruct because the required runtime has been already destroyed.";
		}
		if (connector==nullptr){
			errorMsg = "proxy cannot invoke getInterfaceVersionedStruct because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->getInterfaceVersionedStruct(result, input, std::move(qos));
	}
}

} // namespace tests
} // namespace joynr
