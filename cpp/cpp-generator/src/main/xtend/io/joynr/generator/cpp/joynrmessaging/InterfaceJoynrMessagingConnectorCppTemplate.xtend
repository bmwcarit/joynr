package io.joynr.generator.cpp.joynrmessaging
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
import io.joynr.generator.cpp.util.DatatypeSystemTransformation
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.QtTypeUtil
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.util.InterfaceTemplate
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType

class InterfaceJoynrMessagingConnectorCppTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension QtTypeUtil qtTypeUtil

	@Inject
	private CppStdTypeUtil cppStdTypeUtil
	
	@Inject
	private extension JoynrCppGeneratorExtensions

	def produceParameterSetters(FMethod method)
'''
joynr::Request internalRequestObject;
internalRequestObject.setMethodName(QString("«method.joynrName»"));
«FOR param : getInputParameters(method)»
	«val paramRef = qtTypeUtil.fromStdTypeToQTType(param, param.joynrName, true)»
	«IF isEnum(param.type) && isArray(param)»
		internalRequestObject.addParam(joynr::Util::convertListToVariantList(«paramRef»), "«getJoynrTypeName(param)»");
	«ELSEIF isEnum(param.type)»
		internalRequestObject.addParam(QVariant::fromValue(«paramRef»), "«getJoynrTypeName(param)»");
	«ELSEIF isArray(param)»
		QList<QVariant> «param.joynrName»QVarList = joynr::Util::convertListToVariantList(«paramRef»);
		internalRequestObject.addParam(QVariant::fromValue(«param.joynrName»QVarList), "«getJoynrTypeName(param)»");
	«ELSEIF isComplex(param.type)»
		internalRequestObject.addParam(QVariant::fromValue(«paramRef»), "«getJoynrTypeName(param)»");
	«ELSE»
		internalRequestObject.addParam(QVariant(«paramRef»), "«getJoynrTypeName(param)»");
	«ENDIF»
«ENDFOR»
'''

	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«warning()»

#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»JoynrMessagingConnector.h"
#include "joynr/ReplyCaller.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/joynrlogging.h"
#include "joynr/ISubscriptionManager.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/Util.h"
#include "joynr/TypeUtil.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/Future.h"

«FOR datatype: getAllComplexAndEnumTypes(serviceInterface)»
«IF datatype instanceof FType»
	«IF isComplex(datatype)»
		#include "«getIncludeOf(datatype)»"
	«ENDIF»
«ENDIF»
«ENDFOR»

«getNamespaceStarter(serviceInterface)»

«interfaceName»JoynrMessagingConnector::«interfaceName»JoynrMessagingConnector(
		joynr::IJoynrMessageSender* joynrMessageSender,
		joynr::ISubscriptionManager* subscriptionManager,
		const std::string &domain,
		const std::string proxyParticipantId,
		const std::string& providerParticipantId,
		const joynr::MessagingQos &qosSettings,
		joynr::IClientCache *cache,
		bool cached)
	: joynr::AbstractJoynrMessagingConnector(joynrMessageSender, subscriptionManager, domain, INTERFACE_NAME(), proxyParticipantId, providerParticipantId, qosSettings, cache, cached)
{
}

bool «interfaceName»JoynrMessagingConnector::usesClusterController() const{
	return joynr::AbstractJoynrMessagingConnector::usesClusterController();
}

