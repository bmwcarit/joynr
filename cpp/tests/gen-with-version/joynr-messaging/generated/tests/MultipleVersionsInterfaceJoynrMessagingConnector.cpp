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

#include "joynr/tests/MultipleVersionsInterfaceJoynrMessagingConnector.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/ReplyCaller.h"
#include "joynr/IMessageSender.h"
#include "joynr/UnicastSubscriptionCallback.h"
#include "joynr/MulticastSubscriptionCallback.h"
#include "joynr/Util.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/Future.h"
#include <cstdint>
#include "joynr/SubscriptionUtil.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Request.h"
#include "joynr/OneWayRequest.h"
#include "joynr/SubscriptionRequest.h"


#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct.h"
#include "joynr/tests/AnonymousVersionedStruct.h"
#include "joynr/tests/InterfaceVersionedStruct.h"


namespace joynr { namespace tests { 
MultipleVersionsInterfaceJoynrMessagingConnector::MultipleVersionsInterfaceJoynrMessagingConnector(
		std::weak_ptr<joynr::IMessageSender> messageSender,
		std::weak_ptr<joynr::ISubscriptionManager> subscriptionManager,
		const std::string& domain,
		const std::string& proxyParticipantId,
		const joynr::MessagingQos &qosSettings,
		const joynr::types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry)
	: joynr::AbstractJoynrMessagingConnector(messageSender, subscriptionManager, domain, INTERFACE_NAME(), proxyParticipantId, qosSettings, providerDiscoveryEntry)
{
}

void MultipleVersionsInterfaceJoynrMessagingConnector::getUInt8Attribute1(std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos)
{
	auto future = getUInt8Attribute1Async(nullptr, nullptr, std::move(qos));
	future->get(uInt8Attribute1);
}

std::shared_ptr<joynr::Future<std::uint8_t> > MultipleVersionsInterfaceJoynrMessagingConnector::getUInt8Attribute1Async(
			std::function<void(const std::uint8_t& uInt8Attribute1)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError,
			boost::optional<joynr::MessagingQos> qos)
			noexcept
{
	joynr::Request request;
	// explicitly set to no parameters
	request.setParams();
	request.setMethodName("getUInt8Attribute1");
	auto future = std::make_shared<joynr::Future<std::uint8_t>>();

	std::function<void(const std::uint8_t&)> onSuccessWrapper = [
			future,
			onSuccess = std::move(onSuccess),
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const std::uint8_t& uInt8Attribute1) {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST returns successful: requestReplyId: {}, method: {}, response: {}",
				requestReplyId,
				methodName,
				joynr::serializer::serializeToJson(uInt8Attribute1)
		);
		future->onSuccess(uInt8Attribute1);
		if (onSuccess){
			onSuccess(uInt8Attribute1);
		}
	};

	std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> onErrorWrapper = [
			future,
			onError = onError,
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const std::shared_ptr<exceptions::JoynrException>& error) {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST returns error: requestReplyId: {}, method: {}, response: {}",
				requestReplyId,
				methodName,
				error->what()
		);
		future->onError(error);
		if (onError){
			onError(static_cast<const exceptions::JoynrRuntimeException&>(*error));
		}
	};

	try {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST call proxy: requestReplyId: {}, method: {}, proxy "
				"participantId: {}, provider participantId: [{}]",
				request.getRequestReplyId(),
				request.getMethodName(),
				proxyParticipantId,
				providerParticipantId);
		auto replyCaller = std::make_shared<joynr::ReplyCaller<std::uint8_t>>(std::move(onSuccessWrapper), std::move(onErrorWrapper));
		operationRequest(std::move(replyCaller), std::move(request), std::move(qos));
	} catch (const std::invalid_argument& exception) {
		auto joynrException = std::make_shared<joynr::exceptions::MethodInvocationException>(exception.what());
		future->onError(joynrException);
		if (onError){
			onError(*joynrException);
		}
	} catch (const joynr::exceptions::JoynrRuntimeException& exception) {
		std::shared_ptr<joynr::exceptions::JoynrRuntimeException> exceptionPtr;
		exceptionPtr.reset(exception.clone());
		future->onError(exceptionPtr);
		if (onError){
			onError(exception);
		}
	}
	return future;
}

