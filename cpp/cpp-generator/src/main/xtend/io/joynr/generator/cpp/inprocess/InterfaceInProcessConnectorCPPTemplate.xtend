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
			«val getAttributeName = "get" + attribute.joynrName.toFirstUpper»
			«val setAttributeName = "set" + attribute.joynrName.toFirstUpper»
			void «interfaceName»InProcessConnector::«getAttributeName»(
					joynr::RequestStatus& status,
					«returnType»& result
			) {
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			    assert(!«serviceInterface.interfaceCaller».isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «interfaceName»InProcessConnector::«getAttributeName»(Future) is synchronous.");
			    «serviceInterface.interfaceCaller»->«getAttributeName»(status, result);	
			}

			void «interfaceName»InProcessConnector::«getAttributeName»(
					QSharedPointer<joynr::Future<«returnType»> > future,
					QSharedPointer< joynr::ICallback<«returnType»> > callBack
			) {
			    assert(!future.isNull());
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			    assert(!«serviceInterface.interfaceCaller».isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «interfaceName»InProcessConnector::«getAttributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «returnType» result;
			    «serviceInterface.interfaceCaller»->«getAttributeName»(status, result);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			        future->onSuccess(status, result);
			        callBack->onSuccess(status, result);
			    } else {
			        future->onFailure(status);
			        callBack->onFailure(status);
			    }
			}

			void «interfaceName»InProcessConnector::«getAttributeName»(
					QSharedPointer<joynr::Future<«returnType»> > future
			) {
			    assert(!future.isNull());
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			    assert(!«serviceInterface.interfaceCaller».isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «interfaceName»InProcessConnector::«getAttributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «returnType» result;
			    «serviceInterface.interfaceCaller»->«getAttributeName»(status, result);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			        future->onSuccess(status, result);
			    } else {
			    	future->onFailure(status);
			    }
			}

			void «interfaceName»InProcessConnector::«getAttributeName»(
					QSharedPointer< joynr::ICallback<«returnType»> > callBack
			) {
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			    assert(!«serviceInterface.interfaceCaller».isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «interfaceName»InProcessConnector::«getAttributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «returnType» result;
			    «serviceInterface.interfaceCaller»->«getAttributeName»(status, result);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			    	callBack->onSuccess(status, result);
			    } else {
			        callBack->onFailure(status);
			    }
			}


			void «interfaceName»InProcessConnector::«setAttributeName»(
					QSharedPointer< joynr::ICallback<void> > callBack,
					«returnType» input
			) {
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			    assert(!«serviceInterface.interfaceCaller».isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «interfaceName»InProcessConnector::«setAttributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «serviceInterface.interfaceCaller»->«setAttributeName»(status, input);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			    	callBack->onSuccess(status);
			    } else {
			        callBack->onFailure(status);
			    }

			}

			void «interfaceName»InProcessConnector::«setAttributeName»(
					QSharedPointer< joynr::Future<void> > future,
					QSharedPointer< joynr::ICallback<void> > callBack,
					«returnType» input
			) {
			    assert(!future.isNull());
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			    assert(!«serviceInterface.interfaceCaller».isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «interfaceName»InProcessConnector::«setAttributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «serviceInterface.interfaceCaller»->«setAttributeName»(status, input);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			        future->onSuccess(status);
			        callBack->onSuccess(status);
			    } else {
			        future->onFailure(status);
			        callBack->onFailure(status);
			    }
			}

			void «interfaceName»InProcessConnector::«setAttributeName»(
					QSharedPointer<joynr::Future<void> > future,
					«returnType» input
			) {
			    assert(!future.isNull());
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
			    assert(!«serviceInterface.interfaceCaller».isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «interfaceName»InProcessConnector::«setAttributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «serviceInterface.interfaceCaller»->«setAttributeName»(status, input);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			        future->onSuccess(status);
			    } else {
			    	future->onFailure(status);
			    }
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
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «interfaceName»InProcessConnector::«getAttributeName»(Future) is synchronous.");
			    «serviceInterface.interfaceCaller»->«setAttributeName»(status, input);
			}

			QString «interfaceName»InProcessConnector::subscribeTo«attributeName.toFirstUpper»(
					QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					QSharedPointer<joynr::SubscriptionQos> subscriptionQos)
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
			    logger->log(DEBUG, "Subscribing to «attributeName».");
			    assert(subscriptionManager != NULL);
			    QString attributeName = "«attributeName»";
			    joynr::SubscriptionCallback<«returnType»>* subscriptionCallback = new joynr::SubscriptionCallback<«returnType»>(subscriptionListener);
			    joynr::SubscriptionRequest* subscriptionRequest = new joynr::SubscriptionRequest();//ownership goes to PublicationManager
			    subscriptionManager->registerSubscription(
			    			attributeName,
			    			subscriptionCallback,
			    			subscriptionQos,
			    			*subscriptionRequest);
			    logger->log(DEBUG, "Registered subscription: " + subscriptionRequest->toQString());
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> requestCaller = caller.dynamicCast<«interfaceName»RequestCaller>();
			    QString subscriptionId = subscriptionRequest->getSubscriptionId();

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
			    logger->log(DEBUG, "Unsubscribing. Id=" +subscriptionId);
			    assert(publicationManager != NULL);
			    logger->log(DEBUG, "Stopping publications by publication manager.");
			    publicationManager->stopPublication(subscriptionId);
			    assert(subscriptionManager != NULL);
			    logger->log(DEBUG, "Unregistering attribute subscription.");
			    subscriptionManager->unregisterSubscription(subscriptionId);
			    «ENDIF»
			}
		«ENDFOR»

		«FOR method: getMethods(serviceInterface)»
			«var methodname = method.joynrName»
			«var parameterList = prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»
			«var outputParameter = getMappedOutputParameter(method)»
			«var inputParamList = prependCommaIfNotEmpty(getCommaSeperatedUntypedParameterList(method))»
			«var outputTypedParamList = prependCommaIfNotEmpty(getCommaSeperatedTypedOutputParameterList(method))»
			«var outputUntypedParamList = prependCommaIfNotEmpty(getCommaSeperatedUntypedOutputParameterList(method))»



			«IF outputParameter.head =="void"»
				void «interfaceName»InProcessConnector::«methodname»(
						joynr::RequestStatus& status«parameterList»
				) {
					assert(!address.isNull());
					QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
					assert(!caller.isNull());
					QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
					assert(!«serviceInterface.interfaceCaller».isNull());
					«serviceInterface.interfaceCaller»->«methodname»(status «inputParamList» );
				}
			«ELSE»
				void «interfaceName»InProcessConnector::«methodname»(
						joynr::RequestStatus& status«outputTypedParamList»«parameterList»
				) {
					assert(!address.isNull());
					QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
					assert(!caller.isNull());
					QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
					assert(!«serviceInterface.interfaceCaller».isNull());
					«serviceInterface.interfaceCaller»->«methodname»(status«outputUntypedParamList» «inputParamList»);
				}
			«ENDIF»



			«IF outputParameter.head =="void"»
			void «interfaceName»InProcessConnector::«methodname»(
					QSharedPointer<joynr::Future<«outputParameter.head»> > future,
					QSharedPointer< joynr::ICallback<«outputParameter.head»> > callBack«parameterList»
			) {
				assert(!address.isNull());
				QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
				assert(!«serviceInterface.interfaceCaller».isNull());
				joynr::RequestStatus status;

			    «serviceInterface.interfaceCaller»->«methodname»(status «inputParamList»);

			    if (status.getCode() == joynr::RequestStatusCode::OK) {
			        future->onSuccess(status);
			        callBack->onSuccess(status);
			    } else {
			        future->onFailure(status);
			        callBack->onFailure(status);
			    }	
			}
			«ELSE»
			void «interfaceName»InProcessConnector::«methodname»(
					QSharedPointer<joynr::Future<«outputParameter.head»> > future,
					QSharedPointer< joynr::ICallback<«outputParameter.head»> > callBack«parameterList»
			) {
				assert(!address.isNull());
				QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
				assert(!«serviceInterface.interfaceCaller».isNull());
				joynr::RequestStatus status;
				«outputParameter.head» result;
			    «serviceInterface.interfaceCaller»->«methodname»(status, result «inputParamList»);

			    if (status.getCode() == joynr::RequestStatusCode::OK) {
			        future->onSuccess(status, result);
			        callBack->onSuccess(status, result);
			    } else {
			        future->onFailure(status);
			        callBack->onFailure(status);
			    }
			}
			«ENDIF»


			«IF outputParameter.head =="void"»
			void «interfaceName»InProcessConnector::«methodname»(
					QSharedPointer<joynr::Future<«outputParameter.head»> > future«parameterList»
			) {
				assert(!address.isNull());
				QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
				assert(!«serviceInterface.interfaceCaller».isNull());
				joynr::RequestStatus status;

			    «serviceInterface.interfaceCaller»->«methodname»(status «inputParamList»);

			    if (status.getCode() == joynr::RequestStatusCode::OK) {
			        future->onSuccess(status);
			    } else {
			        future->onFailure(status);
			    }
			}
			«ELSE»
			void «interfaceName»InProcessConnector::«methodname»(
					QSharedPointer<joynr::Future<«outputParameter.head»> > future«parameterList»
			) {
				assert(!address.isNull());
				QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
				assert(!«serviceInterface.interfaceCaller».isNull());
				joynr::RequestStatus status;
				«outputParameter.head» result;
			    «serviceInterface.interfaceCaller»->«methodname»(status, result «inputParamList»);

			    if (status.getCode() == joynr::RequestStatusCode::OK) {
			        future->onSuccess(status, result);
			    } else {
			        future->onFailure(status);
			    }
			}
			«ENDIF»



		«IF outputParameter.head =="void"»
			void «interfaceName»InProcessConnector::«methodname»(
					QSharedPointer< joynr::ICallback<«outputParameter.head»> > callBack«parameterList»
			) {
				assert(!address.isNull());
				QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
				assert(!«serviceInterface.interfaceCaller».isNull());
				joynr::RequestStatus status;

			    «serviceInterface.interfaceCaller»->«methodname»(status «inputParamList»);

			    if (status.getCode() == joynr::RequestStatusCode::OK) {
			        callBack->onSuccess(status);
			    } else {
			        callBack->onFailure(status);
			    }	
			}
		«ELSE»
			void «interfaceName»InProcessConnector::«methodname»(
					QSharedPointer< joynr::ICallback<«outputParameter.head»> > callBack«parameterList»
			) {
				assert(!address.isNull());
				QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«interfaceName»RequestCaller> «serviceInterface.interfaceCaller» = caller.dynamicCast<«interfaceName»RequestCaller>();
				assert(!«serviceInterface.interfaceCaller».isNull());
				joynr::RequestStatus status;
				«outputParameter.head» result;
			    «serviceInterface.interfaceCaller»->«methodname»(status, result «inputParamList»);

			    if (status.getCode() == joynr::RequestStatusCode::OK) {
			        callBack->onSuccess(status, result);
			    } else {
			        callBack->onFailure(status);
			    }
			}
		«ENDIF»
		«ENDFOR»

		«FOR broadcast: serviceInterface.broadcasts»
			«val returnTypes = getMappedOutputParameterTypesCommaSeparated(broadcast)»
			«val broadcastName = broadcast.joynrName»

			«IF isSelective(broadcast)»
			QString «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
			    «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters,
			        QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
			        QSharedPointer<joynr::SubscriptionQos> subscriptionQos
			«ELSE»
			QString «interfaceName»InProcessConnector::subscribeTo«broadcastName.toFirstUpper»Broadcast(
			        QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
			        QSharedPointer<joynr::SubscriptionQos> subscriptionQos
			«ENDIF»
			) {
			    logger->log(DEBUG, "Subscribing to «broadcastName».");
			    assert(subscriptionManager != NULL);
			    QString broadcastName = "«broadcastName»";
			    joynr::BroadcastSubscriptionCallback<«returnTypes»>* subscriptionCallback =
			                new joynr::BroadcastSubscriptionCallback<«returnTypes»>(subscriptionListener);
			    joynr::BroadcastSubscriptionRequest* subscriptionRequest =
			                new joynr::BroadcastSubscriptionRequest();//ownership goes to PublicationManager
			    «IF isSelective(broadcast)»
			    subscriptionRequest->setFilterParameters(filterParameters);
			    «ENDIF»
			    subscriptionManager->registerSubscription(
			                broadcastName,
			                subscriptionCallback,
			                subscriptionQos,
			                *subscriptionRequest);
			    logger->log(DEBUG, "Registered broadcast subscription: " + subscriptionRequest->toQString());
			    assert(!address.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = address->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«interfaceName»RequestCaller> requestCaller = caller.dynamicCast<«interfaceName»RequestCaller>();
			    QString subscriptionId = subscriptionRequest->getSubscriptionId();

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
			    logger->log(DEBUG, "Unsubscribing broadcast. Id=" +subscriptionId);
			    assert(publicationManager != NULL);
			    logger->log(DEBUG, "Stopping publications by publication manager.");
			    publicationManager->stopPublication(subscriptionId);
			    assert(subscriptionManager != NULL);
			    logger->log(DEBUG, "Unregistering broadcast subscription.");
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
