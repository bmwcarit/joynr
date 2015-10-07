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
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType

class InterfaceJoynrMessagingConnectorCppTemplate implements InterfaceTemplate{

	@Inject private extension TemplateBase
	@Inject private extension QtTypeUtil qtTypeUtil
	@Inject private CppStdTypeUtil cppStdTypeUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension MethodUtil
	@Inject private extension BroadcastUtil
	@Inject private extension InterfaceUtil

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
#include "joynr/RequestStatus.h"
#include "joynr/RequestStatusCode.h"

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
		joynr::RequestStatus «interfaceName»JoynrMessagingConnector::get«attributeName.toFirstUpper»(
				«returnTypeStd»& «attributeName»
		) {
			std::shared_ptr<joynr::Future<«returnTypeStd»> > future(new joynr::Future<«returnTypeStd»>());

			std::function<void(const joynr::RequestStatus& status, const «returnTypeQT»& «attributeName»)> onSuccess =
					[future] (const joynr::RequestStatus& status, const «returnTypeQT»& «attributeName») {
						if (status.getCode() == joynr::RequestStatusCode::OK) {
							future->onSuccess(«qtTypeUtil.fromQTTypeToStdType(attribute, attribute.joynrName)»);
						} else {
							future->onError(status);
						}
					};

			std::function<void(const joynr::RequestStatus& status)> onError =
					[future] (const joynr::RequestStatus& status) {
						future->onError(status);
					};

			std::shared_ptr<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«returnTypeQT»>(
					onSuccess,
					onError));
			attributeRequest<«returnTypeQT»>(QString("get«attributeName.toFirstUpper»"), replyCaller);
			joynr::RequestStatus status(future->waitForFinished());
			if (status.successful()) {
				future->getValues(«attributeName»);
			}
			return status;
		}

		std::shared_ptr<joynr::Future<«returnTypeStd»>> «interfaceName»JoynrMessagingConnector::get«attributeName.toFirstUpper»Async(
				std::function<void(const «returnTypeStd»& «attributeName»)> onSuccess,
				std::function<void(const joynr::RequestStatus& status)> onError
		) {
			std::shared_ptr<joynr::Future<«returnTypeStd»> > future(new joynr::Future<«returnTypeStd»>());

			std::function<void(const joynr::RequestStatus& status, const «returnTypeQT»& «attributeName»)> onSuccessWrapper =
					[future, onSuccess, onError] (const joynr::RequestStatus& status, const «returnTypeQT»& «attributeName») {
						if (status.getCode() == joynr::RequestStatusCode::OK) {
							future->onSuccess(«qtTypeUtil.fromQTTypeToStdType(attribute, attribute.joynrName)»);
							if (onSuccess){
								onSuccess(«qtTypeUtil.fromQTTypeToStdType(attribute, attribute.joynrName)»);
							}
						} else {
							future->onError(status);
							if (onError){
								onError(status);
							}
						}
					};

			std::function<void(const joynr::RequestStatus& status)> onErrorWrapper =
					[future, onError] (const joynr::RequestStatus& status) {
						future->onError(status);
						if (onError){
							onError(status);
						}
					};

			std::shared_ptr<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«returnTypeQT»>(
					onSuccessWrapper,
					onErrorWrapper));
			attributeRequest<«returnTypeQT»>(QString("get«attributeName.toFirstUpper»"), replyCaller);

			return future;
		}

	«ENDIF»
	«IF attribute.writable»
		std::shared_ptr<joynr::Future<void>> «interfaceName»JoynrMessagingConnector::set«attributeName.toFirstUpper»Async(
				«returnTypeStd» «attributeName»,
				std::function<void(void)> onSuccess,
				std::function<void(const joynr::RequestStatus& status)> onError
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

			std::function<void(const joynr::RequestStatus& status)> onSuccessWrapper =
					[future, onSuccess, onError] (const joynr::RequestStatus& status) {
						if (status.getCode() == joynr::RequestStatusCode::OK) {
							future->onSuccess();
							if (onSuccess) {
								onSuccess();
							}
						} else {
							future->onError(status);
							if (onError){
								onError(status);
							}
						}
					};

			std::function<void(const joynr::RequestStatus& status)> onErrorWrapper =
				[future, onError] (const joynr::RequestStatus& status) {
					future->onError(status);
					if (onError) {
						onError(status);
					}
				};

			std::shared_ptr<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<void>(
					onSuccessWrapper,
					onErrorWrapper));
			operationRequest(replyCaller, internalRequestObject);
			return future;
		}

		joynr::RequestStatus «interfaceName»JoynrMessagingConnector::set«attributeName.toFirstUpper»(
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

			std::shared_ptr<joynr::Future<void> > future( new joynr::Future<void>());

			std::function<void(const joynr::RequestStatus& status)> onSuccess =
					[future] (const joynr::RequestStatus& status) {
						if (status.getCode() == joynr::RequestStatusCode::OK) {
							future->onSuccess();
						} else {
							future->onError(status);
						}
					};

			std::shared_ptr<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<void>(
					onSuccess,
					std::bind(&joynr::Future<void>::onError, future, std::placeholders::_1)));
			operationRequest(replyCaller, internalRequestObject);
			return future->waitForFinished();
		}

	«ENDIF»
	«IF attribute.notifiable»
		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«attributeName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnTypeStd» > > subscriptionListener,
					const joynr::SubscriptionQos& subscriptionQos
		) {
			joynr::SubscriptionRequest subscriptionRequest;
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«attributeName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnTypeStd» > > subscriptionListener,
					const joynr::SubscriptionQos& subscriptionQos,
					std::string& subscriptionId
		) {

			joynr::SubscriptionRequest subscriptionRequest;
			subscriptionRequest.setSubscriptionId(QString::fromStdString(subscriptionId));
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«attributeName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnTypeStd»> > subscriptionListener,
					const joynr::SubscriptionQos& subscriptionQos,
					SubscriptionRequest& subscriptionRequest
		) {
			LOG_DEBUG(logger, "Subscribing to «attributeName».");
			QString attributeName("«attributeName»");
			joynr::MessagingQos clonedMessagingQos(qosSettings);
			if (subscriptionQos.getExpiryDate() == joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
				clonedMessagingQos.setTtl(joynr::SubscriptionQos::NO_EXPIRY_DATE_TTL());
			}
			else{
				clonedMessagingQos.setTtl(subscriptionQos.getExpiryDate() - QDateTime::currentMSecsSinceEpoch());
			}

			«val subscriptionListenerName = if (needsDatatypeConversion(attribute)) "subscriptionListenerWrapper" else "subscriptionListener"»
			«IF needsDatatypeConversion(attribute)»
				std::shared_ptr<«attribute.joynrName.toFirstUpper»AttributeSubscriptionListenerWrapper> subscriptionListenerWrapper(
					new «attribute.joynrName.toFirstUpper»AttributeSubscriptionListenerWrapper(subscriptionListener));
			«ENDIF»
			std::shared_ptr<joynr::SubscriptionCallback<«returnTypeQT»>> subscriptionCallback(new joynr::SubscriptionCallback<«returnTypeQT»>(«subscriptionListenerName»));
			subscriptionManager->registerSubscription(
						attributeName,
						subscriptionCallback,
						std::shared_ptr<QtSubscriptionQos>(QtSubscriptionQos::createQt(subscriptionQos)),
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
	«var outputTypedConstParamListStd = cppStdTypeUtil.getCommaSeperatedTypedConstOutputParameterList(method)»
	«val outputTypedParamListStd = cppStdTypeUtil.getCommaSeperatedTypedOutputParameterList(method)»
	«val outputParametersStd = cppStdTypeUtil.getCommaSeparatedOutputParameterTypes(method)»
	«val outputParametersQT = qtTypeUtil.getCommaSeparatedOutputParameterTypes(method)»
	«var outputUntypedParamList = qtTypeUtil.getCommaSeperatedUntypedOutputParameterList(method, DatatypeSystemTransformation::FROM_QT_TO_STANDARD)»
	«val inputTypedParamListStd = cppStdTypeUtil.getCommaSeperatedTypedConstInputParameterList(method)»
	«val methodName = method.joynrName»
	joynr::RequestStatus «interfaceName»JoynrMessagingConnector::«methodName»(
		«outputTypedParamListStd»«IF method.outputParameters.size > 0 && method.inputParameters.size > 0», «ENDIF»«inputTypedParamListStd»
	) {
		«produceParameterSetters(method)»
		std::shared_ptr<joynr::Future<«outputParametersStd»> > future(
				new joynr::Future<«outputParametersStd»>());

		std::function<void(const joynr::RequestStatus& status«outputTypedConstParamListQT»)> onSuccess =
				[future] (const joynr::RequestStatus& status«outputTypedConstParamListQT») {
					if (status.getCode() == joynr::RequestStatusCode::OK) {
						future->onSuccess(«outputUntypedParamList»);
					} else {
						future->onError(status);
					}
				};

		std::function<void(const joynr::RequestStatus& status)> onError =
			[future] (const joynr::RequestStatus& status) {
				future->onError(status);
			};

		std::shared_ptr<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«outputParametersQT»>(
				onSuccess,
				onError));
		operationRequest(replyCaller, internalRequestObject);
		«IF method.outputParameters.empty»
			return future->waitForFinished();
		«ELSE»
			joynr::RequestStatus status = future->waitForFinished();
			if (status.successful()) {
				future->getValues(«cppStdTypeUtil.getCommaSeperatedUntypedOutputParameterList(method)»);
			}
			return status;
		«ENDIF»
	}

	std::shared_ptr<joynr::Future<«outputParametersStd»> > «interfaceName»JoynrMessagingConnector::«methodName»Async(
			«cppStdTypeUtil.getCommaSeperatedTypedConstInputParameterList(method)»«IF !method.inputParameters.empty»,«ENDIF»
			std::function<void(«outputTypedConstParamListStd»)> onSuccess,
			std::function<void(const joynr::RequestStatus& status)> onError
	)
	{
		«produceParameterSetters(method)»

		std::shared_ptr<joynr::Future<«outputParametersStd»> > future(
				new joynr::Future<«outputParametersStd»>());

		std::function<void(const joynr::RequestStatus& status«outputTypedConstParamListQT»)> onSuccessWrapper =
				[future, onSuccess, onError] (const joynr::RequestStatus& status«outputTypedConstParamListQT») {
					if (status.getCode() == joynr::RequestStatusCode::OK) {
						future->onSuccess(«outputUntypedParamList»);
						if (onSuccess) {
							onSuccess(«outputUntypedParamList»);
						}
					} else {
						future->onError(status);
						if (onError){
							onError(status);
						}
					}
				};

		std::function<void(const joynr::RequestStatus& status)> onErrorWrapper =
				[future, onError] (const joynr::RequestStatus& status) {
					future->onError(status);
					if (onError) {
						onError(status);
					}
				};

		std::shared_ptr<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«outputParametersQT»>(
				onSuccessWrapper,
				onErrorWrapper));
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
					const «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
					std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					const joynr::OnChangeSubscriptionQos& subscriptionQos
	«ELSE»
		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					const joynr::OnChangeSubscriptionQos& subscriptionQos
	«ENDIF»
	) {
		joynr::BroadcastSubscriptionRequest subscriptionRequest;
		«IF isSelective(broadcast)»
			subscriptionRequest.setFilterParameters(QtBroadcastFilterParameters::createQt(filterParameters));
		«ENDIF»
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(subscriptionListener, subscriptionQos, subscriptionRequest);
	}

	«IF isSelective(broadcast)»
		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					const «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
					std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					const joynr::OnChangeSubscriptionQos& subscriptionQos,
					std::string& subscriptionId
	«ELSE»
		std::string «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					const joynr::OnChangeSubscriptionQos& subscriptionQos,
					std::string& subscriptionId
	«ENDIF»
	) {
		joynr::BroadcastSubscriptionRequest subscriptionRequest;
		«IF isSelective(broadcast)»
			subscriptionRequest.setFilterParameters(QtBroadcastFilterParameters::createQt(filterParameters));
		«ENDIF»
		subscriptionRequest.setSubscriptionId(QString::fromStdString(subscriptionId));
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(subscriptionListener, subscriptionQos, subscriptionRequest);
	}

	std::string «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				const joynr::OnChangeSubscriptionQos& subscriptionQos,
				BroadcastSubscriptionRequest& subscriptionRequest
	) {
		LOG_DEBUG(logger, "Subscribing to «broadcastName» broadcast.");
		QString broadcastName("«broadcastName»");
		joynr::MessagingQos clonedMessagingQos(qosSettings);
		if (subscriptionQos.getExpiryDate() == joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
			clonedMessagingQos.setTtl(joynr::SubscriptionQos::NO_EXPIRY_DATE_TTL());
		}
		else{
			clonedMessagingQos.setTtl(subscriptionQos.getExpiryDate() - QDateTime::currentMSecsSinceEpoch());
		}

		«val subscriptionListenerName = if (needsDatatypeConversion(broadcast)) "subscriptionListenerWrapper" else "subscriptionListener"»
		«IF needsDatatypeConversion(broadcast)»
			std::shared_ptr<«broadcast.joynrName.toFirstUpper»BroadcastSubscriptionListenerWrapper> subscriptionListenerWrapper(
				new «broadcast.joynrName.toFirstUpper»BroadcastSubscriptionListenerWrapper(subscriptionListener));
		«ENDIF»
		std::shared_ptr<joynr::SubscriptionCallback<«returnTypesQt»>> subscriptionCallback(
					new joynr::SubscriptionCallback<«returnTypesQt»>(«subscriptionListenerName»));
		subscriptionManager->registerSubscription(
					broadcastName,
					subscriptionCallback,
					std::shared_ptr<QtOnChangeSubscriptionQos>(QtSubscriptionQos::createQt(subscriptionQos)),
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

