package io.joynr.generator.cpp.inprocess
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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil

class InterfaceInProcessConnectorCPPTemplate extends InterfaceTemplate{

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension CppInterfaceUtil
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension MethodUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension InterfaceSubscriptionUtil

	override  generate()
'''
«var interfaceName = francaIntf.joynrName»
«warning()»
#include <cassert>
#include <functional>
#include <memory>
#include <tuple>

#include "joynr/serializer/Serializer.h"

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»InProcessConnector.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»RequestCaller.h"
«FOR datatype: getAllComplexTypes(francaIntf)»
	«IF isCompound(datatype) || isMap(datatype)»
		#include «getIncludeOf(datatype)»
	«ENDIF»
«ENDFOR»

«FOR broadcastFilterParameters: getBroadcastFilterParametersIncludes(francaIntf)»
	#include «broadcastFilterParameters»
«ENDFOR»

#include "joynr/InProcessAddress.h"
#include "joynr/ISubscriptionManager.h"
#include "joynr/PublicationManager.h"
#include "joynr/UnicastSubscriptionCallback.h"
#include "joynr/MulticastSubscriptionCallback.h"
#include "joynr/Util.h"
#include "joynr/Future.h"
#include "joynr/SubscriptionUtil.h"
#include "joynr/CallContext.h"
#include "joynr/IPlatformSecurityManager.h"
#include "joynr/exceptions/JoynrException.h"
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

«getNamespaceStarter(francaIntf)»

«val className = interfaceName + "InProcessConnector"»
INIT_LOGGER(«className»);

«className»::«className»(
			joynr::ISubscriptionManager* subscriptionManager,
			joynr::PublicationManager* publicationManager,
			joynr::InProcessPublicationSender* inProcessPublicationSender,
			std::shared_ptr<joynr::IPlatformSecurityManager> securityManager,
			const std::string& proxyParticipantId,
			const std::string& providerParticipantId,
			std::shared_ptr<joynr::InProcessAddress> address
) :
	proxyParticipantId(proxyParticipantId),
	providerParticipantId(providerParticipantId),
	address(address),
	subscriptionManager(subscriptionManager),
	publicationManager(publicationManager),
	inProcessPublicationSender(inProcessPublicationSender),
	securityManager(securityManager)
{
}

«FOR attribute : getAttributes(francaIntf)»
	«val returnType = attribute.typeName»
	«val attributeName = attribute.joynrName»
	«val setAttributeName = "set" + attribute.joynrName.toFirstUpper»
	«IF attribute.readable»
		«val getAttributeName = "get" + attribute.joynrName.toFirstUpper»
		«produceSyncGetterSignature(attribute, className)»
		{
			assert(address);
			std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(caller);
			std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
			assert(«francaIntf.interfaceCaller»);

			auto future = std::make_shared<joynr::Future<«returnType»>>();

			std::function<void(const «returnType»& «attributeName»)> onSuccess =
					[future] (const «returnType»& «attributeName») {
						future->onSuccess(«attributeName»);
					};

			std::function<void(const std::shared_ptr<exceptions::ProviderRuntimeException>&)> onError =
					[future] (const std::shared_ptr<exceptions::ProviderRuntimeException>& error) {
						future->onError(error);
					};

			//see header for more information
			«francaIntf.interfaceCaller»->«getAttributeName»(std::move(onSuccess), std::move(onError));
			future->get(«attributeName»);
		}

		«produceAsyncGetterSignature(attribute, className)»
		{
			assert(address);
			std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(caller);
			std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
			assert(«francaIntf.interfaceCaller»);

			auto future = std::make_shared<joynr::Future<«returnType»>>();

			std::function<void(const «returnType»& «attributeName»)> onSuccessWrapper =
					[future, onSuccess = std::move(onSuccess)] (const «returnType»& «attributeName») {
						future->onSuccess(«attributeName»);
						if (onSuccess) {
							onSuccess(«attributeName»);
						}
					};

			std::function<void(const std::shared_ptr<exceptions::ProviderRuntimeException>&)> onErrorWrapper =
					[future, onError = std::move(onError)] (const std::shared_ptr<exceptions::ProviderRuntimeException>& error) {
						future->onError(error);
						if (onError) {
							onError(*error);
						}
					};

			//see header for more information
			«francaIntf.interfaceCaller»->«getAttributeName»(std::move(onSuccessWrapper), std::move(onErrorWrapper));
			return future;
		}

	«ENDIF»
	«IF attribute.writable»
		«produceAsyncSetterSignature(attribute, className)»
		{
			assert(address);
			std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(caller);
			std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
			assert(«francaIntf.interfaceCaller»);

			auto future = std::make_shared<joynr::Future<void>>();
			std::function<void()> onSuccessWrapper =
					[future, onSuccess = std::move(onSuccess)] () {
						future->onSuccess();
						if (onSuccess) {
							onSuccess();
						}
					};

			std::function<void(const std::shared_ptr<exceptions::ProviderRuntimeException>&)> onErrorWrapper =
					[future, onError = std::move(onError)] (const std::shared_ptr<exceptions::ProviderRuntimeException>& error) {
						future->onError(error);
						if (onError) {
							onError(*error);
						}
					};

			//see header for more information
			JOYNR_LOG_ERROR(logger, "#### WARNING ##### «interfaceName»InProcessConnector::«setAttributeName»(Future) is synchronous.");
			«francaIntf.interfaceCaller»->«setAttributeName»(«attributeName», std::move(onSuccessWrapper), std::move(onErrorWrapper));
			return future;
		}

		«produceSyncSetterSignature(attribute, className)»
		{
			assert(address);
			std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(caller);
			std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
			assert(«francaIntf.interfaceCaller»);

			auto future = std::make_shared<joynr::Future<void>>();
			std::function<void()> onSuccess =
					[future] () {
						future->onSuccess();
					};

			std::function<void(const std::shared_ptr<exceptions::ProviderRuntimeException>&)> onError =
					[future] (const std::shared_ptr<exceptions::ProviderRuntimeException>& error) {
						future->onError(error);
					};

			//see header for more information
			«francaIntf.interfaceCaller»->«setAttributeName»(«attributeName», std::move(onSuccess), std::move(onError));
			return future->get();
		}

	«ENDIF»
	«IF attribute.notifiable»
		«produceSubscribeToAttributeSignature(attribute, className)»
		{
			joynr::SubscriptionRequest subscriptionRequest;
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		«produceUpdateAttributeSubscriptionSignature(attribute, className)»
		{
			joynr::SubscriptionRequest subscriptionRequest;
			subscriptionRequest.setSubscriptionId(subscriptionId);
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		std::shared_ptr<joynr::Future<std::string>> «className»::subscribeTo«attributeName.toFirstUpper»(
				std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
				std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
				joynr::SubscriptionRequest& subscriptionRequest)
		{
			«IF isEnum(attribute.type)»
				std::ignore = subscriptionListener;
				std::ignore = subscriptionQos;
				std::ignore = subscriptionRequest;
				// TODO support enum return values in C++ client
				JOYNR_LOG_FATAL(logger, "enum return values are currently not supported in C++ client (attribute name: «interfaceName».«attributeName»)");
				assert(false);
				// Visual C++ requires a return value
				return std::make_shared<Future<std::string>>();
			«ELSE»
				JOYNR_LOG_TRACE(logger, "Subscribing to «attributeName».");
				assert(subscriptionManager != nullptr);
				std::string attributeName("«attributeName»");
				auto future = std::make_shared<Future<std::string>>();
				auto subscriptionCallback = std::make_shared<
						joynr::UnicastSubscriptionCallback<«returnType»>
				>(subscriptionRequest.getSubscriptionId(), future, subscriptionManager);
				subscriptionManager->registerSubscription(
						attributeName,
						subscriptionCallback,
						subscriptionListener,
						subscriptionQos,
						subscriptionRequest);
				JOYNR_LOG_TRACE(logger, "Registered subscription: {}", subscriptionRequest.toString());
				assert(address);
				std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
				assert(caller);
				std::shared_ptr<«interfaceName»RequestCaller> requestCaller = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);

				joynr::CallContext callContext;
				callContext.setPrincipal(securityManager->getCurrentProcessUserId());

				if(!requestCaller) {
					assert(publicationManager != nullptr);
					/**
					* Provider not registered yet
					* Dispatcher will call publicationManger->restore when a new provider is added to activate
					* subscriptions for that provider
					*/
					publicationManager->add(proxyParticipantId, providerParticipantId, subscriptionRequest);
				} else {
					publicationManager->add(proxyParticipantId, providerParticipantId, caller, subscriptionRequest, inProcessPublicationSender);
				}
				return future;
			«ENDIF»
		}

		«produceUnsubscribeFromAttributeSignature(attribute, className)» {
			«IF isEnum(attribute.type)»
				std::ignore = subscriptionId;
				// TODO support enum return values in C++ client
				JOYNR_LOG_FATAL(logger, "enum return values are currently not supported in C++ client (attribute name: «interfaceName».«attributeName»)");
				assert(false);
			«ELSE»
				JOYNR_LOG_TRACE(logger, "Unsubscribing. Id={}", subscriptionId);
				assert(publicationManager != nullptr);
				JOYNR_LOG_TRACE(logger, "Stopping publications by publication manager.");
				publicationManager->stopPublication(subscriptionId);
				assert(subscriptionManager != nullptr);
				JOYNR_LOG_TRACE(logger, "Unregistering attribute subscription.");
				subscriptionManager->unregisterSubscription(subscriptionId);
			«ENDIF»
		}

	«ENDIF»
«ENDFOR»

«FOR method: getMethods(francaIntf)»
«var methodname = method.joynrName»
«var outputParameters = method.commaSeparatedOutputParameterTypes»
«var inputParamList = method.commaSeperatedUntypedInputParameterList»
«var outputTypedConstParamList = method.commaSeperatedTypedConstOutputParameterList»
«var outputUntypedParamList = method.commaSeperatedUntypedOutputParameterList»

«produceSyncMethodSignature(method, className)»
{
	assert(address);
	std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
	assert(caller);
	std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
	assert(«francaIntf.interfaceCaller»);
	«IF method.fireAndForget»
		«francaIntf.interfaceCaller»->«methodname»(«inputParamList»);
	«ELSE»
		auto future = std::make_shared<joynr::Future<«outputParameters»>>();

		std::function<void(«outputTypedConstParamList»)> onSuccess =
				[future] («outputTypedConstParamList») {
					future->onSuccess(
							«outputUntypedParamList»
					);
				};

		std::function<void(const std::shared_ptr<exceptions::JoynrException>&)> onError =
				[future] (const std::shared_ptr<exceptions::JoynrException>& error) {
					future->onError(error);
				};
		«francaIntf.interfaceCaller»->«methodname»(«IF !method.inputParameters.empty»«inputParamList», «ENDIF»std::move(onSuccess), std::move(onError));
		future->get(«method.commaSeperatedUntypedOutputParameterList»);
	«ENDIF»
}
«IF !method.fireAndForget»
	«produceAsyncMethodSignature(francaIntf, method, className)»
	{
		assert(address);
		std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
		assert(caller);
		std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
		assert(«francaIntf.interfaceCaller»);
		auto future = std::make_shared<joynr::Future<«outputParameters»>>();

		std::function<void(«outputTypedConstParamList»)> onSuccessWrapper =
				[future, onSuccess = std::move(onSuccess)] («outputTypedConstParamList») {
					future->onSuccess(«outputUntypedParamList»);
					if (onSuccess)
					{
						onSuccess(«outputUntypedParamList»);
					}
				};

		std::function<void(const std::shared_ptr<exceptions::JoynrException>&)> onErrorWrapper =
				[future, onRuntimeError = std::move(onRuntimeError)«IF method.hasErrorEnum», onApplicationError = std::move(onApplicationError)«ENDIF»] (const std::shared_ptr<exceptions::JoynrException>& error) {
					future->onError(error);
					«produceApplicationRuntimeErrorSplitForOnErrorWrapper(francaIntf, method)»
				};

		«francaIntf.interfaceCaller»->«methodname»(«IF !method.inputParameters.empty»«inputParamList», «ENDIF»std::move(onSuccessWrapper), std::move(onErrorWrapper));
		return future;
	}
«ENDIF»

«ENDFOR»

«FOR broadcast: francaIntf.broadcasts»
	«val returnTypes = broadcast.commaSeparatedOutputParameterTypes»
	«val broadcastName = broadcast.joynrName»

	«produceSubscribeToBroadcastSignature(broadcast, francaIntf, className)» {
		JOYNR_LOG_TRACE(logger, "Subscribing to «broadcastName».");
		assert(subscriptionManager != nullptr);
		«IF broadcast.selective»
			joynr::BroadcastSubscriptionRequest subscriptionRequest;
			subscriptionRequest.setFilterParameters(filterParameters);
		«ELSE»
			auto subscriptionRequest = std::make_shared<joynr::MulticastSubscriptionRequest>();
		«ENDIF»
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(
					subscriptionListener,
					subscriptionQos,
					subscriptionRequest«
					»«IF !broadcast.selective»«
					»,
					partitions«
					»«ENDIF»«
					»);
	}

	«produceUpdateBroadcastSubscriptionSignature(broadcast, francaIntf, className)» {
		«IF broadcast.selective»
			joynr::BroadcastSubscriptionRequest subscriptionRequest;
			subscriptionRequest.setFilterParameters(filterParameters);
			subscriptionRequest.setSubscriptionId(subscriptionId);
		«ELSE»
			auto subscriptionRequest = std::make_shared<joynr::MulticastSubscriptionRequest>();
			subscriptionRequest->setSubscriptionId(subscriptionId);
		«ENDIF»
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(
					subscriptionListener,
					subscriptionQos,
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
		JOYNR_LOG_TRACE(logger, "Subscribing to «broadcastName».");
		assert(subscriptionManager != nullptr);
		std::string broadcastName("«broadcastName»");

		auto future = std::make_shared<Future<std::string>>();
		assert(address);
		«IF broadcast.selective»
			auto subscriptionCallback = std::make_shared<
				joynr::UnicastSubscriptionCallback<«returnTypes»>
			>(subscriptionRequest.getSubscriptionId(), future, subscriptionManager);
			subscriptionManager->registerSubscription(
						broadcastName,
						subscriptionCallback,
						subscriptionListener,
						subscriptionQos,
						subscriptionRequest);
			JOYNR_LOG_TRACE(
					logger,
					"Registered broadcast subscription: {}",
					subscriptionRequest.toString());
			std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(caller);
			assert(publicationManager != nullptr);
			std::shared_ptr<«interfaceName»RequestCaller> requestCaller =
					std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);

			if(!requestCaller) {
				/**
				* Provider not registered yet
				* Dispatcher will call publicationManger->restore when a new provider
				* is added to activate subscriptions for that provider
				*/
				publicationManager->add(
						proxyParticipantId,
						providerParticipantId,
						subscriptionRequest);
			} else {
				publicationManager->add(
							proxyParticipantId,
							providerParticipantId,
							caller,
							subscriptionRequest,
							inProcessPublicationSender);
			}
		«ELSE»
			auto subscriptionCallback = std::make_shared<
				joynr::MulticastSubscriptionCallback<«returnTypes»>
			>(subscriptionRequest->getSubscriptionId(), future, subscriptionManager);
			std::function<void()> onSuccess =
					[this, subscriptionRequest] () {
						JOYNR_LOG_TRACE(
								logger,
								"Registered broadcast subscription: {}",
								subscriptionRequest->toString());
						publicationManager->add(
									proxyParticipantId,
									providerParticipantId,
									*subscriptionRequest,
									inProcessPublicationSender);
					};

			std::string subscriptionId = subscriptionRequest«IF broadcast.selective».«ELSE»->«ENDIF»getSubscriptionId();
			std::function<void(const exceptions::ProviderRuntimeException& error)> onError =
				[this, subscriptionListener, subscriptionId]
				(const exceptions::ProviderRuntimeException& error) {
					std::string message = "Could not register subscription to" \
							" «broadcastName»." \
							" Error from subscription manager: "
							+ error.getMessage();
					JOYNR_LOG_ERROR(logger, message);
					exceptions::SubscriptionException subscriptionException(
							message,
							subscriptionId);
					subscriptionListener->onError(subscriptionException);
					subscriptionManager->unregisterSubscription(subscriptionId);
				};
			subscriptionManager->registerSubscription(
							broadcastName,
							proxyParticipantId,
							providerParticipantId,
							partitions,
							subscriptionCallback,
							subscriptionListener,
							subscriptionQos,
							*subscriptionRequest,
							std::move(onSuccess),
							std::move(onError));
		«ENDIF»
		return future;
	}

	«produceUnsubscribeFromBroadcastSignature(broadcast, className)» {
		JOYNR_LOG_TRACE(logger, "Unsubscribing broadcast. Id={}", subscriptionId);
		assert(publicationManager != nullptr);
		JOYNR_LOG_TRACE(logger, "Stopping publications by publication manager.");
		publicationManager->stopPublication(subscriptionId);
		assert(subscriptionManager != nullptr);
		JOYNR_LOG_TRACE(logger, "Unregistering broadcast subscription.");
		subscriptionManager->unregisterSubscription(subscriptionId);
	}
«ENDFOR»
«getNamespaceEnder(francaIntf)»
'''

	def getInterfaceCaller(FInterface serviceInterface){
		serviceInterface.joynrName.toFirstLower + "Caller"
	}
}
