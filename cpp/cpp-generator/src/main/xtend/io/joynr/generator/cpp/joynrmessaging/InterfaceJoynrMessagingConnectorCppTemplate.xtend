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
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.util.InterfaceTemplate

class InterfaceJoynrMessagingConnectorCppTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension JoynrCppGeneratorExtensions

	def produceParameterSetters(FMethod method)
'''
joynr::Request internalRequestObject;
internalRequestObject.setMethodName(QString("«method.joynrName»"));
«FOR param : getInputParameters(method)»
«IF isEnum(param.type) && isArray(param)»
	internalRequestObject.addParam(joynr::Util::convertListToVariantList(«param.joynrName»), "«getJoynrTypeName(param)»");
«ELSEIF isEnum(param.type)»
	internalRequestObject.addParam(QVariant::fromValue(«param.joynrName»), "«getJoynrTypeName(param)»");
«ELSEIF isArray(param)»
	QList<QVariant> «param.joynrName»QVarList = joynr::Util::convertListToVariantList(«param.joynrName»);
	internalRequestObject.addParam(QVariant::fromValue(«param.joynrName»QVarList), "«getJoynrTypeName(param)»");
«ELSEIF isComplex(param.type)»
	internalRequestObject.addParam(QVariant::fromValue(«param.joynrName»), "«getJoynrTypeName(param)»");
«ELSE»
	internalRequestObject.addParam(QVariant(«param.joynrName»), "«getJoynrTypeName(param)»");
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
#include "joynr/SubscriptionManager.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/BroadcastSubscriptionCallback.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/Util.h"
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
		joynr::SubscriptionManager* subscriptionManager,
		const QString &domain,
		const QString proxyParticipantId,
		const QString& providerParticipantId,
		const joynr::MessagingQos &qosSettings,
		joynr::IClientCache *cache,
		bool cached)
	: joynr::AbstractJoynrMessagingConnector(joynrMessageSender, subscriptionManager, domain, getInterfaceName(), proxyParticipantId, providerParticipantId, qosSettings, cache, cached)
{
}

bool «interfaceName»JoynrMessagingConnector::usesClusterController() const{
	return joynr::AbstractJoynrMessagingConnector::usesClusterController();
}

«FOR attribute: getAttributes(serviceInterface)»
	«val returnType = getMappedDatatypeOrList(attribute)»
	«val attributeName = attribute.joynrName»
	«IF attribute.readable»
		void «interfaceName»JoynrMessagingConnector::get«attributeName.toFirstUpper»(
				joynr::RequestStatus& status,
				«returnType»& «attributeName»
		) {
			QSharedPointer<joynr::Future<«returnType»> > future(new joynr::Future<«returnType»>());

			std::function<void(const joynr::RequestStatus& status, const «returnType»& «attributeName»)> replyCallerCallbackFct =
					[future] (const joynr::RequestStatus& status, const «returnType»& «attributeName») {
						if (status.getCode() == joynr::RequestStatusCode::OK) {
							future->onSuccess(status, «attributeName»);
						} else {
							future->onFailure(status);
						}
					};

			QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«returnType»>(replyCallerCallbackFct));
			attributeRequest<«returnType»>(QString("get«attributeName.toFirstUpper»"), replyCaller);
			status = future->waitForFinished();
			if (status.successful()) {
				«attributeName» = future->getValue();
				// add result to caching
			}
		}

		QSharedPointer<joynr::Future<«returnType»>> «interfaceName»JoynrMessagingConnector::get«attributeName.toFirstUpper»(
				std::function<void(const joynr::RequestStatus& status, const «returnType»& «attributeName»)> callbackFct
		) {
			QSharedPointer<joynr::Future<«returnType»> > future(new joynr::Future<«returnType»>());

			std::function<void(const joynr::RequestStatus& status, const «returnType»& «attributeName»)> replyCallerCallbackFct =
					[future, callbackFct] (const joynr::RequestStatus& status, const «returnType»& «attributeName») {
						if (status.getCode() == joynr::RequestStatusCode::OK) {
							future->onSuccess(status, «attributeName»);
						} else {
							future->onFailure(status);
						}
						if (callbackFct){
							callbackFct(status, «attributeName»);
						}
					};

			QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«returnType»>(replyCallerCallbackFct));
			attributeRequest<«returnType»>(QString("get«attributeName.toFirstUpper»"), replyCaller);

			return future;
		}

	«ENDIF»
	«IF attribute.writable»
		QSharedPointer<joynr::Future<void>> «interfaceName»JoynrMessagingConnector::set«attributeName.toFirstUpper»(
				«returnType» «attributeName»,
				std::function<void(const joynr::RequestStatus& status)> callbackFct
		) {
			joynr::Request internalRequestObject;
			internalRequestObject.setMethodName(QString("set«attributeName.toFirstUpper»"));
			«IF isArray(attribute)»
				QList<QVariant> «attributeName»QVarList = joynr::Util::convertListToVariantList(«attributeName»);
				internalRequestObject.addParam(QVariant::fromValue(«attributeName»QVarList), "«getJoynrTypeName(attribute)»");
			«ELSE»
				internalRequestObject.addParam(QVariant::fromValue(«attributeName»), "«getJoynrTypeName(attribute)»");
			«ENDIF»

			QSharedPointer<joynr::Future<void>> future(new joynr::Future<void>());

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

			QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<void>(replyCallerCallbackFct));
			operationRequest(replyCaller, internalRequestObject);
			return future;
		}

		void «interfaceName»JoynrMessagingConnector::set«attributeName.toFirstUpper»(
				joynr::RequestStatus& status,
				const «returnType»& «attributeName»
		) {
			joynr::Request internalRequestObject;
			internalRequestObject.setMethodName(QString("set«attributeName.toFirstUpper»"));
			«IF isArray(attribute)»
				QList<QVariant> «attributeName»QVarList = joynr::Util::convertListToVariantList(«attributeName»);
				internalRequestObject.addParam(QVariant::fromValue(«attributeName»QVarList), "«getJoynrTypeName(attribute)»");
			«ELSE»
				internalRequestObject.addParam(QVariant::fromValue(«attributeName»), "«getJoynrTypeName(attribute)»");
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

			QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<void>(replyCallerCallbackFct));
			operationRequest(replyCaller, internalRequestObject);
			status = future->waitForFinished();
		}

	«ENDIF»
	«IF attribute.notifiable»
		QString «interfaceName»JoynrMessagingConnector::subscribeTo«attributeName.toFirstUpper»(
					QSharedPointer<joynr::ISubscriptionListener<«returnType» > > subscriptionListener,
					QSharedPointer<joynr::SubscriptionQos> subscriptionQos
		) {
			joynr::SubscriptionRequest subscriptionRequest;
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		QString «interfaceName»JoynrMessagingConnector::subscribeTo«attributeName.toFirstUpper»(
					QSharedPointer<joynr::ISubscriptionListener<«returnType» > > subscriptionListener,
					QSharedPointer<joynr::SubscriptionQos> subscriptionQos,
					QString& subscriptionId
		) {

			joynr::SubscriptionRequest subscriptionRequest;
			subscriptionRequest.setSubscriptionId(subscriptionId);
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		QString «interfaceName»JoynrMessagingConnector::subscribeTo«attributeName.toFirstUpper»(
					QSharedPointer<joynr::ISubscriptionListener<«returnType» > > subscriptionListener,
					QSharedPointer<joynr::SubscriptionQos> subscriptionQos,
					SubscriptionRequest& subscriptionRequest
		) {
			LOG_DEBUG(logger, "Subscribing to «attributeName».");
			QString attributeName = "«attributeName»";
			joynr::MessagingQos clonedMessagingQos(qosSettings);
			if (subscriptionQos->getExpiryDate() == joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
				clonedMessagingQos.setTtl(joynr::SubscriptionQos::NO_EXPIRY_DATE_TTL());
			}
			else{
				clonedMessagingQos.setTtl(subscriptionQos->getExpiryDate() - QDateTime::currentMSecsSinceEpoch());
			}
			QSharedPointer<joynr::SubscriptionCallback<«returnType»>> subscriptionCallback(new joynr::SubscriptionCallback<«returnType»>(subscriptionListener));
			subscriptionManager->registerSubscription(
						attributeName,
						subscriptionCallback,
						subscriptionQos,
						subscriptionRequest);
			LOG_DEBUG(logger, subscriptionRequest.toQString());
			joynrMessageSender->sendSubscriptionRequest(
						proxyParticipantId,
						providerParticipantId,
						clonedMessagingQos,
						subscriptionRequest
			);
			return subscriptionRequest.getSubscriptionId();
		}

		void «interfaceName»JoynrMessagingConnector::unsubscribeFrom«attributeName.toFirstUpper»(
				QString& subscriptionId
		) {
			joynr::SubscriptionStop subscriptionStop;
			subscriptionStop.setSubscriptionId(subscriptionId);

			subscriptionManager->unregisterSubscription(subscriptionId);
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
	«var outputTypedConstParamList = prependCommaIfNotEmpty(getCommaSeperatedConstTypedOutputParameterList(method))»
	«val outputTypedParamList = prependCommaIfNotEmpty(getCommaSeperatedTypedOutputParameterList(method))»
	«var outputUntypedParamList = prependCommaIfNotEmpty(getCommaSeperatedUntypedOutputParameterList(method))»
	«val inputTypedParamList = prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»
	«val outputParameter = getMappedOutputParameter(method)»
	«val methodName = method.joynrName»
	void «interfaceName»JoynrMessagingConnector::«methodName»(
			joynr::RequestStatus& status«outputTypedParamList»«inputTypedParamList»
	) {
		«produceParameterSetters(method)»
		QSharedPointer<joynr::Future<«outputParameter.head»> > future(new joynr::Future<«outputParameter.head»>());

		std::function<void(const joynr::RequestStatus& status«outputTypedConstParamList»)> replyCallerCallbackFct =
				[future] (const joynr::RequestStatus& status«outputTypedConstParamList») {
					if (status.getCode() == joynr::RequestStatusCode::OK) {
						future->onSuccess(status«outputUntypedParamList»);
					} else {
						future->onFailure(status);
					}
				};

		QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«outputParameter.head»>(replyCallerCallbackFct));
		operationRequest(replyCaller, internalRequestObject);
		status = future->waitForFinished();
		«IF outputParameter.head != "void"»
			if (status.successful()) {
				«getOutputParameters(method).head.joynrName» = future->getValue();
			}
		«ENDIF»
	}

	QSharedPointer<joynr::Future<«outputParameter.head»> > «interfaceName»JoynrMessagingConnector::«methodName»(
			«getCommaSeperatedTypedParameterList(method)»«IF !method.inputParameters.empty»,«ENDIF»
			std::function<void(const joynr::RequestStatus& status«outputTypedConstParamList»)> callbackFct)
	{
		«produceParameterSetters(method)»

		QSharedPointer<joynr::Future<«FOR param: outputParameter SEPARATOR ','»«param»«ENDFOR»> > future(
				new joynr::Future<«FOR param: outputParameter SEPARATOR ','»«param»«ENDFOR»>());

		std::function<void(const joynr::RequestStatus& status«outputTypedConstParamList»)> replyCallerCallbackFct =
				[future, callbackFct] (const joynr::RequestStatus& status«outputTypedConstParamList») {
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

		QSharedPointer<joynr::IReplyCaller> replyCaller(new joynr::ReplyCaller<«outputParameter.head»>(replyCallerCallbackFct));
		operationRequest(replyCaller, internalRequestObject);
		return future;
	}

«ENDFOR»

«FOR broadcast: serviceInterface.broadcasts»
	«val returnTypes = getMappedOutputParameterTypesCommaSeparated(broadcast)»
	«val broadcastName = broadcast.joynrName»
	«IF isSelective(broadcast)»
		QString «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					«interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters,
					QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					QSharedPointer<joynr::OnChangeSubscriptionQos> subscriptionQos
	«ELSE»
		QString «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					QSharedPointer<joynr::OnChangeSubscriptionQos> subscriptionQos
	«ENDIF»
	) {
		joynr::BroadcastSubscriptionRequest subscriptionRequest;
		«IF isSelective(broadcast)»
			subscriptionRequest.setFilterParameters(filterParameters);
		«ENDIF»
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(subscriptionListener, subscriptionQos, subscriptionRequest);
	}

	«IF isSelective(broadcast)»
		QString «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					«interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters,
					QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					QSharedPointer<joynr::OnChangeSubscriptionQos> subscriptionQos,
					QString& subscriptionId
	«ELSE»
		QString «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
					QSharedPointer<joynr::OnChangeSubscriptionQos> subscriptionQos,
					QString& subscriptionId
	«ENDIF»
	) {
		joynr::BroadcastSubscriptionRequest subscriptionRequest;
		«IF isSelective(broadcast)»
			subscriptionRequest.setFilterParameters(filterParameters);
		«ENDIF»
		subscriptionRequest.setSubscriptionId(subscriptionId);
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(subscriptionListener, subscriptionQos, subscriptionRequest);
	}

	QString «interfaceName»JoynrMessagingConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				QSharedPointer<joynr::OnChangeSubscriptionQos> subscriptionQos,
				BroadcastSubscriptionRequest& subscriptionRequest
	) {
		LOG_DEBUG(logger, "Subscribing to «broadcastName» broadcast.");
		QString broadcastName = "«broadcastName»";
		joynr::MessagingQos clonedMessagingQos(qosSettings);
		if (subscriptionQos->getExpiryDate() == joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
			clonedMessagingQos.setTtl(joynr::SubscriptionQos::NO_EXPIRY_DATE_TTL());
		}
		else{
			clonedMessagingQos.setTtl(subscriptionQos->getExpiryDate() - QDateTime::currentMSecsSinceEpoch());
		}
		QSharedPointer<joynr::BroadcastSubscriptionCallback<«returnTypes»>> subscriptionCallback(
					new joynr::BroadcastSubscriptionCallback<«returnTypes»>(subscriptionListener));
		subscriptionManager->registerSubscription(
					broadcastName,
					subscriptionCallback,
					subscriptionQos,
					subscriptionRequest);
		LOG_DEBUG(logger, subscriptionRequest.toQString());
		joynrMessageSender->sendBroadcastSubscriptionRequest(
					proxyParticipantId,
					providerParticipantId,
					clonedMessagingQos,
					subscriptionRequest
		);
		return subscriptionRequest.getSubscriptionId();
	}

	void «interfaceName»JoynrMessagingConnector::unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(
			QString& subscriptionId
	) {
		joynr::SubscriptionStop subscriptionStop;
		subscriptionStop.setSubscriptionId(subscriptionId);

		subscriptionManager->unregisterSubscription(subscriptionId);
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

