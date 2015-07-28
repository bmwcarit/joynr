package io.joynr.generator.cpp.inprocess
/*
 * !!!
 *
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.util.InterfaceTemplate
import org.franca.core.franca.FInterface
import org.franca.core.franca.FType
import io.joynr.generator.cpp.util.QtTypeUtil

class InterfaceInProcessConnectorCPPTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension CppStdTypeUtil cppStdTypeUtil

	@Inject
	private QtTypeUtil qtTypeUtil

	@Inject
	private extension JoynrCppGeneratorExtensions

	override  generate(FInterface serviceInterface)
'''
«var interfaceName = serviceInterface.joynrName»
«warning()»
#include <functional>

#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»InProcessConnector.h"
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestCaller.h"
#include "joynr/DeclareMetatypeUtil.h"
«FOR datatype: getAllComplexAndEnumTypes(serviceInterface)»
«IF datatype instanceof FType»
	«IF isComplex(datatype)»
		#include "«getIncludeOf(datatype)»"
	«ENDIF»
«ENDIF»
«ENDFOR»

#include "joynr/InProcessAddress.h"
#include "joynr/ISubscriptionManager.h"
#include "joynr/PublicationManager.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/Future.h"
#include "joynr/TypeUtil.h"

«getNamespaceStarter(serviceInterface)»

using namespace joynr::joynr_logging;
Logger* «interfaceName»InProcessConnector::logger = Logging::getInstance()->getLogger("MSG", "«interfaceName»InProcessConnector");

«interfaceName»InProcessConnector::«interfaceName»InProcessConnector(
			joynr::ISubscriptionManager* subscriptionManager,
			joynr::PublicationManager* publicationManager,
			joynr::InProcessPublicationSender* inProcessPublicationSender,
			const std::string& proxyParticipantId,
			const std::string& providerParticipantId,
			QSharedPointer<joynr::InProcessAddress> address
) :
	proxyParticipantId(proxyParticipantId),
	providerParticipantId(providerParticipantId),
	address(address),
	subscriptionManager(subscriptionManager),
	publicationManager(publicationManager),
	inProcessPublicationSender(inProcessPublicationSender)
{
}

bool «interfaceName»InProcessConnector::usesClusterController() const{
	return false;
}

«FOR attribute : getAttributes(serviceInterface)»
	«val returnType = cppStdTypeUtil.getTypeName(attribute)»
	«val returnTypeQt = qtTypeUtil.getTypeName(attribute)»
	«val attributeName = attribute.joynrName»
	«val setAttributeName = "set" + attribute.joynrName.toFirstUpper»
	«IF attribute.readable»
		«val getAttributeName = "get" + attribute.joynrName.toFirstUpper»
		void «interfaceName»InProcessConnector::«getAttributeName»(
					joynr::RequestStatus& status,
					«returnType»& attributeValue
		) {
			assert(!address.isNull());
			QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(!caller.isNull());
			QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			assert(!«serviceInterface.interfaceCaller».isNull());

			QSharedPointer<joynr::Future<«returnType»> > future(new joynr::Future<«returnType»>());

			std::function<void(const «returnType»& «attributeName»)> onSuccess =
					[future] (const «returnType»& «attributeName») {
						future->onSuccess(joynr::RequestStatusCode::OK, «attributeName»);
					};

			//see header for more information
			«serviceInterface.interfaceCaller»->«getAttributeName»(onSuccess);
			status = future->waitForFinished();
			if (status.successful()) {
				future->getValues(attributeValue);
			}
		}

		std::shared_ptr<joynr::Future<«returnType»>> «interfaceName»InProcessConnector::«getAttributeName»(
				std::function<void(const joynr::RequestStatus& status, const «returnType»& «attributeName»)> callbackFct
		) {
			assert(!address.isNull());
			QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(!caller.isNull());
			QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			assert(!«serviceInterface.interfaceCaller».isNull());

			std::shared_ptr<joynr::Future<«returnType»> > future(new joynr::Future<«returnType»>());

			std::function<void(const «returnType»& «attributeName»)> onSuccess =
					[future, callbackFct] (const «returnType»& «attributeName») {
						future->onSuccess(joynr::RequestStatusCode::OK, «attributeName»);
						if (callbackFct) {
							callbackFct(joynr::RequestStatusCode::OK, «attributeName»);
						}
					};

			//see header for more information
			«serviceInterface.interfaceCaller»->«getAttributeName»(onSuccess);
			return future;
		}

	«ENDIF»
	«IF attribute.writable»
		std::shared_ptr<joynr::Future<void>> «interfaceName»InProcessConnector::«setAttributeName»(
				«returnType» input,
				std::function<void(const joynr::RequestStatus& status)> callbackFct
		) {
			assert(!address.isNull());
			QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(!caller.isNull());
			QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			assert(!«serviceInterface.interfaceCaller».isNull());

			std::shared_ptr<joynr::Future<void>> future(new joynr::Future<void>());
			std::function<void()> onSuccess =
					[future, callbackFct] () {
						future->onSuccess(joynr::RequestStatusCode::OK);
						if (callbackFct) {
							callbackFct(joynr::RequestStatusCode::OK);
						}
					};

			//see header for more information
			LOG_ERROR(logger,"#### WARNING ##### «interfaceName»InProcessConnector::«setAttributeName»(Future) is synchronous.");
			«serviceInterface.interfaceCaller»->«setAttributeName»(input, onSuccess);
			return future;
		}

		void «interfaceName»InProcessConnector::«setAttributeName»(
				joynr::RequestStatus& status,
				const «returnType»& input
		) {
			assert(!address.isNull());
			QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(!caller.isNull());
			QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			assert(!«serviceInterface.interfaceCaller».isNull());

			QSharedPointer<joynr::Future<void>> future(new joynr::Future<void>());
			std::function<void()> onSuccess =
					[future] () {
						future->onSuccess(joynr::RequestStatusCode::OK);
					};

			//see header for more information
			«serviceInterface.interfaceCaller»->«setAttributeName»(input, onSuccess);
			status = future->waitForFinished();
		}

	«ENDIF»
	«IF attribute.notifiable»
		std::string «interfaceName»InProcessConnector::subscribeTo«attributeName.toFirstUpper»(
				std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
				const joynr::StdSubscriptionQos& subscriptionQos,
				std::string& subscriptionId)
		{
			joynr::SubscriptionRequest subscriptionRequest;
			subscriptionRequest.setSubscriptionId(QString::fromStdString(subscriptionId));
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		std::string «interfaceName»InProcessConnector::subscribeTo«attributeName.toFirstUpper»(
				std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
				const joynr::StdSubscriptionQos& subscriptionQos)
		{
			joynr::SubscriptionRequest subscriptionRequest;
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		std::string «interfaceName»InProcessConnector::subscribeTo«attributeName.toFirstUpper»(
				std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
				const joynr::StdSubscriptionQos& subscriptionQos,
				joynr::SubscriptionRequest& subscriptionRequest)
		{
			«IF isEnum(attribute.type)»
				Q_UNUSED(subscriptionListener);
				Q_UNUSED(subscriptionQos);
				// TODO support enum return values in C++ client
				LOG_FATAL(logger, "enum return values are currently not supported in C++ client (attribute name: «interfaceName».«attributeName»)");
				assert(false);
				// Visual C++ requires a return value
				return std::string();
			«ELSE»
				LOG_DEBUG(logger, "Subscribing to «attributeName».");
				assert(subscriptionManager != NULL);
				QString attributeName("«attributeName»");
				QSharedPointer<joynr::SubscriptionCallback<«returnTypeQt»>> subscriptionCallback(
						«IF qtTypeUtil.needsDatatypeConversion(attribute)»
							new «attribute.joynrName.toFirstUpper»AttributeSubscriptionCallbackWrapper(
						«ELSE»
							new joynr::SubscriptionCallback<«returnTypeQt»>(
						«ENDIF»
								subscriptionListener
						)
				);
				subscriptionManager->registerSubscription(
						attributeName,
						subscriptionCallback,
						QSharedPointer<SubscriptionQos>(SubscriptionQos::createQt(subscriptionQos)),
						subscriptionRequest);
				LOG_DEBUG(logger, "Registered subscription: " + subscriptionRequest.toQString());
				assert(!address.isNull());
				QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«interfaceName»RequestCaller> requestCaller = caller.dynamicCast<«interfaceName»RequestCaller>();
				std::string subscriptionId(subscriptionRequest.getSubscriptionId().toStdString());

				if(caller.isNull()) {
					assert(publicationManager != NULL);
					/**
					* Provider not registered yet
					* Dispatcher will call publicationManger->restore when a new provider is added to activate
					* subscriptions for that provider
					*/
					publicationManager->add(QString::fromStdString(proxyParticipantId), QString::fromStdString(providerParticipantId), subscriptionRequest);
				} else {
					publicationManager->add(QString::fromStdString(proxyParticipantId), QString::fromStdString(providerParticipantId), caller, subscriptionRequest, inProcessPublicationSender);
				}
				return subscriptionId;
			«ENDIF»
		}

		void «interfaceName»InProcessConnector::unsubscribeFrom«attributeName.toFirstUpper»(
				std::string& subscriptionId
		) {
			«IF isEnum(attribute.type)»
				Q_UNUSED(subscriptionId);
				// TODO support enum return values in C++ client
				LOG_FATAL(logger, "enum return values are currently not supported in C++ client (attribute name: «interfaceName».«attributeName»)");
				assert(false);
			«ELSE»
				QString subscriptionIdQT(QString::fromStdString(subscriptionId));
				LOG_DEBUG(logger, "Unsubscribing. Id=" +subscriptionIdQT);
				assert(publicationManager != NULL);
				LOG_DEBUG(logger, "Stopping publications by publication manager.");
				publicationManager->stopPublication(subscriptionIdQT);
				assert(subscriptionManager != NULL);
				LOG_DEBUG(logger, "Unregistering attribute subscription.");
				subscriptionManager->unregisterSubscription(subscriptionIdQT);
			«ENDIF»
		}

	«ENDIF»