std::shared_ptr<joynr::Future<void> > MultipleVersionsInterfaceJoynrMessagingConnector::setUInt8Attribute1Async(
			std::uint8_t uInt8Attribute1,
			std::function<void(void)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError,
			boost::optional<joynr::MessagingQos> qos)
			noexcept
{
	joynr::Request request;
	request.setMethodName("setUInt8Attribute1");
	request.setParamDatatypes({"Byte"});
	request.setParams(uInt8Attribute1);

	auto future = std::make_shared<joynr::Future<void>>();

	std::function<void()> onSuccessWrapper = [
			future,
			onSuccess = std::move(onSuccess),
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] () {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST returns successful: requestReplyId: {}, method: {}",
				requestReplyId,
				methodName
		);
		future->onSuccess();
		if (onSuccess) {
			onSuccess();
		}
	};

	std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> onErrorWrapper = [
			future,
			onError = onError,
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const std::shared_ptr<exceptions::JoynrException>& error) {
		JOYNR_LOG_DEBUG(logger(),
			"REQUEST returns error: requestReplyId: {}, method: {}, response: {}",
			requestReplyId,
			methodName,
			error->what()
		);
		future->onError(error);
		if (onError) {
			onError(static_cast<const exceptions::JoynrRuntimeException&>(*error));
		}
	};

	try {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST call proxy: requestReplyId: {}, method: {}, params: {}, proxy "
				"participantId: {}, provider participantId: [{}]",
				request.getRequestReplyId(),
				request.getMethodName(),
				joynr::serializer::serializeToJson(uInt8Attribute1),
				proxyParticipantId,
				providerParticipantId);
		auto replyCaller = std::make_shared<joynr::ReplyCaller<void>>(std::move(onSuccessWrapper), std::move(onErrorWrapper));
		operationRequest(std::move(replyCaller), std::move(request), std::move(qos));
	} catch (const std::invalid_argument& exception) {
		auto joynrException = std::make_shared<joynr::exceptions::MethodInvocationException>(exception.what());
		future->onError(joynrException);
		if (onError){
			onError(*joynrException);
		}
	} catch (const joynr::exceptions::JoynrRuntimeException& exception) {
		std::shared_ptr<joynr::exceptions::JoynrRuntimeException> exceptionPtr;
		exceptionPtr.reset(exception.clone());
		future->onError(exceptionPtr);
		if (onError){
			onError(exception);
		}
	}
	return future;
}

void MultipleVersionsInterfaceJoynrMessagingConnector::setUInt8Attribute1(const std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos)
{
	auto future = setUInt8Attribute1Async(uInt8Attribute1, nullptr, nullptr, std::move(qos));
	future->get();
}

std::shared_ptr<joynr::Future<std::string>> MultipleVersionsInterfaceJoynrMessagingConnector::subscribeToUInt8Attribute1(
			std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
			std::shared_ptr<joynr::SubscriptionQos> subscriptionQos)
 {
	joynr::SubscriptionRequest subscriptionRequest;
	return subscribeToUInt8Attribute1(subscriptionListener, subscriptionQos, subscriptionRequest);
}

std::shared_ptr<joynr::Future<std::string>> MultipleVersionsInterfaceJoynrMessagingConnector::subscribeToUInt8Attribute1(
			std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
			std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
			const std::string& subscriptionId)
 {

	joynr::SubscriptionRequest subscriptionRequest;
	subscriptionRequest.setSubscriptionId(subscriptionId);
	return subscribeToUInt8Attribute1(subscriptionListener, subscriptionQos, subscriptionRequest);
}

