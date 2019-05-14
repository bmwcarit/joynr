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
#include <functional>
#include <tuple>

#include "joynr/tests/MultipleVersionsInterface1RequestInterpreter.h"
#include "joynr/tests/MultipleVersionsInterface1RequestCaller.h"
#include "joynr/Util.h"
#include "joynr/Request.h"
#include "joynr/OneWayRequest.h"
#include "joynr/BaseReply.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/MethodInvocationException.h"

#include "joynr/tests/InterfaceVersionedStruct1.h"
#include <cstdint>
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct1.h"
#include "joynr/tests/AnonymousVersionedStruct1.h"

namespace joynr { namespace tests { 

void MultipleVersionsInterface1RequestInterpreter::execute(
		std::shared_ptr<joynr::RequestCaller> requestCaller,
		Request& request,
		std::function<void (BaseReply&& reply)>&& onSuccess,
		std::function<void (const std::shared_ptr<exceptions::JoynrException>& exception)>&& onError
) {
	// cast generic RequestCaller to MultipleVersionsInterface1Requestcaller
	std::shared_ptr<MultipleVersionsInterface1RequestCaller> multipleVersionsInterface1RequestCallerVar =
			std::dynamic_pointer_cast<MultipleVersionsInterface1RequestCaller>(requestCaller);

	const std::vector<std::string>& paramTypes = request.getParamDatatypes();
	const std::string& methodName = request.getMethodName();

	// execute operation
		if (methodName == "getUInt8Attribute1" && paramTypes.size() == 0){
			try {
				auto requestCallerOnSuccess =
						[onSuccess = std::move(onSuccess)](std::uint8_t uInt8Attribute1){
							BaseReply reply;
							reply.setResponse(std::move(uInt8Attribute1));
							onSuccess(std::move(reply));
						};
				multipleVersionsInterface1RequestCallerVar->getUInt8Attribute1(
																	std::move(requestCallerOnSuccess),
																	onError);
			} catch (const std::exception& exception) {
				const std::string errorMessage = "Unexpected exception occurred in attribute getter getUInt8Attribute1 (): " + std::string(exception.what());
				JOYNR_LOG_ERROR(logger(), errorMessage);
				onError(
					std::make_shared<exceptions::MethodInvocationException>(
						errorMessage,
						requestCaller->getProviderVersion()));
			}
			return;
		}
	if (methodName == "setUInt8Attribute1" && paramTypes.size() == 1){
		try {
			std::uint8_t typedInputUInt8Attribute1;
			request.getParams(typedInputUInt8Attribute1);
			auto requestCallerOnSuccess =
					[onSuccess = std::move(onSuccess)] () {
						BaseReply reply;
						reply.setResponse();
						onSuccess(std::move(reply));
					};
			multipleVersionsInterface1RequestCallerVar->setUInt8Attribute1(
																typedInputUInt8Attribute1,
																std::move(requestCallerOnSuccess),
																onError);
		} catch (const std::exception& exception) {
			const std::string errorMessage = "Unexpected exception occurred in attribute setter setUInt8Attribute1 (Byte): " + std::string(exception.what());
			JOYNR_LOG_ERROR(logger(), errorMessage);
			onError(
				std::make_shared<exceptions::MethodInvocationException>(
					errorMessage,
					requestCaller->getProviderVersion()));
		}
		return;
	}
	if (methodName == "getTrue" && paramTypes.size() == 0
	) {
		auto requestCallerOnSuccess =
				[onSuccess = std::move(onSuccess)](const bool& result){
					BaseReply reply;
					reply.setResponse(
					result
					);
					onSuccess(std::move(reply));
				};

		try {
			multipleVersionsInterface1RequestCallerVar->getTrue(
					std::move(requestCallerOnSuccess),
					onError);
		} catch (const std::exception& exception) {
			const std::string errorMessage = "Unexpected exception occurred in method getTrue (...): " + std::string(exception.what());
			JOYNR_LOG_ERROR(logger(), errorMessage);
			onError(std::make_shared<exceptions::MethodInvocationException>(errorMessage, requestCaller->getProviderVersion()));
		}

		return;
	}
	if (methodName == "getVersionedStruct" && paramTypes.size() == 1
		&& paramTypes.at(0) == "joynr.tests.MultipleVersionsTypeCollection.VersionedStruct1"
	) {
		auto requestCallerOnSuccess =
				[onSuccess = std::move(onSuccess)](const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1& result){
					BaseReply reply;
					reply.setResponse(
					result
					);
					onSuccess(std::move(reply));
				};

		joynr::tests::MultipleVersionsTypeCollection::VersionedStruct1 input;
		try {
			request.getParams(input);
			multipleVersionsInterface1RequestCallerVar->getVersionedStruct(
					input,
					std::move(requestCallerOnSuccess),
					onError);
		} catch (const std::exception& exception) {
			const std::string errorMessage = "Unexpected exception occurred in method getVersionedStruct (...): " + std::string(exception.what());
			JOYNR_LOG_ERROR(logger(), errorMessage);
			onError(std::make_shared<exceptions::MethodInvocationException>(errorMessage, requestCaller->getProviderVersion()));
		}

		return;
	}
	if (methodName == "getAnonymousVersionedStruct" && paramTypes.size() == 1
		&& paramTypes.at(0) == "joynr.tests.AnonymousVersionedStruct1"
	) {
		auto requestCallerOnSuccess =
				[onSuccess = std::move(onSuccess)](const joynr::tests::AnonymousVersionedStruct1& result){
					BaseReply reply;
					reply.setResponse(
					result
					);
					onSuccess(std::move(reply));
				};

		joynr::tests::AnonymousVersionedStruct1 input;
		try {
			request.getParams(input);
			multipleVersionsInterface1RequestCallerVar->getAnonymousVersionedStruct(
					input,
					std::move(requestCallerOnSuccess),
					onError);
		} catch (const std::exception& exception) {
			const std::string errorMessage = "Unexpected exception occurred in method getAnonymousVersionedStruct (...): " + std::string(exception.what());
			JOYNR_LOG_ERROR(logger(), errorMessage);
			onError(std::make_shared<exceptions::MethodInvocationException>(errorMessage, requestCaller->getProviderVersion()));
		}

		return;
	}
	if (methodName == "getInterfaceVersionedStruct" && paramTypes.size() == 1
		&& paramTypes.at(0) == "joynr.tests.InterfaceVersionedStruct1"
	) {
		auto requestCallerOnSuccess =
				[onSuccess = std::move(onSuccess)](const joynr::tests::InterfaceVersionedStruct1& result){
					BaseReply reply;
					reply.setResponse(
					result
					);
					onSuccess(std::move(reply));
				};

		joynr::tests::InterfaceVersionedStruct1 input;
		try {
			request.getParams(input);
			multipleVersionsInterface1RequestCallerVar->getInterfaceVersionedStruct(
					input,
					std::move(requestCallerOnSuccess),
					onError);
		} catch (const std::exception& exception) {
			const std::string errorMessage = "Unexpected exception occurred in method getInterfaceVersionedStruct (...): " + std::string(exception.what());
			JOYNR_LOG_ERROR(logger(), errorMessage);
			onError(std::make_shared<exceptions::MethodInvocationException>(errorMessage, requestCaller->getProviderVersion()));
		}

		return;
	}

	JOYNR_LOG_WARN(logger(), "unknown method name for interface MultipleVersionsInterface1: {}", request.getMethodName());
	onError(
		std::make_shared<exceptions::MethodInvocationException>(
			"unknown method name for interface MultipleVersionsInterface1: " + request.getMethodName(),
			requestCaller->getProviderVersion()));
}

void MultipleVersionsInterface1RequestInterpreter::execute(
		std::shared_ptr<joynr::RequestCaller> requestCaller,
		OneWayRequest& request
) {
	std::ignore = requestCaller;

	JOYNR_LOG_WARN(logger(), "unknown method name for interface MultipleVersionsInterface1: {}", request.getMethodName());
}

} // namespace tests
} // namespace joynr
