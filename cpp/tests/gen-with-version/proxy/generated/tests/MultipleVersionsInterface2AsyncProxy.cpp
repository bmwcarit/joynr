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

#include "joynr/tests/MultipleVersionsInterface2AsyncProxy.h"

#include "joynr/tests/InterfaceVersionedStruct2.h"
#include <cstdint>
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct2.h"
#include <string>
#include "joynr/tests/AnonymousVersionedStruct2.h"

#include "joynr/Future.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr { namespace tests { 
MultipleVersionsInterface2AsyncProxy::MultipleVersionsInterface2AsyncProxy(
		std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
		std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
		const std::string &domain,
		const joynr::MessagingQos &qosSettings
) :
	joynr::ProxyBase(runtime, connectorFactory, domain, qosSettings),
	MultipleVersionsInterface2ProxyBase(runtime, connectorFactory, domain, qosSettings)
{
}

/*
 * getUInt8Attribute1
 */

std::shared_ptr<joynr::Future<std::uint8_t> > MultipleVersionsInterface2AsyncProxy::getUInt8Attribute1Async(
			std::function<void(const std::uint8_t& uInt8Attribute1)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError,
			boost::optional<joynr::MessagingQos> qos)
			noexcept
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorText;
		if (!runtimeSharedPtr) {
			errorText = "proxy cannot invoke getUInt8Attribute1 because the required runtime has been already destroyed.";
		}
		else {
			errorText = "proxy cannot invoke getUInt8Attribute1, because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorText);
		auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorText);
		if (onError) {
			onError(*error);
		}
		auto future = std::make_shared<joynr::Future<std::uint8_t>>();
		future->onError(error);
		return future;
	}
	else{
		return connector->getUInt8Attribute1Async(std::move(onSuccess), std::move(onError), std::move(qos));
	}
}

/*
 * setUInt8Attribute1
 */
std::shared_ptr<joynr::Future<void> > MultipleVersionsInterface2AsyncProxy::setUInt8Attribute1Async(
			std::uint8_t uInt8Attribute1,
			std::function<void(void)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError,
			boost::optional<joynr::MessagingQos> qos)
			noexcept
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorText;
		if (!runtimeSharedPtr) {
			errorText = "proxy cannot invoke setUInt8Attribute1 because the required runtime has been already destroyed.";
		}
		else {
			errorText = "proxy cannot invoke setUInt8Attribute1, because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorText);
		auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorText);
		if (onError) {
			onError(*error);
		}
		auto future = std::make_shared<joynr::Future<void>>();
		future->onError(error);
		return future;
	}
	else {
		return connector->setUInt8Attribute1Async(uInt8Attribute1, std::move(onSuccess), std::move(onError), std::move(qos));
	}
}

/*
 * getUInt8Attribute2
 */

std::shared_ptr<joynr::Future<std::uint8_t> > MultipleVersionsInterface2AsyncProxy::getUInt8Attribute2Async(
			std::function<void(const std::uint8_t& uInt8Attribute2)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError,
			boost::optional<joynr::MessagingQos> qos)
			noexcept
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorText;
		if (!runtimeSharedPtr) {
			errorText = "proxy cannot invoke getUInt8Attribute2 because the required runtime has been already destroyed.";
		}
		else {
			errorText = "proxy cannot invoke getUInt8Attribute2, because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorText);
		auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorText);
		if (onError) {
			onError(*error);
		}
		auto future = std::make_shared<joynr::Future<std::uint8_t>>();
		future->onError(error);
		return future;
	}
	else{
		return connector->getUInt8Attribute2Async(std::move(onSuccess), std::move(onError), std::move(qos));
	}
}

/*
 * setUInt8Attribute2
 */
std::shared_ptr<joynr::Future<void> > MultipleVersionsInterface2AsyncProxy::setUInt8Attribute2Async(
			std::uint8_t uInt8Attribute2,
			std::function<void(void)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError,
			boost::optional<joynr::MessagingQos> qos)
			noexcept
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorText;
		if (!runtimeSharedPtr) {
			errorText = "proxy cannot invoke setUInt8Attribute2 because the required runtime has been already destroyed.";
		}
		else {
			errorText = "proxy cannot invoke setUInt8Attribute2, because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorText);
		auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorText);
		if (onError) {
			onError(*error);
		}
		auto future = std::make_shared<joynr::Future<void>>();
		future->onError(error);
		return future;
	}
	else {
		return connector->setUInt8Attribute2Async(uInt8Attribute2, std::move(onSuccess), std::move(onError), std::move(qos));
	}
}