std::shared_ptr<joynr::Future<std::string>> MultipleVersionsInterfaceJoynrMessagingConnector::subscribeToUInt8Attribute1(
			std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
			std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
			SubscriptionRequest& subscriptionRequest
) {
	JOYNR_LOG_TRACE(logger(), "Subscribing to uInt8Attribute1.");
	std::string attributeName("uInt8Attribute1");
	joynr::MessagingQos clonedMessagingQos(qosSettings);
	clonedMessagingQos.setTtl(ISubscriptionManager::convertExpiryDateIntoTtlMs(*subscriptionQos));

	auto future = std::make_shared<Future<std::string>>();
	auto subscriptionCallback = std::make_shared<joynr::UnicastSubscriptionCallback<std::uint8_t>
	>(subscriptionRequest.getSubscriptionId(), future, subscriptionManager);
	if (auto ptr = subscriptionManager.lock()) {
		ptr->registerSubscription(
				attributeName,
				subscriptionCallback,
				subscriptionListener,
				subscriptionQos,
				subscriptionRequest);
	}
	JOYNR_LOG_DEBUG(logger(),
			"SUBSCRIPTION call proxy: subscriptionId: {}, attribute: {}, qos: {}, proxy "
			"participantId: {}, provider participantId: [{}]",
			subscriptionRequest.getSubscriptionId(),
			attributeName,
			joynr::serializer::serializeToJson(*subscriptionQos),
			proxyParticipantId,
			providerParticipantId);

	if (auto ptr = messageSender.lock()) {
		ptr->sendSubscriptionRequest(
				proxyParticipantId,
				providerParticipantId,
				clonedMessagingQos,
				subscriptionRequest,
				providerDiscoveryEntry.getIsLocal()
				);
	}
	return future;
}

void MultipleVersionsInterfaceJoynrMessagingConnector::unsubscribeFromUInt8Attribute1(const std::string& subscriptionId)
 {
	joynr::SubscriptionStop subscriptionStop;
	subscriptionStop.setSubscriptionId(subscriptionId);

	if (auto ptr = subscriptionManager.lock()) {
		ptr->unregisterSubscription(subscriptionId);
	}
	if (auto ptr = messageSender.lock()) {
		ptr->sendSubscriptionStop(
				proxyParticipantId,
				providerParticipantId,
				qosSettings,
				subscriptionStop
				);
	}
}

void MultipleVersionsInterfaceJoynrMessagingConnector::getUInt8Attribute2(std::uint8_t& uInt8Attribute2, boost::optional<joynr::MessagingQos> qos)
{
	auto future = getUInt8Attribute2Async(nullptr, nullptr, std::move(qos));
	future->get(uInt8Attribute2);
}

std::shared_ptr<joynr::Future<std::uint8_t> > MultipleVersionsInterfaceJoynrMessagingConnector::getUInt8Attribute2Async(
			std::function<void(const std::uint8_t& uInt8Attribute2)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError,
			boost::optional<joynr::MessagingQos> qos)
			noexcept
{
	joynr::Request request;
	// explicitly set to no parameters
	request.setParams();
	request.setMethodName("getUInt8Attribute2");
	auto future = std::make_shared<joynr::Future<std::uint8_t>>();

	std::function<void(const std::uint8_t&)> onSuccessWrapper = [
			future,
			onSuccess = std::move(onSuccess),
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const std::uint8_t& uInt8Attribute2) {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST returns successful: requestReplyId: {}, method: {}, response: {}",
				requestReplyId,
				methodName,
				joynr::serializer::serializeToJson(uInt8Attribute2)
		);
		future->onSuccess(uInt8Attribute2);
		if (onSuccess){
			onSuccess(uInt8Attribute2);
		}
	};

	std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> onErrorWrapper = [
			future,
			onError = onError,
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const std::shared_ptr<exceptions::JoynrException>& error) {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST returns error: requestReplyId: {}, method: {}, response: {}",
				requestReplyId,
				methodName,
				error->what()
		);
		future->onError(error);
		if (onError){
			onError(static_cast<const exceptions::JoynrRuntimeException&>(*error));
		}
	};

	try {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST call proxy: requestReplyId: {}, method: {}, proxy "
				"participantId: {}, provider participantId: [{}]",
				request.getRequestReplyId(),
				request.getMethodName(),
				proxyParticipantId,
				providerParticipantId);
		auto replyCaller = std::make_shared<joynr::ReplyCaller<std::uint8_t>>(std::move(onSuccessWrapper), std::move(onErrorWrapper));
		operationRequest(std::move(replyCaller), std::move(request), std::move(qos));
	} catch (const std::invalid_argument& exception) {
		auto joynrException = std::make_shared<joynr::exceptions::MethodInvocationException>(exception.what());
		future->onError(joynrException);
		if (onError){
			onError(*joynrException);
		}
	} catch (const joynr::exceptions::JoynrRuntimeException& exception) {
		std::shared_ptr<joynr::exceptions::JoynrRuntimeException> exceptionPtr;
		exceptionPtr.reset(exception.clone());
		future->onError(exceptionPtr);
		if (onError){
			onError(exception);
		}
	}
	return future;
}

