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
import org.franca.core.franca.FInterface
import org.franca.core.franca.FType
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.util.InterfaceTemplate

class InterfaceInProcessConnectorCPPTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension JoynrCppGeneratorExtensions

	override  generate(FInterface serviceInterface){
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
		#include "joynr/SubscriptionManager.h"
		#include "joynr/PublicationManager.h"
		#include "joynr/SubscriptionCallback.h"
		#include "joynr/BroadcastSubscriptionCallback.h"
		#include "joynr/BroadcastSubscriptionRequest.h"
		#include "joynr/Future.h"

		«getNamespaceStarter(serviceInterface)»

		using namespace joynr::joynr_logging;
		Logger* «interfaceName»InProcessConnector::logger = Logging::getInstance()->getLogger("MSG", "«interfaceName»InProcessConnector");

		«interfaceName»InProcessConnector::«interfaceName»InProcessConnector(
		            joynr::SubscriptionManager* subscriptionManager,
		            joynr::PublicationManager* publicationManager,
		            joynr::InProcessPublicationSender* inProcessPublicationSender,
		            const QString& proxyParticipantId,
		            const QString& providerParticipantId,
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
			«val returnType = getMappedDatatypeOrList(attribute)»
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

			    std::function<void(const joynr::RequestStatus& status, const «returnType»& «attributeName»)> requestCallerCallbackFct =
			            [future] (const joynr::RequestStatus& internalStatus, const «returnType»& «attributeName») {
			                if (internalStatus.getCode() == joynr::RequestStatusCode::OK) {
			                    future->onSuccess(internalStatus, «attributeName»);
			                } else {
			                    future->onFailure(internalStatus);
			                }
			            };

			    //see header for more information
			    «serviceInterface.interfaceCaller»->«getAttributeName»(requestCallerCallbackFct);
			    status = future->waitForFinished();
			    if (status.successful()) {
			        attributeValue = future->getValue();
			    }
			}

			QSharedPointer<joynr::Future<«returnType»>> «interfaceName»InProcessConnector::«getAttributeName»(
			        std::function<void(const joynr::RequestStatus& status, const «returnType»& «attributeName»)> callbackFct
			) {
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			    assert(!«serviceInterface.interfaceCaller».isNull());

			    QSharedPointer<joynr::Future<«returnType»> > future(new joynr::Future<«returnType»>());

			    std::function<void(const joynr::RequestStatus& status, const «returnType»& «attributeName»)> requestCallerCallbackFct =
			            [future, callbackFct] (const joynr::RequestStatus& internalStatus, const «returnType»& «attributeName») {
			                if (internalStatus.getCode() == joynr::RequestStatusCode::OK) {
			                    future->onSuccess(internalStatus, «attributeName»);
			                } else {
			                    future->onFailure(internalStatus);
			                }
			                if (callbackFct) {
			                    callbackFct(internalStatus, «attributeName»);
			                }
			            };

			    //see header for more information
			    «serviceInterface.interfaceCaller»->«getAttributeName»(requestCallerCallbackFct);
			    return future;
			}

			«ENDIF»
			«IF attribute.writable»
			QSharedPointer<joynr::Future<void>> «interfaceName»InProcessConnector::«setAttributeName»(
			        «returnType» input,
			        std::function<void(const joynr::RequestStatus& status)> callbackFct
			) {
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			    assert(!«serviceInterface.interfaceCaller».isNull());

			    QSharedPointer<joynr::Future<void>> future(new joynr::Future<void>());
			    std::function<void(const joynr::RequestStatus& status)> requestCallerCallbackFct =
			            [future, callbackFct] (const joynr::RequestStatus& internalStatus) {
			                if (internalStatus.getCode() == joynr::RequestStatusCode::OK) {
			                    future->onSuccess(internalStatus);
			                } else {
			                    future->onFailure(internalStatus);
			                }
			                if (callbackFct) {
			                    callbackFct(internalStatus);
			                }
			            };

			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «interfaceName»InProcessConnector::«setAttributeName»(Future) is synchronous.");
			    «serviceInterface.interfaceCaller»->«setAttributeName»(input, requestCallerCallbackFct);
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
			    std::function<void(const joynr::RequestStatus& status)> requestCallerCallbackFct =
			            [future] (const joynr::RequestStatus& internalStatus) {
			                if (internalStatus.getCode() == joynr::RequestStatusCode::OK) {
			                    future->onSuccess(internalStatus);
			                } else {
			                    future->onFailure(internalStatus);
			                }
			            };

			    //see header for more information
			    «serviceInterface.interfaceCaller»->«setAttributeName»(input, requestCallerCallbackFct);
			    status = future->waitForFinished();
			}

			«ENDIF»
			«IF attribute.notifiable»
			QString «interfaceName»InProcessConnector::subscribeTo«attributeName.toFirstUpper»(
			        QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
			        QSharedPointer<joynr::SubscriptionQos> subscriptionQos,
			        QString& subscriptionId)
			{
			    joynr::SubscriptionRequest subscriptionRequest;
			    subscriptionRequest.setSubscriptionId(subscriptionId);
			    return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
			}

			QString «interfaceName»InProcessConnector::subscribeTo«attributeName.toFirstUpper»(
			        QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
			        QSharedPointer<joynr::SubscriptionQos> subscriptionQos)
			{
			    joynr::SubscriptionRequest subscriptionRequest;
			    return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
			}

			QString «interfaceName»InProcessConnector::subscribeTo«attributeName.toFirstUpper»(
			        QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
			        QSharedPointer<joynr::SubscriptionQos> subscriptionQos,
			        joynr::SubscriptionRequest& subscriptionRequest)
			{
			    «IF isEnum(attribute.type)»
			    Q_UNUSED(subscriptionListener);
			    Q_UNUSED(subscriptionQos);
			    // TODO support enum return values in C++ client
			    LOG_FATAL(logger, "enum return values are currently not supported in C++ client (attribute name: «interfaceName».«attributeName»)");
			    assert(false);
			    // Visual C++ requires a return value
			    return QString();
				«ELSE»
			    LOG_DEBUG(logger, "Subscribing to «attributeName».");
			    assert(subscriptionManager != NULL);
			    QString attributeName = "«attributeName»";
			    QSharedPointer<joynr::SubscriptionCallback<«returnType»>> subscriptionCallback(new joynr::SubscriptionCallback<«returnType»>(subscriptionListener));
			    subscriptionManager->registerSubscription(
			            attributeName,
			            subscriptionCallback,
			            subscriptionQos,
			            subscriptionRequest);
			    LOG_DEBUG(logger, "Registered subscription: " + subscriptionRequest.toQString());
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> requestCaller = caller.dynamicCast<«interfaceName»RequestCaller>();
			    QString subscriptionId(subscriptionRequest.getSubscriptionId());

			    if(caller.isNull()) {
			        assert(publicationManager != NULL);
			        /**
			        * Provider not registered yet
			        * Dispatcher will call publicationManger->restore when a new provider is added to activate
			        * subscriptions for that provider
			        */
			        publicationManager->add(proxyParticipantId, providerParticipantId, subscriptionRequest);
			    } else {
			        publicationManager->add(proxyParticipantId, providerParticipantId, caller, subscriptionRequest, inProcessPublicationSender);
			    }
			    return subscriptionId;
			    «ENDIF»
			}

			void «interfaceName»InProcessConnector::unsubscribeFrom«attributeName.toFirstUpper»(
			        QString& subscriptionId
			) {
			    «IF isEnum(attribute.type)»
			    Q_UNUSED(subscriptionId);
			    // TODO support enum return values in C++ client
			    LOG_FATAL(logger, "enum return values are currently not supported in C++ client (attribute name: «interfaceName».«attributeName»)");
			    assert(false);
				«ELSE»
			    LOG_DEBUG(logger, "Unsubscribing. Id=" +subscriptionId);
			    assert(publicationManager != NULL);
			    LOG_DEBUG(logger, "Stopping publications by publication manager.");
			    publicationManager->stopPublication(subscriptionId);
			    assert(subscriptionManager != NULL);
			    LOG_DEBUG(logger, "Unregistering attribute subscription.");
			    subscriptionManager->unregisterSubscription(subscriptionId);
			    «ENDIF»
			}

			«ENDIF»
		«ENDFOR»

		«FOR method: getMethods(serviceInterface)»
		«var methodname = method.joynrName»
		«var parameterList = prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»
		«var outputParameter = getMappedOutputParameter(method)»
		«var inputParamList = getCommaSeperatedUntypedParameterList(method)»
		«var outputTypedParamList = prependCommaIfNotEmpty(getCommaSeperatedConstTypedOutputParameterList(method))»
		«var outputUntypedParamList = prependCommaIfNotEmpty(getCommaSeperatedUntypedOutputParameterList(method))»

		void «interfaceName»InProcessConnector::«methodname»(
		            joynr::RequestStatus& status«prependCommaIfNotEmpty(getCommaSeperatedTypedOutputParameterList(method))»«parameterList»
		) {
		    assert(!address.isNull());
		    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
		    assert(!caller.isNull());
		    QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
		    assert(!«serviceInterface.interfaceCaller».isNull());
		    QSharedPointer<joynr::Future<«FOR param: outputParameter SEPARATOR ','»«param»«ENDFOR»> > future(new joynr::Future<«FOR param: outputParameter SEPARATOR ','»«param»«ENDFOR»>());

		    std::function<void(const joynr::RequestStatus& status«outputTypedParamList»)> requestCallerCallbackFct =
		            [future] (const joynr::RequestStatus& internalStatus«outputTypedParamList») {
		                if (internalStatus.getCode() == joynr::RequestStatusCode::OK) {
		                    future->onSuccess(internalStatus«outputUntypedParamList»);
		                } else {
		                    future->onFailure(internalStatus);
		                }
		            };

		    «serviceInterface.interfaceCaller»->«methodname»(«IF !method.inputParameters.empty»«inputParamList», «ENDIF»requestCallerCallbackFct);
		    status = future->waitForFinished();
		    «IF !method.outputParameters.empty»
		    if (status.successful()) {
		        «method.outputParameters.head.joynrName» = future->getValue();
		    }
		    «ENDIF»
		}

		QSharedPointer<joynr::Future<«outputParameter.head»> > «interfaceName»InProcessConnector::«methodname»(«getCommaSeperatedTypedParameterList(method)»«IF !method.inputParameters.empty»,«ENDIF»
		            std::function<void(const joynr::RequestStatus& status«outputTypedParamList»)> callbackFct)
		{
		    assert(!address.isNull());
		    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
		    assert(!caller.isNull());
		    QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
		    assert(!«serviceInterface.interfaceCaller».isNull());
		    QSharedPointer<joynr::Future<«FOR param: outputParameter SEPARATOR ','»«param»«ENDFOR»> > future(new joynr::Future<«FOR param: outputParameter SEPARATOR ','»«param»«ENDFOR»>());

		    std::function<void(const joynr::RequestStatus& status«outputTypedParamList»)> requestCallerCallbackFct =
		            [future, callbackFct] (const joynr::RequestStatus& status«outputTypedParamList») {
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
		    «serviceInterface.interfaceCaller»->«methodname»(«IF !method.inputParameters.empty»«inputParamList», «ENDIF»requestCallerCallbackFct);
		    return future;
		}

		«ENDFOR»

		«FOR broadcast: serviceInterface.broadcasts»
			«val returnTypes = getMappedOutputParameterTypesCommaSeparated(broadcast)»
			«val broadcastName = broadcast.joynrName»

			«IF isSelective(broadcast)»
			QString «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
			    «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters,
			        QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
			        QSharedPointer<joynr::OnChangeSubscriptionQos> subscriptionQos
			«ELSE»
			QString «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
			        QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
			        QSharedPointer<joynr::OnChangeSubscriptionQos> subscriptionQos
			«ENDIF»
			) {
			    LOG_DEBUG(logger, "Subscribing to «broadcastName».");
			    assert(subscriptionManager != NULL);
			    QString broadcastName = "«broadcastName»";
			    joynr::BroadcastSubscriptionRequest subscriptionRequest;
			    «IF isSelective(broadcast)»
			    subscriptionRequest.setFilterParameters(filterParameters);
			    «ENDIF»
			    return subscribeTo«broadcastName.toFirstUpper»Broadcast(
			                subscriptionListener,
			                subscriptionQos,
			                subscriptionRequest);
			}

			«IF isSelective(broadcast)»
			QString «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
			    «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters,
			        QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
			        QSharedPointer<joynr::OnChangeSubscriptionQos> subscriptionQos,
			        QString& subscriptionId
			«ELSE»
			QString «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
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
			    return subscribeTo«broadcastName.toFirstUpper»Broadcast(
			                subscriptionListener,
			                subscriptionQos,
			                subscriptionRequest);
			}

			QString «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
			        QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
			        QSharedPointer<joynr::OnChangeSubscriptionQos> subscriptionQos,
			        joynr::BroadcastSubscriptionRequest& subscriptionRequest
			) {
			    LOG_DEBUG(logger, "Subscribing to «broadcastName».");
			    assert(subscriptionManager != NULL);
			    QString broadcastName = "«broadcastName»";
			    QSharedPointer<joynr::BroadcastSubscriptionCallback<«returnTypes»>> subscriptionCallback(
			                new joynr::BroadcastSubscriptionCallback<«returnTypes»>(subscriptionListener));
			    subscriptionManager->registerSubscription(
			                broadcastName,
			                subscriptionCallback,
			                subscriptionQos,
			                subscriptionRequest);
			    LOG_DEBUG(logger, "Registered broadcast subscription: " + subscriptionRequest.toQString());
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> requestCaller = caller.dynamicCast<«interfaceName»RequestCaller>();
			    QString subscriptionId(subscriptionRequest.getSubscriptionId());

			    if(caller.isNull()) {
			        assert(publicationManager != NULL);
			        /**
			        * Provider not registered yet
			        * Dispatcher will call publicationManger->restore when a new provider is added to activate
			        * subscriptions for that provider
			        */
			        publicationManager->add(proxyParticipantId, providerParticipantId, subscriptionRequest);
			    } else {
			        publicationManager->add(
			                    proxyParticipantId,
			                    providerParticipantId,
			                    caller,
			                    subscriptionRequest,
			                    inProcessPublicationSender);
			    }
			    return subscriptionId;
			}

			void «interfaceName»InProcessConnector::unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(
			        QString& subscriptionId
			) {
			    LOG_DEBUG(logger, "Unsubscribing broadcast. Id=" +subscriptionId);
			    assert(publicationManager != NULL);
			    LOG_DEBUG(logger, "Stopping publications by publication manager.");
			    publicationManager->stopPublication(subscriptionId);
			    assert(subscriptionManager != NULL);
			    LOG_DEBUG(logger, "Unregistering broadcast subscription.");
			    subscriptionManager->unregisterSubscription(subscriptionId);
			}
		«ENDFOR»
		«getNamespaceEnder(serviceInterface)»
		'''
	}

	def getInterfaceCaller(FInterface serviceInterface){
	   serviceInterface.joynrName.toFirstLower + "Caller"
	}
}