/*
 * getTrue
 */
std::shared_ptr<joynr::Future<bool>> MultipleVersionsInterface2AsyncProxy:: getTrueAsync(
			std::function<void(const bool& result)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
			boost::optional<joynr::MessagingQos> qos
) noexcept
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorText;
		if (!runtimeSharedPtr) {
			errorText = "proxy cannot invoke getTrue because the required runtime has been already destroyed.";
		}
		else {
			errorText = "proxy cannot invoke getTrue, because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorText);
		auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorText);
		if (onRuntimeError) {
			onRuntimeError(*error);
		}
		auto future = std::make_shared<joynr::Future<bool>>();
		future->onError(error);
		return future;
	}
	else{
		return connector->getTrueAsync(std::move(onSuccess), std::move(onRuntimeError), std::move(qos));
	}
}
/*
 * getVersionedStruct
 */
std::shared_ptr<joynr::Future<joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2>> MultipleVersionsInterface2AsyncProxy:: getVersionedStructAsync(
			const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& input,
			std::function<void(const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& result)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
			boost::optional<joynr::MessagingQos> qos
) noexcept
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorText;
		if (!runtimeSharedPtr) {
			errorText = "proxy cannot invoke getVersionedStruct because the required runtime has been already destroyed.";
		}
		else {
			errorText = "proxy cannot invoke getVersionedStruct, because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorText);
		auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorText);
		if (onRuntimeError) {
			onRuntimeError(*error);
		}
		auto future = std::make_shared<joynr::Future<joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2>>();
		future->onError(error);
		return future;
	}
	else{
		return connector->getVersionedStructAsync(input, std::move(onSuccess), std::move(onRuntimeError), std::move(qos));
	}
}
/*
 * getAnonymousVersionedStruct
 */
std::shared_ptr<joynr::Future<joynr::tests::AnonymousVersionedStruct2>> MultipleVersionsInterface2AsyncProxy:: getAnonymousVersionedStructAsync(
			const joynr::tests::AnonymousVersionedStruct2& input,
			std::function<void(const joynr::tests::AnonymousVersionedStruct2& result)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
			boost::optional<joynr::MessagingQos> qos
) noexcept
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorText;
		if (!runtimeSharedPtr) {
			errorText = "proxy cannot invoke getAnonymousVersionedStruct because the required runtime has been already destroyed.";
		}
		else {
			errorText = "proxy cannot invoke getAnonymousVersionedStruct, because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorText);
		auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorText);
		if (onRuntimeError) {
			onRuntimeError(*error);
		}
		auto future = std::make_shared<joynr::Future<joynr::tests::AnonymousVersionedStruct2>>();
		future->onError(error);
		return future;
	}
	else{
		return connector->getAnonymousVersionedStructAsync(input, std::move(onSuccess), std::move(onRuntimeError), std::move(qos));
	}
}
/*
 * getInterfaceVersionedStruct
 */
std::shared_ptr<joynr::Future<joynr::tests::InterfaceVersionedStruct2>> MultipleVersionsInterface2AsyncProxy:: getInterfaceVersionedStructAsync(
			const joynr::tests::InterfaceVersionedStruct2& input,
			std::function<void(const joynr::tests::InterfaceVersionedStruct2& result)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
			boost::optional<joynr::MessagingQos> qos
) noexcept
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorText;
		if (!runtimeSharedPtr) {
			errorText = "proxy cannot invoke getInterfaceVersionedStruct because the required runtime has been already destroyed.";
		}
		else {
			errorText = "proxy cannot invoke getInterfaceVersionedStruct, because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorText);
		auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorText);
		if (onRuntimeError) {
			onRuntimeError(*error);
		}
		auto future = std::make_shared<joynr::Future<joynr::tests::InterfaceVersionedStruct2>>();
		future->onError(error);
		return future;
	}
	else{
		return connector->getInterfaceVersionedStructAsync(input, std::move(onSuccess), std::move(onRuntimeError), std::move(qos));
	}
}

} // namespace tests
} // namespace joynr