std::shared_ptr<joynr::Future<void> > MultipleVersionsInterfaceJoynrMessagingConnector::setUInt8Attribute2Async(
			std::uint8_t uInt8Attribute2,
			std::function<void(void)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError,
			boost::optional<joynr::MessagingQos> qos)
			noexcept
{
	joynr::Request request;
	request.setMethodName("setUInt8Attribute2");
	request.setParamDatatypes({"Byte"});
	request.setParams(uInt8Attribute2);

	auto future = std::make_shared<joynr::Future<void>>();

	std::function<void()> onSuccessWrapper = [
			future,
			onSuccess = std::move(onSuccess),
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] () {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST returns successful: requestReplyId: {}, method: {}",
				requestReplyId,
				methodName
		);
		future->onSuccess();
		if (onSuccess) {
			onSuccess();
		}
	};

	std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> onErrorWrapper = [
			future,
			onError = onError,
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const std::shared_ptr<exceptions::JoynrException>& error) {
		JOYNR_LOG_DEBUG(logger(),
			"REQUEST returns error: requestReplyId: {}, method: {}, response: {}",
			requestReplyId,
			methodName,
			error->what()
		);
		future->onError(error);
		if (onError) {
			onError(static_cast<const exceptions::JoynrRuntimeException&>(*error));
		}
	};

	try {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST call proxy: requestReplyId: {}, method: {}, params: {}, proxy "
				"participantId: {}, provider participantId: [{}]",
				request.getRequestReplyId(),
				request.getMethodName(),
				joynr::serializer::serializeToJson(uInt8Attribute2),
				proxyParticipantId,
				providerParticipantId);
		auto replyCaller = std::make_shared<joynr::ReplyCaller<void>>(std::move(onSuccessWrapper), std::move(onErrorWrapper));
		operationRequest(std::move(replyCaller), std::move(request), std::move(qos));
	} catch (const std::invalid_argument& exception) {
		auto joynrException = std::make_shared<joynr::exceptions::MethodInvocationException>(exception.what());
		future->onError(joynrException);
		if (onError){
			onError(*joynrException);
		}
	} catch (const joynr::exceptions::JoynrRuntimeException& exception) {
		std::shared_ptr<joynr::exceptions::JoynrRuntimeException> exceptionPtr;
		exceptionPtr.reset(exception.clone());
		future->onError(exceptionPtr);
		if (onError){
			onError(exception);
		}
	}
	return future;
}

void MultipleVersionsInterfaceJoynrMessagingConnector::setUInt8Attribute2(const std::uint8_t& uInt8Attribute2, boost::optional<joynr::MessagingQos> qos)
{
	auto future = setUInt8Attribute2Async(uInt8Attribute2, nullptr, nullptr, std::move(qos));
	future->get();
}

std::shared_ptr<joynr::Future<std::string>> MultipleVersionsInterfaceJoynrMessagingConnector::subscribeToUInt8Attribute2(
			std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
			std::shared_ptr<joynr::SubscriptionQos> subscriptionQos)
 {
	joynr::SubscriptionRequest subscriptionRequest;
	return subscribeToUInt8Attribute2(subscriptionListener, subscriptionQos, subscriptionRequest);
}