«ENDFOR»

«FOR method: getMethods(serviceInterface)»
«var methodname = method.joynrName»
«var parameterList = prependCommaIfNotEmpty(cppStdTypeUtil.getCommaSeperatedTypedConstInputParameterList(method))»
«var outputParameters = cppStdTypeUtil.getCommaSeparatedOutputParameterTypes(method)»
«var inputParamList = cppStdTypeUtil.getCommaSeperatedUntypedInputParameterList(method)»
«var outputTypedParamListStd = cppStdTypeUtil.getCommaSeperatedTypedConstOutputParameterList(method)»
«var outputUntypedParamList = prependCommaIfNotEmpty(cppStdTypeUtil.getCommaSeperatedUntypedOutputParameterList(method))»

void «interfaceName»InProcessConnector::«methodname»(
			joynr::RequestStatus& status«prependCommaIfNotEmpty(cppStdTypeUtil.getCommaSeperatedTypedOutputParameterList(method))»«parameterList»
) {
	assert(!address.isNull());
	QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
	assert(!caller.isNull());
	QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
	assert(!«serviceInterface.interfaceCaller».isNull());
	QSharedPointer<joynr::Future<«outputParameters»> > future(
			new joynr::Future<«outputParameters»>());

	std::function<void(«outputTypedParamListStd»)> onSuccess =
			[future] («outputTypedParamListStd») {
				future->onSuccess(
						joynr::RequestStatusCode::OK«outputUntypedParamList»
				);
			};

	«serviceInterface.interfaceCaller»->«methodname»(«IF !method.inputParameters.empty»«inputParamList», «ENDIF»onSuccess);
	status = future->waitForFinished();
	«IF !method.outputParameters.empty»
		if (status.successful()) {
			future->getValues(«cppStdTypeUtil.getCommaSeperatedUntypedOutputParameterList(method)»);
		}
	«ENDIF»
}