«FOR attribute: getAttributes(serviceInterface)»
	«val returnTypeStd = cppStdTypeUtil.getTypeName(attribute)»
	«val returnTypeQT = qtTypeUtil.getTypeName(attribute)»
	«val attributeName = attribute.joynrName»
	«IF attribute.readable»
		void «interfaceName»JoynrMessagingConnector::get«attributeName.toFirstUpper»(
				joynr::RequestStatus& status,
				«returnTypeStd»& «attributeName»
		) {
			std::shared_ptr<joynr::Future<«returnTypeStd»> > future(new joynr::Future<«returnTypeStd»>());

			std::function<void(const joynr::RequestStatus& status, const «returnTypeQT»& «attributeName»)> replyCallerCallbackFct =
					[future] (const joynr::RequestStatus& status, const «returnTypeQT»& «attributeName») {
						if (status.getCode() == joynr::RequestStatusCode::OK) {
							future->onSuccess(status, «qtTypeUtil.fromQTTypeToStdType(attribute, attribute.joynrName)»);
						} else {
							future->onFailure(status);
						}
					};

			std::function<void(const joynr::RequestStatus& status)> replyCallerErrorFct =
					[future] (const joynr::RequestStatus& status) {
						future->onFailure(status);
					};

			QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«returnTypeQT»>(
					replyCallerCallbackFct,
					replyCallerErrorFct));
			attributeRequest<«returnTypeQT»>(QString("get«attributeName.toFirstUpper»"), replyCaller);
			status = future->waitForFinished();
			if (status.successful()) {
				future->getValues(«attributeName»);
				// add result to caching
			}
		}

		std::shared_ptr<joynr::Future<«returnTypeStd»>> «interfaceName»JoynrMessagingConnector::get«attributeName.toFirstUpper»(
				std::function<void(const joynr::RequestStatus& status, const «returnTypeStd»& «attributeName»)> callbackFct
		) {
			std::shared_ptr<joynr::Future<«returnTypeStd»> > future(new joynr::Future<«returnTypeStd»>());

			std::function<void(const joynr::RequestStatus& status, const «returnTypeQT»& «attributeName»)> replyCallerCallbackFct =
					[future, callbackFct] (const joynr::RequestStatus& status, const «returnTypeQT»& «attributeName») {
						if (status.getCode() == joynr::RequestStatusCode::OK) {
							future->onSuccess(status, «qtTypeUtil.fromQTTypeToStdType(attribute, attribute.joynrName)»);
						} else {
							future->onFailure(status);
						}
						if (callbackFct){
							callbackFct(status, «qtTypeUtil.fromQTTypeToStdType(attribute, attribute.joynrName)»);
						}
					};

			std::function<void(const joynr::RequestStatus& status)> replyCallerErrorFct =
					[future] (const joynr::RequestStatus& status) {
						future->onFailure(status);
					};

			QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«returnTypeQT»>(
					replyCallerCallbackFct,
					replyCallerErrorFct));
			attributeRequest<«returnTypeQT»>(QString("get«attributeName.toFirstUpper»"), replyCaller);

			return future;
		}

	«ENDIF»
	«IF attribute.writable»
		std::shared_ptr<joynr::Future<void>> «interfaceName»JoynrMessagingConnector::set«attributeName.toFirstUpper»(
				«returnTypeStd» «attributeName»,
				std::function<void(const joynr::RequestStatus& status)> callbackFct
		) {
			joynr::Request internalRequestObject;
			internalRequestObject.setMethodName(QString("set«attributeName.toFirstUpper»"));
			«IF isArray(attribute)»
				QList<QVariant> «attributeName»QVarList = joynr::Util::convertListToVariantList(«qtTypeUtil.fromStdTypeToQTType(attribute, attributeName)»);
				internalRequestObject.addParam(QVariant::fromValue(«attributeName»QVarList), "«getJoynrTypeName(attribute)»");
			«ELSE»
				internalRequestObject.addParam(QVariant::fromValue(«qtTypeUtil.fromStdTypeToQTType(attribute, attributeName)»), "«getJoynrTypeName(attribute)»");
			«ENDIF»

			std::shared_ptr<joynr::Future<void>> future(new joynr::Future<void>());

			std::function<void(const joynr::RequestStatus& status)> replyCallerCallbackFct =
					[future, callbackFct] (const joynr::RequestStatus& status) {
						if (status.getCode() == joynr::RequestStatusCode::OK) {
							future->onSuccess(status);
						} else {
							future->onFailure(status);
						}
						if (callbackFct){
							callbackFct(status);
						}
					};

			QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<void>(
					replyCallerCallbackFct,
					std::bind(&joynr::Future<void>::onFailure, future, std::placeholders::_1)));
			operationRequest(replyCaller, internalRequestObject);
			return future;
		}

		void «interfaceName»JoynrMessagingConnector::set«attributeName.toFirstUpper»(
				joynr::RequestStatus& status,
				const «returnTypeStd»& «attributeName»
		) {
			joynr::Request internalRequestObject;
			internalRequestObject.setMethodName(QString("set«attributeName.toFirstUpper»"));
			«IF isArray(attribute)»
				QList<QVariant> «attributeName»QVarList = joynr::Util::convertListToVariantList(«qtTypeUtil.fromStdTypeToQTType(attribute, attributeName)»);
				internalRequestObject.addParam(QVariant::fromValue(«attributeName»QVarList), "«getJoynrTypeName(attribute)»");
			«ELSE»
				internalRequestObject.addParam(QVariant::fromValue(«qtTypeUtil.fromStdTypeToQTType(attribute, attributeName)»), "«getJoynrTypeName(attribute)»");
			«ENDIF»

			QSharedPointer<joynr::Future<void> > future( new joynr::Future<void>());

			std::function<void(const joynr::RequestStatus& status)> replyCallerCallbackFct =
					[future] (const joynr::RequestStatus& status) {
						if (status.getCode() == joynr::RequestStatusCode::OK) {
							future->onSuccess(status);
						} else {
							future->onFailure(status);
						}
					};

			QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<void>(
					replyCallerCallbackFct,
					std::bind(&joynr::Future<void>::onFailure, future, std::placeholders::_1)));
			operationRequest(replyCaller, internalRequestObject);
			status = future->waitForFinished();
		}

	«ENDIF»
	«IF attribute.notifiable»
		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«attributeName.toFirstUpper»(
					QSharedPointer<joynr::ISubscriptionListener<«returnTypeStd» > > subscriptionListener,
					QSharedPointer<joynr::StdSubscriptionQos> subscriptionQos
		) {
			joynr::SubscriptionRequest subscriptionRequest;
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«attributeName.toFirstUpper»(
					QSharedPointer<joynr::ISubscriptionListener<«returnTypeStd» > > subscriptionListener,
					QSharedPointer<joynr::StdSubscriptionQos> subscriptionQos,
					std::string& subscriptionId
		) {

			joynr::SubscriptionRequest subscriptionRequest;
			subscriptionRequest.setSubscriptionId(QString::fromStdString(subscriptionId));
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«attributeName.toFirstUpper»(
					QSharedPointer<joynr::ISubscriptionListener<«returnTypeStd»> > subscriptionListener,
					QSharedPointer<joynr::StdSubscriptionQos> subscriptionQos,
					SubscriptionRequest& subscriptionRequest
		) {
			LOG_DEBUG(logger, "Subscribing to «attributeName».");
			QString attributeName("«attributeName»");
			joynr::MessagingQos clonedMessagingQos(qosSettings);
			if (subscriptionQos->getExpiryDate() == joynr::StdSubscriptionQos::NO_EXPIRY_DATE()) {
				clonedMessagingQos.setTtl(joynr::StdSubscriptionQos::NO_EXPIRY_DATE_TTL());
			}
			else{
				clonedMessagingQos.setTtl(subscriptionQos->getExpiryDate() - QDateTime::currentMSecsSinceEpoch());
			}

			«val subscriptionListenerName = if (needsDatatypeConversion(attribute)) "subscriptionListenerWrapper" else "subscriptionListener"»
			«IF needsDatatypeConversion(attribute)»
				QSharedPointer<«attribute.joynrName.toFirstUpper»AttributeSubscriptionListenerWrapper> subscriptionListenerWrapper(
					new «attribute.joynrName.toFirstUpper»AttributeSubscriptionListenerWrapper(subscriptionListener));
			«ENDIF»
			QSharedPointer<joynr::SubscriptionCallback<«returnTypeQT»>> subscriptionCallback(new joynr::SubscriptionCallback<«returnTypeQT»>(«subscriptionListenerName»));
			subscriptionManager->registerSubscription(
						attributeName,
						subscriptionCallback,
						QSharedPointer<SubscriptionQos>(SubscriptionQos::createQt(*subscriptionQos)),
						subscriptionRequest);
			LOG_DEBUG(logger, subscriptionRequest.toQString());
			joynrMessageSender->sendSubscriptionRequest(
						proxyParticipantId,
						providerParticipantId,
						clonedMessagingQos,
						subscriptionRequest
			);
			return subscriptionRequest.getSubscriptionId().toStdString();
		}

		void «interfaceName»JoynrMessagingConnector::unsubscribeFrom«attributeName.toFirstUpper»(
				std::string& subscriptionId
		) {
			joynr::SubscriptionStop subscriptionStop;
			subscriptionStop.setSubscriptionId(QString::fromStdString(subscriptionId));

			subscriptionManager->unregisterSubscription(QString::fromStdString(subscriptionId));
			joynrMessageSender->sendSubscriptionStop(
						proxyParticipantId,
						providerParticipantId,
						qosSettings,
						subscriptionStop
			);
		}

	«ENDIF»
«ENDFOR»

«FOR method: getMethods(serviceInterface)»
	«var outputTypedConstParamListQT = prependCommaIfNotEmpty(qtTypeUtil.getCommaSeperatedTypedConstOutputParameterList(method))»
	«var outputTypedConstParamListStd = prependCommaIfNotEmpty(cppStdTypeUtil.getCommaSeperatedTypedConstOutputParameterList(method))»
	«val outputTypedParamListStd = prependCommaIfNotEmpty(cppStdTypeUtil.getCommaSeperatedTypedOutputParameterList(method))»
	«val outputParametersStd = cppStdTypeUtil.getCommaSeparatedOutputParameterTypes(method)»
	«val outputParametersQT = qtTypeUtil.getCommaSeparatedOutputParameterTypes(method)»
	«var outputUntypedParamList = prependCommaIfNotEmpty(qtTypeUtil.getCommaSeperatedUntypedOutputParameterList(method, DatatypeSystemTransformation::FROM_QT_TO_STANDARD))»
	«val inputTypedParamListStd = prependCommaIfNotEmpty(cppStdTypeUtil.getCommaSeperatedTypedConstInputParameterList(method))»
	«val methodName = method.joynrName»
	void «interfaceName»JoynrMessagingConnector::«methodName»(
			joynr::RequestStatus& status«outputTypedParamListStd»«inputTypedParamListStd»
	) {
		«produceParameterSetters(method)»
		QSharedPointer<joynr::Future<«outputParametersStd»> > future(
				new joynr::Future<«outputParametersStd»>());

		std::function<void(const joynr::RequestStatus& status«outputTypedConstParamListQT»)> replyCallerCallbackFct =
				[future] (const joynr::RequestStatus& status«outputTypedConstParamListQT») {
					if (status.getCode() == joynr::RequestStatusCode::OK) {
						future->onSuccess(status«outputUntypedParamList»);
					} else {
						future->onFailure(status);
					}
				};

		std::function<void(const joynr::RequestStatus& status)> replyCallerErrorFct =
			[future] (const joynr::RequestStatus& status) {
				future->onFailure(status);
			};

		QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«outputParametersQT»>(
				replyCallerCallbackFct,
				replyCallerErrorFct));
		operationRequest(replyCaller, internalRequestObject);
		status = future->waitForFinished();
		«IF !method.outputParameters.empty»
			if (status.successful()) {
				future->getValues(«cppStdTypeUtil.getCommaSeperatedUntypedOutputParameterList(method)»);
			}
		«ENDIF»
	}

	std::shared_ptr<joynr::Future<«outputParametersStd»> > «interfaceName»JoynrMessagingConnector::«methodName»(
			«cppStdTypeUtil.getCommaSeperatedTypedConstInputParameterList(method)»«IF !method.inputParameters.empty»,«ENDIF»
			std::function<void(const joynr::RequestStatus& status«outputTypedConstParamListStd»)> callbackFct)
	{
		«produceParameterSetters(method)»

		std::shared_ptr<joynr::Future<«outputParametersStd»> > future(
				new joynr::Future<«outputParametersStd»>());

		std::function<void(const joynr::RequestStatus& status«outputTypedConstParamListQT»)> replyCallerCallbackFct =
				[future, callbackFct] (const joynr::RequestStatus& status«outputTypedConstParamListQT») {
					if (status.getCode() == joynr::RequestStatusCode::OK) {
						future->onSuccess(status«outputUntypedParamList»);
					} else {
						future->onFailure(status);
					}
					if (callbackFct)
					{
						callbackFct(status«outputUntypedParamList»);
					}
				};

		std::function<void(const joynr::RequestStatus& status)> replyCallerErrorFct =
				[future] (const joynr::RequestStatus& status) {
					future->onFailure(status);
				};

		QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«outputParametersQT»>(
				replyCallerCallbackFct,
				replyCallerErrorFct));
		operationRequest(replyCaller, internalRequestObject);
		return future;
	}