std::shared_ptr<joynr::Future<std::string>> MultipleVersionsInterfaceJoynrMessagingConnector::subscribeToUInt8Attribute2(
			std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
			std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
			const std::string& subscriptionId)
 {

	joynr::SubscriptionRequest subscriptionRequest;
	subscriptionRequest.setSubscriptionId(subscriptionId);
	return subscribeToUInt8Attribute2(subscriptionListener, subscriptionQos, subscriptionRequest);
}

std::shared_ptr<joynr::Future<std::string>> MultipleVersionsInterfaceJoynrMessagingConnector::subscribeToUInt8Attribute2(
			std::shared_ptr<joynr::ISubscriptionListener<std::uint8_t> > subscriptionListener,
			std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
			SubscriptionRequest& subscriptionRequest
) {
	JOYNR_LOG_TRACE(logger(), "Subscribing to uInt8Attribute2.");
	std::string attributeName("uInt8Attribute2");
	joynr::MessagingQos clonedMessagingQos(qosSettings);
	clonedMessagingQos.setTtl(ISubscriptionManager::convertExpiryDateIntoTtlMs(*subscriptionQos));

	auto future = std::make_shared<Future<std::string>>();
	auto subscriptionCallback = std::make_shared<joynr::UnicastSubscriptionCallback<std::uint8_t>
	>(subscriptionRequest.getSubscriptionId(), future, subscriptionManager);
	if (auto ptr = subscriptionManager.lock()) {
		ptr->registerSubscription(
				attributeName,
				subscriptionCallback,
				subscriptionListener,
				subscriptionQos,
				subscriptionRequest);
	}
	JOYNR_LOG_DEBUG(logger(),
			"SUBSCRIPTION call proxy: subscriptionId: {}, attribute: {}, qos: {}, proxy "
			"participantId: {}, provider participantId: [{}]",
			subscriptionRequest.getSubscriptionId(),
			attributeName,
			joynr::serializer::serializeToJson(*subscriptionQos),
			proxyParticipantId,
			providerParticipantId);

	if (auto ptr = messageSender.lock()) {
		ptr->sendSubscriptionRequest(
				proxyParticipantId,
				providerParticipantId,
				clonedMessagingQos,
				subscriptionRequest,
				providerDiscoveryEntry.getIsLocal()
				);
	}
	return future;
}

void MultipleVersionsInterfaceJoynrMessagingConnector::unsubscribeFromUInt8Attribute2(const std::string& subscriptionId)
 {
	joynr::SubscriptionStop subscriptionStop;
	subscriptionStop.setSubscriptionId(subscriptionId);

	if (auto ptr = subscriptionManager.lock()) {
		ptr->unregisterSubscription(subscriptionId);
	}
	if (auto ptr = messageSender.lock()) {
		ptr->sendSubscriptionStop(
				proxyParticipantId,
				providerParticipantId,
				qosSettings,
				subscriptionStop
				);
	}
}



void MultipleVersionsInterfaceJoynrMessagingConnector::getTrue(
			bool& result,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto future = getTrueAsync(nullptr, nullptr, std::move(qos));
	future->get(result);
}

