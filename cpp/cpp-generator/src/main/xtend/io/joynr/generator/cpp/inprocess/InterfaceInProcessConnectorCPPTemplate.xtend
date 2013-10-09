package io.joynr.generator.cpp.inprocess
/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

class InterfaceInProcessConnectorCPPTemplate { 
	
	@Inject
	private extension TemplateBase
	
	@Inject
	private extension JoynrCppGeneratorExtensions
		
	def generate(FInterface serviceInterface){
		'''
		«var Interfacename = serviceInterface.name.toFirstUpper»
		«var interfacename = serviceInterface.name.toLowerCase»
		«warning()»
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«Interfacename»InProcessConnector.h"
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«Interfacename»RequestCaller.h"
		#include "joynr/DeclareMetatypeUtil.h"
		«FOR datatype: getAllComplexAndEnumTypes(serviceInterface)»
		«IF datatype instanceof FType»
			«IF isComplex(datatype as FType)»
				#include "«getIncludeOf(datatype as FType)»"
			«ENDIF»
		«ENDIF»
		«ENDFOR»
		
		#include "joynr/InProcessEndpointAddress.h"
		#include "joynr/SubscriptionManager.h"
		#include "joynr/PublicationManager.h"
		#include "joynr/SubscriptionCallback.h"
		#include "joynr/Future.h"
		

		«getNamespaceStarter(serviceInterface)»
		
		using namespace joynr::joynr_logging;
		Logger* «Interfacename»InProcessConnector::logger = Logging::getInstance()->getLogger("MSG", "«Interfacename»InProcessConnector");
		
		«Interfacename»InProcessConnector::«Interfacename»InProcessConnector(
					joynr::SubscriptionManager* subscriptionManager,
					joynr::PublicationManager* publicationManager,
					joynr::InProcessPublicationSender* inProcessPublicationSender,
					const QString& proxyParticipantId,
					const QString& providerParticipantId,
					QSharedPointer<joynr::InProcessEndpointAddress> endpointAddress
		) :
			proxyParticipantId(proxyParticipantId),
			providerParticipantId(providerParticipantId),
		    endpointAddress(endpointAddress),
		    subscriptionManager(subscriptionManager),
		    publicationManager(publicationManager),
		    inProcessPublicationSender(inProcessPublicationSender)
		{
		}
		
		bool «Interfacename»InProcessConnector::usesClusterController() const{
			return false;
		}
		
		«FOR attribute : getAttributes(serviceInterface)»
			«val returnType = getMappedDatatypeOrList(attribute)»
			«val attributeName = attribute.name.toFirstUpper»
			void «Interfacename»InProcessConnector::get«attributeName»(
					joynr::RequestStatus& status,
					«returnType»& result
			) {
			    assert(!endpointAddress.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
			    assert(!«interfacename»Caller.isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «Interfacename»InProcessConnector::get«attributeName»(Future) is synchronous.");
			    «interfacename»Caller->get«attributeName»(status, result);				
			}

			void «Interfacename»InProcessConnector::get«attributeName»(
					QSharedPointer<joynr::Future<«returnType»> > future,
					QSharedPointer< joynr::ICallback<«returnType»> > callBack
			) {
			    assert(!future.isNull());
			    assert(!endpointAddress.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
			    assert(!«interfacename»Caller.isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «Interfacename»InProcessConnector::get«attributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «returnType» result;
			    «interfacename»Caller->get«attributeName»(status, result);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			        future->onSuccess(status, result);
			        callBack->onSuccess(status, result);
			    } else {
			        future->onFailure(status);
			        callBack->onFailure(status);
			    }			    
			}
			
			void «Interfacename»InProcessConnector::get«attributeName»(
					QSharedPointer<joynr::Future<«returnType»> > future
			) {
			    assert(!future.isNull());
			    assert(!endpointAddress.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
			    assert(!«interfacename»Caller.isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «Interfacename»InProcessConnector::get«attributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «returnType» result;
			    «interfacename»Caller->get«attributeName»(status, result);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			        future->onSuccess(status, result);
			    } else {
			    	future->onFailure(status);
			    }
			}
			
			void «Interfacename»InProcessConnector::get«attributeName»(
					QSharedPointer< joynr::ICallback<«returnType»> > callBack
			) {
			    assert(!endpointAddress.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
			    assert(!«interfacename»Caller.isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «Interfacename»InProcessConnector::get«attributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «returnType» result;
			    «interfacename»Caller->get«attributeName»(status, result);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			    	callBack->onSuccess(status, result);
			    } else {
			        callBack->onFailure(status);
			    }
			}
			
			
			void «Interfacename»InProcessConnector::set«attributeName»(
					QSharedPointer< joynr::ICallback<void> > callBack,
					«returnType» input
			) {
			    assert(!endpointAddress.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
			    assert(!«interfacename»Caller.isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «Interfacename»InProcessConnector::set«attributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «interfacename»Caller->set«attributeName»(status, input);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			    	callBack->onSuccess(status);
			    } else {
			        callBack->onFailure(status);
			    }

			}
			
			void «Interfacename»InProcessConnector::set«attributeName»(
					QSharedPointer< joynr::Future<void> > future,
					QSharedPointer< joynr::ICallback<void> > callBack,
					«returnType» input
			) {
			    assert(!future.isNull());
			    assert(!endpointAddress.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
			    assert(!«interfacename»Caller.isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «Interfacename»InProcessConnector::set«attributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «interfacename»Caller->set«attributeName»(status, input);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			        future->onSuccess(status);
			        callBack->onSuccess(status);
			    } else {
			        future->onFailure(status);
			        callBack->onFailure(status);
			    }			    
			}
			
			void «Interfacename»InProcessConnector::set«attributeName»(
					QSharedPointer<joynr::Future<void> > future,
					«returnType» input
			) {
			    assert(!future.isNull());
			    assert(!endpointAddress.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
			    assert(!«interfacename»Caller.isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «Interfacename»InProcessConnector::set«attributeName»(Future) is synchronous.");
			    joynr::RequestStatus status;
			    «interfacename»Caller->set«attributeName»(status, input);
			    if (status.getCode()== joynr::RequestStatusCode::OK){
			        future->onSuccess(status);
			    } else {
			    	future->onFailure(status);
			    }
			}
			
			void «Interfacename»InProcessConnector::set«attributeName»(
					joynr::RequestStatus& status,
					const «returnType»& input
			) {
			    assert(!endpointAddress.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
			    assert(!«interfacename»Caller.isNull());
			    //see header for more information
			    LOG_ERROR(logger,"#### WARNING ##### «Interfacename»InProcessConnector::get«attributeName»(Future) is synchronous.");
			    «interfacename»Caller->set«attributeName»(status, input);				
			}
			
			QString «Interfacename»InProcessConnector::subscribeTo«attributeName»(
					QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					QSharedPointer<joynr::SubscriptionQos> subscriptionQos)
			{
			    «IF isEnum(attribute.type)»
			    Q_UNUSED(subscriptionListener);
			    Q_UNUSED(subscriptionQos);
			    // TODO support enum return values in C++ client
			    LOG_FATAL(logger, "enum return values are currently not supported in C++ client (attribute name: «Interfacename».«attributeName»)");
			    assert(false);
			    // Visual C++ requires a return value
			    return QString();
				«ELSE»
			    logger->log(DEBUG, "Subscribing to «attributeName».");
			    assert(subscriptionManager != NULL);
			    QString attributeName = "«attributeName»";
			    joynr::SubscriptionCallback<«returnType»>* subscriptionCallback = new joynr::SubscriptionCallback<«returnType»>(subscriptionListener);
			    joynr::SubscriptionRequest* subscriptionRequest = new joynr::SubscriptionRequest();//ownership goes to PublicationManager
			    subscriptionManager->registerAttributeSubscription(
			    			attributeName,
			    			subscriptionCallback,
			    			subscriptionQos,
			    			*subscriptionRequest);
			    logger->log(DEBUG, "Registered subscription: " + subscriptionRequest->toQString());
			    assert(!endpointAddress.isNull());
			    QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
			    assert(!caller.isNull());
			    QSharedPointer<«Interfacename»RequestCaller> requestCaller = caller.dynamicCast<«Interfacename»RequestCaller>();
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

			void «Interfacename»InProcessConnector::unsubscribeFrom«attributeName»(
					QString& subscriptionId
			) {
			    «IF isEnum(attribute.type)»
			    Q_UNUSED(subscriptionId);
			    // TODO support enum return values in C++ client
			    LOG_FATAL(logger, "enum return values are currently not supported in C++ client (attribute name: «Interfacename».«attributeName»)");
			    assert(false);
				«ELSE»
			    logger->log(DEBUG, "Unsubscribing. Id=" +subscriptionId);
			    assert(publicationManager != NULL);
			    logger->log(DEBUG, "Stopping publications by publication manager.");
			    publicationManager->stopPublication(subscriptionId);
			    assert(subscriptionManager != NULL);
			    logger->log(DEBUG, "Unregistering attribute subscription.");
			    subscriptionManager->unregisterAttributeSubscription(subscriptionId);
			    «ENDIF»
			}
		«ENDFOR»
		
		«FOR method: getMethods(serviceInterface)»
			«var methodname = method.name»
			«var parameterList = prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»
			«var outputParameter = getMappedOutputParameter(method)»
			«var inputParamList = prependCommaIfNotEmpty(getCommaSeperatedUntypedParameterList(method))»
			«var outputTypedParamList = prependCommaIfNotEmpty(getCommaSeperatedTypedOutputParameterList(method))»
			«var outputUntypedParamList = prependCommaIfNotEmpty(getCommaSeperatedUntypedOutputParameterList(method))»

		

			«IF outputParameter.head =="void"»
				void «Interfacename»InProcessConnector::«methodname»(
						joynr::RequestStatus& status«parameterList»
				) {
					assert(!endpointAddress.isNull());
					QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
					assert(!caller.isNull());
					QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
					assert(!«interfacename»Caller.isNull());
					«interfacename»Caller->«methodname»(status «inputParamList» );									
				}
			«ELSE»
				void «Interfacename»InProcessConnector::«methodname»(
						joynr::RequestStatus& status«outputTypedParamList»«parameterList»
				) {
					assert(!endpointAddress.isNull());
					QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
					assert(!caller.isNull());
					QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
					assert(!«interfacename»Caller.isNull());
					«interfacename»Caller->«methodname»(status«outputUntypedParamList» «inputParamList»);
				}
			«ENDIF»
			
		
			
			«IF outputParameter.head =="void"»
			void «Interfacename»InProcessConnector::«methodname»(
					QSharedPointer<joynr::Future<«outputParameter.head»> > future,
					QSharedPointer< joynr::ICallback<«outputParameter.head»> > callBack«parameterList»
			) {
				assert(!endpointAddress.isNull());
				QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
				assert(!«interfacename»Caller.isNull());
				joynr::RequestStatus status;

			    «interfacename»Caller->«methodname»(status «inputParamList»);

			    if (status.getCode() == joynr::RequestStatusCode::OK) {
			        future->onSuccess(status);
			        callBack->onSuccess(status);
			    } else {
			        future->onFailure(status);
			        callBack->onFailure(status);
			    }	
			}
			«ELSE»
			void «Interfacename»InProcessConnector::«methodname»(
					QSharedPointer<joynr::Future<«outputParameter.head»> > future,
					QSharedPointer< joynr::ICallback<«outputParameter.head»> > callBack«parameterList»
			) {
				assert(!endpointAddress.isNull());
				QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
				assert(!«interfacename»Caller.isNull());
				joynr::RequestStatus status;
				«outputParameter.head» result;
			    «interfacename»Caller->«methodname»(status, result «inputParamList»);

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
			void «Interfacename»InProcessConnector::«methodname»(
					QSharedPointer<joynr::Future<«outputParameter.head»> > future«parameterList»
			) {
				assert(!endpointAddress.isNull());
				QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
				assert(!«interfacename»Caller.isNull());
				joynr::RequestStatus status;

			    «interfacename»Caller->«methodname»(status «inputParamList»);

			    if (status.getCode() == joynr::RequestStatusCode::OK) {
			        future->onSuccess(status);
			    } else {
			        future->onFailure(status);
			    }	
			}
			«ELSE»
			void «Interfacename»InProcessConnector::«methodname»(
					QSharedPointer<joynr::Future<«outputParameter.head»> > future«parameterList»
			) {
				assert(!endpointAddress.isNull());
				QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
				assert(!«interfacename»Caller.isNull());
				joynr::RequestStatus status;
				«outputParameter.head» result;
			    «interfacename»Caller->«methodname»(status, result «inputParamList»);

			    if (status.getCode() == joynr::RequestStatusCode::OK) {
			        future->onSuccess(status, result);
			    } else {
			        future->onFailure(status);
			    }	
			}
			«ENDIF»
			
			
			
		«IF outputParameter.head =="void"»
			void «Interfacename»InProcessConnector::«methodname»(
					QSharedPointer< joynr::ICallback<«outputParameter.head»> > callBack«parameterList»
			) {
				assert(!endpointAddress.isNull());
				QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
				assert(!«interfacename»Caller.isNull());
				joynr::RequestStatus status;

			    «interfacename»Caller->«methodname»(status «inputParamList»);

			    if (status.getCode() == joynr::RequestStatusCode::OK) {
			        callBack->onSuccess(status);
			    } else {
			        callBack->onFailure(status);
			    }	
			}
		«ELSE»
			void «Interfacename»InProcessConnector::«methodname»(
					QSharedPointer< joynr::ICallback<«outputParameter.head»> > callBack«parameterList»
			) {
				assert(!endpointAddress.isNull());
				QSharedPointer<joynr::RequestCaller> caller = endpointAddress->getRequestCaller();
				assert(!caller.isNull());
				QSharedPointer<«Interfacename»RequestCaller> «interfacename»Caller = caller.dynamicCast<«Interfacename»RequestCaller>();
				assert(!«interfacename»Caller.isNull());
				joynr::RequestStatus status;
				«outputParameter.head» result;
			    «interfacename»Caller->«methodname»(status, result «inputParamList»);

			    if (status.getCode() == joynr::RequestStatusCode::OK) {
			        callBack->onSuccess(status, result);
			    } else {
			        callBack->onFailure(status);
			    }	
			}
		«ENDIF»
			
		«ENDFOR»
		«getNamespaceEnder(serviceInterface)»
		'''
	}
}