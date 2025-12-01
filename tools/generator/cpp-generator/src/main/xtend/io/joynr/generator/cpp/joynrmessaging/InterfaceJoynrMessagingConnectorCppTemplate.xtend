package io.joynr.generator.cpp.joynrmessaging
/*
 * !!!
 *
 * Copyright (C) 2017 BMW Car IT GmbH
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

import com.google.inject.Inject
import io.joynr.generator.cpp.util.CppInterfaceUtil
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import java.io.File
import org.franca.core.franca.FMethod

class InterfaceJoynrMessagingConnectorCppTemplate extends InterfaceTemplate{

	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension NamingUtil
	@Inject extension AttributeUtil
	@Inject extension MethodUtil
	@Inject extension CppInterfaceUtil
	@Inject extension InterfaceSubscriptionUtil

	def produceParameterSetters(FMethod method, boolean generateVersion)
'''
«IF !method.fireAndForget»
joynr::Request request;
«ELSE»
joynr::OneWayRequest request;
«ENDIF»

request.setMethodName("«method.joynrName»");
request.setParamDatatypes({
	«FOR param : getInputParameters(method) SEPARATOR ','»
	"«param.getJoynrTypeName(generateVersion)»"
	«ENDFOR»
	});
request.setParams(
	«FOR param : getInputParameters(method) SEPARATOR ','»
		«param.name»
	«ENDFOR»
);
'''

	def getParamsPlaceholders(int numberOfParams) {
		var placeholders = ""
		for (var i = 0; i < numberOfParams; i++) {
			if(i != 0) {
				placeholders += ", "
			}
			placeholders += "{}"
		}
		return placeholders;
	}

	def logMethodCall(FMethod method)
	'''
		JOYNR_LOG_DEBUG(logger(),
				"«IF method.fireAndForget»ONEWAY«ENDIF»REQUEST call proxy: «
					IF !method.fireAndForget»requestReplyId: {}, «ENDIF
					»method: {}, params: «getParamsPlaceholders(method.inputParameters.size)»",
				«IF !method.fireAndForget»request.getRequestReplyId(),«ENDIF»
				request.getMethodName()
				«IF method.inputParameters.size > 0»,«ENDIF»
				«FOR inputParam : method.inputParameters SEPARATOR ','»
					joynr::serializer::serializeToJson(«inputParam.joynrName»)
				«ENDFOR»
				);
	'''

	override generate(boolean generateVersion)
'''
«val interfaceName = francaIntf.joynrName»
«val methodToErrorEnumName = francaIntf.methodToErrorEnumName()»
«warning()»

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«interfaceName»JoynrMessagingConnector.h"
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
#include "joynr/exceptions/MethodInvocationException.h"
#include "joynr/Request.h"
#include "joynr/OneWayRequest.h"
«IF !francaIntf.attributes.empty»
	#include "joynr/SubscriptionRequest.h"
«ENDIF»
«IF !francaIntf.broadcasts.filter[selective].empty»
	#include "joynr/BroadcastSubscriptionRequest.h"
«ENDIF»
«IF !francaIntf.broadcasts.filter[!selective].empty»
	#include "joynr/MulticastSubscriptionQos.h"
	#include "joynr/MulticastSubscriptionRequest.h"
«ENDIF»

«FOR method : getMethods(francaIntf)»
	«IF method.hasErrorEnum»
		«var enumType = method.errors»
		«IF enumType !== null»
			«enumType.name = methodToErrorEnumName.get(method)»
		«ELSE»
			«{enumType = method.errorEnum; ""}»
		«ENDIF»
		#include "«enumType.getPackagePathWithJoynrPrefix(File::separator, true, generateVersion) + File::separator + enumType.joynrName».h"
	«ENDIF»
«ENDFOR»

«FOR datatype: getAllComplexTypes(francaIntf)»
	«IF isCompound(datatype) || isMap(datatype)»
		#include «getIncludeOf(datatype, generateVersion)»
	«ENDIF»
«ENDFOR»

«FOR broadcastFilterParameters: getBroadcastFilterParametersIncludes(francaIntf, generateVersion)»
	#include «broadcastFilterParameters»
«ENDFOR»

using joynr::util::safeInvokeCallback;

«getNamespaceStarter(francaIntf, generateVersion)»
«val className = interfaceName + "JoynrMessagingConnector"»
«className»::«className»(
		std::weak_ptr<joynr::IMessageSender> messageSender,
		std::weak_ptr<joynr::ISubscriptionManager> subscriptionManager,
		const std::string& domain,
		const std::string& proxyParticipantId,
		const joynr::MessagingQos &qosSettings,
		const joynr::types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry)
	: joynr::AbstractJoynrMessagingConnector(messageSender, subscriptionManager, domain, INTERFACE_NAME(), proxyParticipantId, qosSettings, providerDiscoveryEntry)
{
}

«FOR attribute: getAttributes(francaIntf)»
	«val returnType = getTypeName(attribute, generateVersion)»
	«val attributeName = attribute.joynrName»
	«IF attribute.readable»
		«produceSyncGetterSignature(attribute, className, generateVersion)»
		{
			auto future = get«attributeName.toFirstUpper»Async(nullptr, nullptr, std::move(qos));
			future->get(«attributeName»);
		}

		«produceAsyncGetterSignature(attribute, className, generateVersion)»
		{
			joynr::Request request;
			// explicitly set to no parameters
			request.setParams();
			request.setMethodName("get«attributeName.toFirstUpper»");
			auto future = std::make_shared<joynr::Future<«returnType»>>();

			std::function<void(const «returnType»&)> onSuccessWrapper = [
					future,
					onSuccess = std::move(onSuccess),
					requestReplyId = request.getRequestReplyId(),
					methodName = request.getMethodName(),
					thisSharedPtr = shared_from_this()
			] (const «returnType»& «attributeName») {
				JOYNR_LOG_DEBUG(logger(),
						"REQUEST returns successful: requestReplyId: {}, method: {}, response: {}",
						requestReplyId,
						methodName,
						joynr::serializer::serializeToJson(«attributeName»)
				);
				safeInvokeCallback(logger(), onSuccess, «attributeName»);
				future->onSuccess(«attributeName»);
			};

			std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> onErrorWrapper = [
					future,
					onError = onError,
					requestReplyId = request.getRequestReplyId(),
					methodName = request.getMethodName()
			] (const std::shared_ptr<exceptions::JoynrException>& error) {
				assert(error);
				if (!error) {
					JOYNR_LOG_FATAL(logger(),
						"REQUEST returns error: requestReplyId: {}, method: {}, error not set, aborting",
						requestReplyId,
						methodName
					);
					return;
				}
				JOYNR_LOG_DEBUG(logger(),
						"REQUEST returns error: requestReplyId: {}, method: {}, response: {}",
						requestReplyId,
						methodName,
						error->what()
				);
				safeInvokeCallback(logger(), onError, static_cast<const exceptions::JoynrRuntimeException&>(*error));
				future->onError(error);
			};

			try {
				JOYNR_LOG_DEBUG(logger(),
						"REQUEST call proxy: requestReplyId: {}, method: {}",
						request.getRequestReplyId(),
						request.getMethodName());
				auto replyCaller = std::make_shared<joynr::ReplyCaller<«returnType»>>(std::move(onSuccessWrapper), std::move(onErrorWrapper));
				operationRequest(std::move(replyCaller), std::move(request), std::move(qos));
			} catch (const std::invalid_argument& exception) {
				auto joynrException = std::make_shared<joynr::exceptions::MethodInvocationException>(exception.what());
				safeInvokeCallback(logger(), onError, *joynrException);
				future->onError(joynrException);
			} catch (const joynr::exceptions::JoynrRuntimeException& exception) {
				std::shared_ptr<joynr::exceptions::JoynrRuntimeException> exceptionPtr;
				exceptionPtr.reset(exception.clone());
				safeInvokeCallback(logger(), onError, exception);
				future->onError(exceptionPtr);
			}
			return future;
		}

	«ENDIF»
	«IF attribute.writable»
		«produceAsyncSetterSignature(attribute, className, generateVersion)»
		{
			joynr::Request request;
			request.setMethodName("set«attributeName.toFirstUpper»");
			request.setParamDatatypes({"«attribute.getJoynrTypeName(generateVersion)»"});
			request.setParams(«attributeName»);

			auto future = std::make_shared<joynr::Future<void>>();

			std::function<void()> onSuccessWrapper = [
					future,
					onSuccess = std::move(onSuccess),
					requestReplyId = request.getRequestReplyId(),
					methodName = request.getMethodName(),
					thisSharedPtr = shared_from_this()
			] () {
				JOYNR_LOG_DEBUG(logger(),
						"REQUEST returns successful: requestReplyId: {}, method: {}",
						requestReplyId,
						methodName
				);
				safeInvokeCallback(logger(), onSuccess);
				future->onSuccess();
			};

			std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> onErrorWrapper = [
					future,
					onError = onError,
					requestReplyId = request.getRequestReplyId(),
					methodName = request.getMethodName()
			] (const std::shared_ptr<exceptions::JoynrException>& error) {
				assert(error);
				if (!error) {
					JOYNR_LOG_FATAL(logger(),
						"REQUEST returns error: requestReplyId: {}, method: {}, error not set, aborting",
						requestReplyId,
						methodName
					);
					return;
				}
				JOYNR_LOG_DEBUG(logger(),
					"REQUEST returns error: requestReplyId: {}, method: {}, response: {}",
					requestReplyId,
					methodName,
					error->what()
				);
				safeInvokeCallback(logger(), onError, static_cast<const exceptions::JoynrRuntimeException&>(*error));
				future->onError(error);
			};

			try {
				JOYNR_LOG_DEBUG(logger(),
						"REQUEST call proxy: requestReplyId: {}, method: {}, params: {}",
						request.getRequestReplyId(),
						request.getMethodName(),
						joynr::serializer::serializeToJson(«attributeName»));
				auto replyCaller = std::make_shared<joynr::ReplyCaller<void>>(std::move(onSuccessWrapper), std::move(onErrorWrapper));
				operationRequest(std::move(replyCaller), std::move(request), std::move(qos));
			} catch (const std::invalid_argument& exception) {
				auto joynrException = std::make_shared<joynr::exceptions::MethodInvocationException>(exception.what());
				safeInvokeCallback(logger(), onError, *joynrException);
				future->onError(joynrException);
			} catch (const joynr::exceptions::JoynrRuntimeException& exception) {
				std::shared_ptr<joynr::exceptions::JoynrRuntimeException> exceptionPtr;
				exceptionPtr.reset(exception.clone());
				safeInvokeCallback(logger(), onError, exception);
				future->onError(exceptionPtr);
			}
			return future;
		}

		«produceSyncSetterSignature(attribute, className, generateVersion)»
		{
			auto future = set«attributeName.toFirstUpper»Async(«attributeName», nullptr, nullptr, std::move(qos));
			future->get();
		}

	«ENDIF»
	«IF attribute.notifiable»
		«produceSubscribeToAttributeSignature(attribute, className, generateVersion)» {
			joynr::SubscriptionRequest subscriptionRequest;
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		«produceUpdateAttributeSubscriptionSignature(attribute, className, generateVersion)» {

			joynr::SubscriptionRequest subscriptionRequest;
			subscriptionRequest.setSubscriptionId(subscriptionId);
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		std::shared_ptr<joynr::Future<std::string>> «className»::subscribeTo«attributeName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
					SubscriptionRequest& subscriptionRequest
		) {
			JOYNR_LOG_TRACE(logger(), "Subscribing to «attributeName».");
			std::string attributeName("«attributeName»");
			joynr::MessagingQos clonedMessagingQos(_qosSettings);
			clonedMessagingQos.setTtl(static_cast<std::uint64_t>(ISubscriptionManager::convertExpiryDateIntoTtlMs(*subscriptionQos)));

			auto future = std::make_shared<Future<std::string>>();
			std::shared_ptr<joynr::AbstractJoynrMessagingConnector> messagingConnector = shared_from_this();
			auto subscriptionCallback = std::make_shared<joynr::UnicastSubscriptionCallback<«returnType»>
			>(subscriptionRequest.getSubscriptionId(), future, _subscriptionManager, messagingConnector);
			if (auto ptr = _subscriptionManager.lock()) {
				ptr->registerSubscription(
						attributeName,
						subscriptionCallback,
						subscriptionListener,
						subscriptionQos,
						subscriptionRequest);
			}
			JOYNR_LOG_DEBUG(logger(),
					"SUBSCRIPTION call proxy: subscriptionId: {}, attribute: {}, qos: {}",
					subscriptionRequest.getSubscriptionId(),
					attributeName,
					joynr::serializer::serializeToJson(*subscriptionQos));

			if (auto ptr = _messageSender.lock()) {
				ptr->sendSubscriptionRequest(
						_proxyParticipantId,
						_providerParticipantId,
						clonedMessagingQos,
						subscriptionRequest,
						_providerDiscoveryEntry.getIsLocal()
						);
			}
			return future;
		}

		«produceUnsubscribeFromAttributeSignature(attribute, className)» {
			joynr::SubscriptionStop subscriptionStop;
			subscriptionStop.setSubscriptionId(subscriptionId);

			if (auto ptr = _subscriptionManager.lock()) {
				ptr->unregisterSubscription(subscriptionId);
			}
			if (auto ptr = _messageSender.lock()) {
				ptr->sendSubscriptionStop(
						_proxyParticipantId,
						_providerParticipantId,
						_qosSettings,
						subscriptionStop
						);
			}
		}

	«ENDIF»
«ENDFOR»

«FOR method: getMethods(francaIntf)»
	«var outputTypedConstParamList = getCommaSeperatedTypedConstOutputParameterList(method, generateVersion)»
	«val outputParameters = getCommaSeparatedOutputParameterTypes(method, generateVersion)»
	«var outputUntypedParamList = getCommaSeperatedUntypedOutputParameterList(method)»

	«IF !method.fireAndForget»
		«produceSyncMethodSignature(method, className, generateVersion)»
		{
			auto future = «method.joynrName»Async(«method.commaSeperatedUntypedInputParameterList»«IF !method.inputParameters.empty», «ENDIF»«IF method.hasErrorEnum»nullptr,«ENDIF»nullptr, nullptr, std::move(qos));
			future->get(«method.commaSeperatedUntypedOutputParameterList»);
		}

		«produceAsyncMethodSignature(francaIntf, method, className, generateVersion)»
		{
			«produceParameterSetters(method, generateVersion)»

			auto future = std::make_shared<joynr::Future<«outputParameters»>>();

			std::function<void(«outputTypedConstParamList»)> onSuccessWrapper = [
					future,
					onSuccess = std::move(onSuccess),
					requestReplyId = request.getRequestReplyId(),
					methodName = request.getMethodName(),
					thisSharedPtr = shared_from_this()
			] («outputTypedConstParamList»)
			{
				JOYNR_LOG_DEBUG(logger(),
					"REQUEST returns successful: requestReplyId: {}, method: {}, response: «getParamsPlaceholders(method.outputParameters.size)»",
					requestReplyId,
					methodName«IF !method.outputParameters.empty»,«ENDIF»
					«FOR outParam : method.outputParameters SEPARATOR ", "»
						joynr::serializer::serializeToJson(«outParam.joynrName»)
					«ENDFOR»
				);
				«IF outputUntypedParamList.length > 0»
				safeInvokeCallback(logger(), onSuccess, «outputUntypedParamList»);
				«ELSE»
				safeInvokeCallback(logger(), onSuccess);
				«ENDIF»
				future->onSuccess(«outputUntypedParamList»);
			};

			std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> onErrorWrapper = [
					future,
					onRuntimeError = onRuntimeError,
					«IF method.hasErrorEnum»onApplicationError = std::move(onApplicationError),«ENDIF»
					requestReplyId = request.getRequestReplyId(),
					methodName = request.getMethodName()
			] (const std::shared_ptr<exceptions::JoynrException>& error)
			{
				assert(error);
				if (!error) {
					JOYNR_LOG_FATAL(logger(),
						"REQUEST returns error: requestReplyId: {}, method: {}, error not set, aborting",
						requestReplyId,
						methodName
					);
					return;
				}
				JOYNR_LOG_DEBUG(logger(),
					"REQUEST returns error: requestReplyId: {}, method: {}, response: {}",
					requestReplyId,
					methodName,
					error->what()
				);
				«produceApplicationRuntimeErrorSplitForOnErrorWrapper(francaIntf, method, generateVersion)»
			};

			try {
				«logMethodCall(method)»

				auto replyCaller = std::make_shared<joynr::ReplyCaller<«outputParameters»>>(std::move(onSuccessWrapper), std::move(onErrorWrapper));
				operationRequest(std::move(replyCaller), std::move(request), std::move(qos));
			} catch (const std::invalid_argument& exception) {
				auto joynrException = std::make_shared<joynr::exceptions::MethodInvocationException>(exception.what());
				safeInvokeCallback(logger(), onRuntimeError, *joynrException);
				future->onError(joynrException);
			} catch (const joynr::exceptions::JoynrRuntimeException& exception) {
				std::shared_ptr<joynr::exceptions::JoynrRuntimeException> exceptionPtr;
				exceptionPtr.reset(exception.clone());
				safeInvokeCallback(logger(), onRuntimeError, exception);
				future->onError(exceptionPtr);
			}
			return future;
		}
	«ELSE»
		«produceFireAndForgetMethodSignature(method, className, generateVersion)»
			{
				«produceParameterSetters(method, generateVersion)»

				«logMethodCall(method)»
				operationOneWayRequest(std::move(request), std::move(qos));
			}
	«ENDIF»
«ENDFOR»

«FOR broadcast: francaIntf.broadcasts»
	«val returnTypes = getCommaSeparatedOutputParameterTypes(broadcast, generateVersion)»
	«val broadcastName = broadcast.joynrName»
	«produceSubscribeToBroadcastSignature(broadcast, francaIntf, className, generateVersion)» {
		«IF broadcast.selective»
			joynr::BroadcastSubscriptionRequest subscriptionRequest;
			subscriptionRequest.setFilterParameters(filterParameters);
		«ELSE»
			auto subscriptionRequest = std::make_shared<joynr::MulticastSubscriptionRequest>();
		«ENDIF»
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(
					std::move(subscriptionListener),
					std::move(subscriptionQos),
					subscriptionRequest«
					»«IF !broadcast.selective»«
					»,
					partitions«
					»«ENDIF»«
					»);
	}

	«produceUpdateBroadcastSubscriptionSignature(broadcast, francaIntf, className, generateVersion)» {
		«IF broadcast.selective»
			joynr::BroadcastSubscriptionRequest subscriptionRequest;
			subscriptionRequest.setFilterParameters(filterParameters);
			subscriptionRequest.setSubscriptionId(subscriptionId);
		«ELSE»
			auto subscriptionRequest = std::make_shared<joynr::MulticastSubscriptionRequest>();
			subscriptionRequest->setSubscriptionId(subscriptionId);
		«ENDIF»
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(
					std::move(subscriptionListener),
					std::move(subscriptionQos),
					subscriptionRequest«
					»«IF !broadcast.selective»«
					»,
					partitions«
					»«ENDIF»«
					»);
	}

	std::shared_ptr<joynr::Future<std::string>> «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				«IF broadcast.selective»
					std::shared_ptr<joynr::OnChangeSubscriptionQos> subscriptionQos,
					BroadcastSubscriptionRequest& subscriptionRequest
				«ELSE»
					std::shared_ptr<joynr::MulticastSubscriptionQos> subscriptionQos,
					std::shared_ptr<MulticastSubscriptionRequest> subscriptionRequest,
					const std::vector<std::string>& partitions
				«ENDIF»
	) {
		JOYNR_LOG_TRACE(logger(), "Subscribing to «broadcastName» «IF broadcast.selective»broadcast«ELSE»multicast«ENDIF».");
		std::string broadcastName("«broadcastName»");
		joynr::MessagingQos clonedMessagingQos(_qosSettings);
		clonedMessagingQos.setTtl(static_cast<std::uint64_t>(ISubscriptionManager::convertExpiryDateIntoTtlMs(*subscriptionQos)));

		auto future = std::make_shared<Future<std::string>>();
		std::shared_ptr<joynr::AbstractJoynrMessagingConnector> messagingConnector = shared_from_this();
		«IF broadcast.selective»
			auto subscriptionCallback = std::make_shared<joynr::UnicastSubscriptionCallback<«returnTypes»>
			>(subscriptionRequest.getSubscriptionId(), future, _subscriptionManager, messagingConnector);
			if (auto ptr = _subscriptionManager.lock()) {
				ptr->registerSubscription(
						broadcastName,
						subscriptionCallback,
						subscriptionListener,
						subscriptionQos,
						subscriptionRequest);
			}

			JOYNR_LOG_DEBUG(logger(),
					"SUBSCRIPTION call proxy: subscriptionId: {}, broadcast: {}, qos: {}",
					subscriptionRequest.getSubscriptionId(),
					broadcastName,
					joynr::serializer::serializeToJson(*subscriptionQos));
			if (auto ptr = _messageSender.lock()) {
				ptr->sendBroadcastSubscriptionRequest(
						_proxyParticipantId,
						_providerParticipantId,
						clonedMessagingQos,
						subscriptionRequest,
						_providerDiscoveryEntry.getIsLocal()
						);
			}
		«ELSE»
			auto subscriptionCallback = std::make_shared<joynr::MulticastSubscriptionCallback<«returnTypes»>
			>(subscriptionRequest->getSubscriptionId(), future, _subscriptionManager, messagingConnector);
			std::string subscriptionId = subscriptionRequest«IF broadcast.selective».«ELSE»->«ENDIF»getSubscriptionId();
			std::function<void()> onSuccess =
					[messageSender = _messageSender,
					proxyParticipantId = _proxyParticipantId,
					providerParticipantId = _providerParticipantId,
					clonedMessagingQos, subscriptionId] () {
						if (auto ptr = messageSender.lock())
						{
							SubscriptionReply subscriptionReply;
							subscriptionReply.setSubscriptionId(subscriptionId);
							ptr->sendSubscriptionReply(
										providerParticipantId,
										proxyParticipantId,
										clonedMessagingQos,
										subscriptionReply
							);
						}
					};

			std::function<void(const exceptions::ProviderRuntimeException& error)> onError =
					[subscriptionListener,
					subscriptionManager = _subscriptionManager,
					subscriptionId] (const exceptions::ProviderRuntimeException& error) {
						std::string message = "Could not register subscription to «broadcastName». Error from subscription manager: "
									+ error.getMessage();
						JOYNR_LOG_ERROR(logger(), message);
						exceptions::SubscriptionException subscriptionException(message, subscriptionId);
						subscriptionListener->onError(subscriptionException);
						if (auto ptr = subscriptionManager.lock())
						{
							ptr->unregisterSubscription(subscriptionId);
						}
				};

			if (auto ptr = _subscriptionManager.lock()) {
				ptr->registerSubscription(
						broadcastName,
						_proxyParticipantId,
						_providerParticipantId,
						partitions,
						subscriptionCallback,
						subscriptionListener,
						subscriptionQos,
						*subscriptionRequest,
						std::move(onSuccess),
						std::move(onError));
			}
		«ENDIF»
		return future;
	}

	«produceUnsubscribeFromBroadcastSignature(broadcast, className)» {
		if (auto ptr = _subscriptionManager.lock()) {
			ptr->unregisterSubscription(subscriptionId);
		}
		«IF broadcast.selective»
			joynr::SubscriptionStop subscriptionStop;
			subscriptionStop.setSubscriptionId(subscriptionId);
			if (auto msgSenderPtr = _messageSender.lock()) {
				msgSenderPtr->sendSubscriptionStop(
						_proxyParticipantId,
						_providerParticipantId,
						_qosSettings,
						subscriptionStop
						);
			}
		«ENDIF»
	}

«ENDFOR»
«getNamespaceEnder(francaIntf, generateVersion)»
'''
}