std::shared_ptr<joynr::Future<bool>> MultipleVersionsInterfaceJoynrMessagingConnector:: getTrueAsync(
			std::function<void(const bool& result)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
			boost::optional<joynr::MessagingQos> qos
) noexcept
{
	joynr::Request request;
	
	request.setMethodName("getTrue");
	request.setParamDatatypes({
		});
	request.setParams(
	);

	auto future = std::make_shared<joynr::Future<bool>>();

	std::function<void(const bool& result)> onSuccessWrapper = [
			future,
			onSuccess = std::move(onSuccess),
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const bool& result)
	{
		JOYNR_LOG_DEBUG(logger(),
			"REQUEST returns successful: requestReplyId: {}, method: {}, response: {}",
			requestReplyId,
			methodName,
			joynr::serializer::serializeToJson(result)
		);
		future->onSuccess(result);
		if (onSuccess) {
			onSuccess(result);
		}
	};

	std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> onErrorWrapper = [
			future,
			onRuntimeError = onRuntimeError,
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const std::shared_ptr<exceptions::JoynrException>& error)
	{
		JOYNR_LOG_DEBUG(logger(),
			"REQUEST returns error: requestReplyId: {}, method: {}, response: {}",
			requestReplyId,
			methodName,
			error->what()
		);
		future->onError(error);
			if (onRuntimeError) {
				onRuntimeError(static_cast<const exceptions::JoynrRuntimeException&>(*error));
			}
		};

	try {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST call proxy: requestReplyId: {}, method: {}, params: , "
				"proxy participantId: {}, provider participantId: [{}]",
				request.getRequestReplyId(),
				request.getMethodName(),
				proxyParticipantId,
				providerParticipantId);

		auto replyCaller = std::make_shared<joynr::ReplyCaller<bool>>(std::move(onSuccessWrapper), std::move(onErrorWrapper));
		operationRequest(std::move(replyCaller), std::move(request), std::move(qos));
	} catch (const std::invalid_argument& exception) {
		auto joynrException = std::make_shared<joynr::exceptions::MethodInvocationException>(exception.what());
		future->onError(joynrException);
		if (onRuntimeError){
			onRuntimeError(*joynrException);
		}
	} catch (const joynr::exceptions::JoynrRuntimeException& exception) {
		std::shared_ptr<joynr::exceptions::JoynrRuntimeException> exceptionPtr;
		exceptionPtr.reset(exception.clone());
		future->onError(exceptionPtr);
		if (onRuntimeError){
			onRuntimeError(exception);
		}
	}
	return future;
}

void MultipleVersionsInterfaceJoynrMessagingConnector::getVersionedStruct(
			joynr::tests::MultipleVersionsTypeCollection::VersionedStruct& result,
			const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct& input,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto future = getVersionedStructAsync(input, nullptr, nullptr, std::move(qos));
	future->get(result);
}

std::shared_ptr<joynr::Future<joynr::tests::MultipleVersionsTypeCollection::VersionedStruct>> MultipleVersionsInterfaceJoynrMessagingConnector:: getVersionedStructAsync(
			const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct& input,
			std::function<void(const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct& result)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
			boost::optional<joynr::MessagingQos> qos
) noexcept
{
	joynr::Request request;
	
	request.setMethodName("getVersionedStruct");
	request.setParamDatatypes({
		"joynr.tests.MultipleVersionsTypeCollection.VersionedStruct"
		});
	request.setParams(
		input
	);

	auto future = std::make_shared<joynr::Future<joynr::tests::MultipleVersionsTypeCollection::VersionedStruct>>();

	std::function<void(const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct& result)> onSuccessWrapper = [
			future,
			onSuccess = std::move(onSuccess),
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct& result)
	{
		JOYNR_LOG_DEBUG(logger(),
			"REQUEST returns successful: requestReplyId: {}, method: {}, response: {}",
			requestReplyId,
			methodName,
			joynr::serializer::serializeToJson(result)
		);
		future->onSuccess(result);
		if (onSuccess) {
			onSuccess(result);
		}
	};

	std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> onErrorWrapper = [
			future,
			onRuntimeError = onRuntimeError,
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const std::shared_ptr<exceptions::JoynrException>& error)
	{
		JOYNR_LOG_DEBUG(logger(),
			"REQUEST returns error: requestReplyId: {}, method: {}, response: {}",
			requestReplyId,
			methodName,
			error->what()
		);
		future->onError(error);
			if (onRuntimeError) {
				onRuntimeError(static_cast<const exceptions::JoynrRuntimeException&>(*error));
			}
		};

	try {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST call proxy: requestReplyId: {}, method: {}, params: {}, "
				"proxy participantId: {}, provider participantId: [{}]",
				request.getRequestReplyId(),
				request.getMethodName(),
				joynr::serializer::serializeToJson(input),
				proxyParticipantId,
				providerParticipantId);

		auto replyCaller = std::make_shared<joynr::ReplyCaller<joynr::tests::MultipleVersionsTypeCollection::VersionedStruct>>(std::move(onSuccessWrapper), std::move(onErrorWrapper));
		operationRequest(std::move(replyCaller), std::move(request), std::move(qos));
	} catch (const std::invalid_argument& exception) {
		auto joynrException = std::make_shared<joynr::exceptions::MethodInvocationException>(exception.what());
		future->onError(joynrException);
		if (onRuntimeError){
			onRuntimeError(*joynrException);
		}
	} catch (const joynr::exceptions::JoynrRuntimeException& exception) {
		std::shared_ptr<joynr::exceptions::JoynrRuntimeException> exceptionPtr;
		exceptionPtr.reset(exception.clone());
		future->onError(exceptionPtr);
		if (onRuntimeError){
			onRuntimeError(exception);
		}
	}
	return future;
}