«ENDFOR»

«FOR broadcast: serviceInterface.broadcasts»
	«val returnTypes = cppStdTypeUtil.getCommaSeparatedOutputParameterTypes(broadcast)»
	«val returnTypesQt = qtTypeUtil.getCommaSeparatedOutputParameterTypes(broadcast)»
	«val broadcastName = broadcast.joynrName»
	«IF isSelective(broadcast)»
		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					«interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters,
					QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					QSharedPointer<joynr::StdOnChangeSubscriptionQos> subscriptionQos
	«ELSE»
		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					QSharedPointer<joynr::StdOnChangeSubscriptionQos> subscriptionQos
	«ENDIF»
	) {
		joynr::BroadcastSubscriptionRequest subscriptionRequest;
		«IF isSelective(broadcast)»
			subscriptionRequest.setFilterParameters(BroadcastFilterParameters::createQt(filterParameters));
		«ENDIF»
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(subscriptionListener, subscriptionQos, subscriptionRequest);
	}

	«IF isSelective(broadcast)»
		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					«interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters,
					QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					QSharedPointer<joynr::StdOnChangeSubscriptionQos> subscriptionQos,
					std::string& subscriptionId
	«ELSE»
		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					QSharedPointer<joynr::StdOnChangeSubscriptionQos> subscriptionQos,
					std::string& subscriptionId
	«ENDIF»
	) {
		joynr::BroadcastSubscriptionRequest subscriptionRequest;
		«IF isSelective(broadcast)»
			subscriptionRequest.setFilterParameters(BroadcastFilterParameters::createQt(filterParameters));
		«ENDIF»
		subscriptionRequest.setSubscriptionId(QString::fromStdString(subscriptionId));
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(subscriptionListener, subscriptionQos, subscriptionRequest);
	}

	std::string «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				QSharedPointer<joynr::StdOnChangeSubscriptionQos> subscriptionQos,
				BroadcastSubscriptionRequest& subscriptionRequest
	) {
		LOG_DEBUG(logger, "Subscribing to «broadcastName» broadcast.");
		QString broadcastName("«broadcastName»");
		joynr::MessagingQos clonedMessagingQos(qosSettings);
		if (subscriptionQos->getExpiryDate() == joynr::StdSubscriptionQos::NO_EXPIRY_DATE()) {
			clonedMessagingQos.setTtl(joynr::StdSubscriptionQos::NO_EXPIRY_DATE_TTL());
		}
		else{
			clonedMessagingQos.setTtl(subscriptionQos->getExpiryDate() - QDateTime::currentMSecsSinceEpoch());
		}

		«val subscriptionListenerName = if (needsDatatypeConversion(broadcast)) "subscriptionListenerWrapper" else "subscriptionListener"»
		«IF needsDatatypeConversion(broadcast)»
			QSharedPointer<«broadcast.joynrName.toFirstUpper»BroadcastSubscriptionListenerWrapper> subscriptionListenerWrapper(
				new «broadcast.joynrName.toFirstUpper»BroadcastSubscriptionListenerWrapper(subscriptionListener));
		«ENDIF»
		QSharedPointer<joynr::SubscriptionCallback<«returnTypesQt»>> subscriptionCallback(
					new joynr::SubscriptionCallback<«returnTypesQt»>(«subscriptionListenerName»));
		subscriptionManager->registerSubscription(
					broadcastName,
					subscriptionCallback,
					QSharedPointer<OnChangeSubscriptionQos>(SubscriptionQos::createQt(*subscriptionQos)),
					subscriptionRequest);
		LOG_DEBUG(logger, subscriptionRequest.toQString());
		joynrMessageSender->sendBroadcastSubscriptionRequest(
					proxyParticipantId,
					providerParticipantId,
					clonedMessagingQos,
					subscriptionRequest
		);
		return subscriptionRequest.getSubscriptionId().toStdString();
	}

	void «interfaceName»JoynrMessagingConnector::unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(
			std::string& subscriptionId
	) {
		joynr::SubscriptionStop subscriptionStop;
		subscriptionStop.setSubscriptionId(QString::fromStdString(subscriptionId));

		subscriptionManager->unregisterSubscription(QString::fromStdString(subscriptionId));
		joynrMessageSender->sendSubscriptionStop(
					proxyParticipantId,
					providerParticipantId,
					qosSettings,
					subscriptionStop
		);
	}

«ENDFOR»
«getNamespaceEnder(serviceInterface)»
'''
}