std::shared_ptr<joynr::Future<«outputParameters»> > «interfaceName»InProcessConnector::«methodname»(«cppStdTypeUtil.getCommaSeperatedTypedConstInputParameterList(method)»«IF !method.inputParameters.empty»,«ENDIF»
			std::function<void(const joynr::RequestStatus& status«outputTypedParamListStd.prependCommaIfNotEmpty»)> callbackFct)
{
	assert(!address.isNull());
	QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
	assert(!caller.isNull());
	QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
	assert(!«serviceInterface.interfaceCaller».isNull());
	std::shared_ptr<joynr::Future<«outputParameters»> > future(
			new joynr::Future<«outputParameters»>());

	std::function<void(«outputTypedParamListStd»)> onSuccess =
			[future, callbackFct] («outputTypedParamListStd») {
				future->onSuccess(joynr::RequestStatusCode::OK«outputUntypedParamList»);
				if (callbackFct)
				{
					callbackFct(joynr::RequestStatusCode::OK«outputUntypedParamList»);
				}
			};
	«serviceInterface.interfaceCaller»->«methodname»(«IF !method.inputParameters.empty»«inputParamList», «ENDIF»onSuccess);
	return future;
}

«ENDFOR»

«FOR broadcast: serviceInterface.broadcasts»
	«val returnTypes = cppStdTypeUtil.getCommaSeparatedOutputParameterTypes(broadcast)»
	«val returnTypesQt = qtTypeUtil.getCommaSeparatedOutputParameterTypes(broadcast)»
	«val broadcastName = broadcast.joynrName»

	«IF isSelective(broadcast)»
		std::string «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				const «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
				std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				const joynr::StdOnChangeSubscriptionQos& subscriptionQos
	«ELSE»
		std::string «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				const joynr::StdOnChangeSubscriptionQos& subscriptionQos
	«ENDIF»
	) {
		LOG_DEBUG(logger, "Subscribing to «broadcastName».");
		assert(subscriptionManager != NULL);
		QString broadcastName("«broadcastName»");
		joynr::BroadcastSubscriptionRequest subscriptionRequest;
		«IF isSelective(broadcast)»
			subscriptionRequest.setFilterParameters(BroadcastFilterParameters::createQt(filterParameters));
		«ENDIF»
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(
					subscriptionListener,
					subscriptionQos,
					subscriptionRequest);
	}

	«IF isSelective(broadcast)»
		std::string «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				const «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
				std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				const joynr::StdOnChangeSubscriptionQos& subscriptionQos,
				std::string& subscriptionId
	«ELSE»
		std::string «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				const joynr::StdOnChangeSubscriptionQos& subscriptionQos,
				std::string& subscriptionId
	«ENDIF»
	) {
		joynr::BroadcastSubscriptionRequest subscriptionRequest;
		«IF isSelective(broadcast)»
			subscriptionRequest.setFilterParameters(BroadcastFilterParameters::createQt(filterParameters));
		«ENDIF»
		subscriptionRequest.setSubscriptionId(QString::fromStdString(subscriptionId));
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(
					subscriptionListener,
					subscriptionQos,
					subscriptionRequest);
	}

	std::string «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
			std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
			const joynr::StdOnChangeSubscriptionQos& subscriptionQos,
			joynr::BroadcastSubscriptionRequest& subscriptionRequest
	) {
		LOG_DEBUG(logger, "Subscribing to «broadcastName».");
		assert(subscriptionManager != NULL);
		QString broadcastName("«broadcastName»");

		QSharedPointer<joynr::SubscriptionCallback<«returnTypesQt»>> subscriptionCallback(
					new «broadcastName.toFirstUpper»BroadcastSubscriptionCallbackWrapper(subscriptionListener));
		subscriptionManager->registerSubscription(
					broadcastName,
					subscriptionCallback,
					QSharedPointer<OnChangeSubscriptionQos>(SubscriptionQos::createQt(subscriptionQos)),
					subscriptionRequest);
		LOG_DEBUG(logger, "Registered broadcast subscription: " + subscriptionRequest.toQString());
		assert(!address.isNull());
		QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
		assert(!caller.isNull());
		QSharedPointer<«interfaceName»RequestCaller> requestCaller = caller.dynamicCast<«interfaceName»RequestCaller>();
		std::string subscriptionId(subscriptionRequest.getSubscriptionId().toStdString());

		if(caller.isNull()) {
			assert(publicationManager != NULL);
			/**
			* Provider not registered yet
			* Dispatcher will call publicationManger->restore when a new provider is added to activate
			* subscriptions for that provider
			*/
			publicationManager->add(QString::fromStdString(proxyParticipantId), QString::fromStdString(providerParticipantId), subscriptionRequest);
		} else {
			publicationManager->add(
						QString::fromStdString(proxyParticipantId),
						QString::fromStdString(providerParticipantId),
						caller,
						subscriptionRequest,
						inProcessPublicationSender);
		}
		return subscriptionId;
	}

	void «interfaceName»InProcessConnector::unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(
			std::string& subscriptionId
	) {
		QString subscriptionIdQT(QString::fromStdString(subscriptionId));
		LOG_DEBUG(logger, "Unsubscribing broadcast. Id=" + subscriptionIdQT);
		assert(publicationManager != NULL);
		LOG_DEBUG(logger, "Stopping publications by publication manager.");
		publicationManager->stopPublication(subscriptionIdQT);
		assert(subscriptionManager != NULL);
		LOG_DEBUG(logger, "Unregistering broadcast subscription.");
		subscriptionManager->unregisterSubscription(subscriptionIdQT);
	}
«ENDFOR»
«getNamespaceEnder(serviceInterface)»
'''

	def getInterfaceCaller(FInterface serviceInterface){
		serviceInterface.joynrName.toFirstLower + "Caller"
	}
}