void MultipleVersionsInterfaceJoynrMessagingConnector::getAnonymousVersionedStruct(
			joynr::tests::AnonymousVersionedStruct& result,
			const joynr::tests::AnonymousVersionedStruct& input,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto future = getAnonymousVersionedStructAsync(input, nullptr, nullptr, std::move(qos));
	future->get(result);
}

std::shared_ptr<joynr::Future<joynr::tests::AnonymousVersionedStruct>> MultipleVersionsInterfaceJoynrMessagingConnector:: getAnonymousVersionedStructAsync(
			const joynr::tests::AnonymousVersionedStruct& input,
			std::function<void(const joynr::tests::AnonymousVersionedStruct& result)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
			boost::optional<joynr::MessagingQos> qos
) noexcept
{
	joynr::Request request;
	
	request.setMethodName("getAnonymousVersionedStruct");
	request.setParamDatatypes({
		"joynr.tests.AnonymousVersionedStruct"
		});
	request.setParams(
		input
	);

	auto future = std::make_shared<joynr::Future<joynr::tests::AnonymousVersionedStruct>>();

	std::function<void(const joynr::tests::AnonymousVersionedStruct& result)> onSuccessWrapper = [
			future,
			onSuccess = std::move(onSuccess),
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const joynr::tests::AnonymousVersionedStruct& result)
	{
		JOYNR_LOG_DEBUG(logger(),
			"REQUEST returns successful: requestReplyId: {}, method: {}, response: {}",
			requestReplyId,
			methodName,
			joynr::serializer::serializeToJson(result)
		);
		future->onSuccess(result);
		if (onSuccess) {
			onSuccess(result);
		}
	};

	std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> onErrorWrapper = [
			future,
			onRuntimeError = onRuntimeError,
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const std::shared_ptr<exceptions::JoynrException>& error)
	{
		JOYNR_LOG_DEBUG(logger(),
			"REQUEST returns error: requestReplyId: {}, method: {}, response: {}",
			requestReplyId,
			methodName,
			error->what()
		);
		future->onError(error);
			if (onRuntimeError) {
				onRuntimeError(static_cast<const exceptions::JoynrRuntimeException&>(*error));
			}
		};

	try {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST call proxy: requestReplyId: {}, method: {}, params: {}, "
				"proxy participantId: {}, provider participantId: [{}]",
				request.getRequestReplyId(),
				request.getMethodName(),
				joynr::serializer::serializeToJson(input),
				proxyParticipantId,
				providerParticipantId);

		auto replyCaller = std::make_shared<joynr::ReplyCaller<joynr::tests::AnonymousVersionedStruct>>(std::move(onSuccessWrapper), std::move(onErrorWrapper));
		operationRequest(std::move(replyCaller), std::move(request), std::move(qos));
	} catch (const std::invalid_argument& exception) {
		auto joynrException = std::make_shared<joynr::exceptions::MethodInvocationException>(exception.what());
		future->onError(joynrException);
		if (onRuntimeError){
			onRuntimeError(*joynrException);
		}
	} catch (const joynr::exceptions::JoynrRuntimeException& exception) {
		std::shared_ptr<joynr::exceptions::JoynrRuntimeException> exceptionPtr;
		exceptionPtr.reset(exception.clone());
		future->onError(exceptionPtr);
		if (onRuntimeError){
			onRuntimeError(exception);
		}
	}
	return future;
}

void MultipleVersionsInterfaceJoynrMessagingConnector::getInterfaceVersionedStruct(
			joynr::tests::InterfaceVersionedStruct& result,
			const joynr::tests::InterfaceVersionedStruct& input,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto future = getInterfaceVersionedStructAsync(input, nullptr, nullptr, std::move(qos));
	future->get(result);
}

std::shared_ptr<joynr::Future<joynr::tests::InterfaceVersionedStruct>> MultipleVersionsInterfaceJoynrMessagingConnector:: getInterfaceVersionedStructAsync(
			const joynr::tests::InterfaceVersionedStruct& input,
			std::function<void(const joynr::tests::InterfaceVersionedStruct& result)> onSuccess,
			std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
			boost::optional<joynr::MessagingQos> qos
) noexcept
{
	joynr::Request request;
	
	request.setMethodName("getInterfaceVersionedStruct");
	request.setParamDatatypes({
		"joynr.tests.InterfaceVersionedStruct"
		});
	request.setParams(
		input
	);

	auto future = std::make_shared<joynr::Future<joynr::tests::InterfaceVersionedStruct>>();

	std::function<void(const joynr::tests::InterfaceVersionedStruct& result)> onSuccessWrapper = [
			future,
			onSuccess = std::move(onSuccess),
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const joynr::tests::InterfaceVersionedStruct& result)
	{
		JOYNR_LOG_DEBUG(logger(),
			"REQUEST returns successful: requestReplyId: {}, method: {}, response: {}",
			requestReplyId,
			methodName,
			joynr::serializer::serializeToJson(result)
		);
		future->onSuccess(result);
		if (onSuccess) {
			onSuccess(result);
		}
	};

	std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> onErrorWrapper = [
			future,
			onRuntimeError = onRuntimeError,
			requestReplyId = request.getRequestReplyId(),
			methodName = request.getMethodName()
	] (const std::shared_ptr<exceptions::JoynrException>& error)
	{
		JOYNR_LOG_DEBUG(logger(),
			"REQUEST returns error: requestReplyId: {}, method: {}, response: {}",
			requestReplyId,
			methodName,
			error->what()
		);
		future->onError(error);
			if (onRuntimeError) {
				onRuntimeError(static_cast<const exceptions::JoynrRuntimeException&>(*error));
			}
		};

	try {
		JOYNR_LOG_DEBUG(logger(),
				"REQUEST call proxy: requestReplyId: {}, method: {}, params: {}, "
				"proxy participantId: {}, provider participantId: [{}]",
				request.getRequestReplyId(),
				request.getMethodName(),
				joynr::serializer::serializeToJson(input),
				proxyParticipantId,
				providerParticipantId);

		auto replyCaller = std::make_shared<joynr::ReplyCaller<joynr::tests::InterfaceVersionedStruct>>(std::move(onSuccessWrapper), std::move(onErrorWrapper));
		operationRequest(std::move(replyCaller), std::move(request), std::move(qos));
	} catch (const std::invalid_argument& exception) {
		auto joynrException = std::make_shared<joynr::exceptions::MethodInvocationException>(exception.what());
		future->onError(joynrException);
		if (onRuntimeError){
			onRuntimeError(*joynrException);
		}
	} catch (const joynr::exceptions::JoynrRuntimeException& exception) {
		std::shared_ptr<joynr::exceptions::JoynrRuntimeException> exceptionPtr;
		exceptionPtr.reset(exception.clone());
		future->onError(exceptionPtr);
		if (onRuntimeError){
			onRuntimeError(exception);
		}
	}
	return future;
}


} // namespace tests
} // namespace joynr
