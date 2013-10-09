package io.joynr.generator.cpp.joynrmessaging
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
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions

class InterfaceJoynrMessagingConnectorCppTemplate {
	
	@Inject
	private extension TemplateBase
	
	@Inject
	private extension JoynrCppGeneratorExtensions
	
	
	def produceParameterSetters(FMethod method)
	'''
	joynr::Request internalRequestObject;
	internalRequestObject.setMethodName(QString("«method.name»"));
	«FOR param : getInputParameters(method)»
		«IF isEnum(param.type) && isArray(param)»
			internalRequestObject.addParam(joynr::Util::convertListToVariantList(«param.name»), "«getJoynrTypeName(param)»");
		«ELSEIF isEnum(param.type)»
			internalRequestObject.addParam(QVariant::fromValue(«param.name»), "«getJoynrTypeName(param)»");
		«ELSEIF isArray(param)»
		    QList<QVariant> «param.name»QVarList = joynr::Util::convertListToVariantList(«param.name»);
		    internalRequestObject.addParam(QVariant::fromValue(«param.name»QVarList), "«getJoynrTypeName(param)»");
		«ELSEIF isComplex(param.type)»
			internalRequestObject.addParam(QVariant::fromValue(«param.name»), "«getJoynrTypeName(param)»");
		«ELSE»
			internalRequestObject.addParam(QVariant(«param.name»), "«getJoynrTypeName(param)»");
		«ENDIF»
	«ENDFOR»
	'''
	

	def generate(FInterface serviceInterface)
	'''
		«warning()»

		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«serviceInterface.name.toFirstUpper»JoynrMessagingConnector.h"
		#include "joynr/ReplyCaller.h"
		#include "joynr/JoynrMessageSender.h"
		#include "joynr/joynrlogging.h"
		#include "joynr/SubscriptionManager.h"
		#include "joynr/SubscriptionCallback.h"
		#include "joynr/Util.h"
		#include "joynr/SubscriptionStop.h"
		#include "joynr/Future.h"
		
		
		«FOR datatype: getAllComplexAndEnumTypes(serviceInterface)»
		«IF datatype instanceof FType»
			«IF isComplex(datatype as FType)»
				#include "«getIncludeOf(datatype as FType)»"
			«ENDIF»
		«ENDIF»
		«ENDFOR»
		
		«getNamespaceStarter(serviceInterface)» 
		
		«serviceInterface.name.toFirstUpper»JoynrMessagingConnector::«serviceInterface.name.toFirstUpper»JoynrMessagingConnector(
		        joynr::IJoynrMessageSender* joynrMessageSender,
		        joynr::SubscriptionManager* subscriptionManager,
		        const QString &domain,
		        const QString proxyParticipantId,
		        const QString& providerParticipantId,
		        const joynr::MessagingQos &qosSettings,
		        joynr::IClientCache *cache,
		        bool cached,
		        qint64 reqCacheDataFreshness_ms)
		    : joynr::AbstractJoynrMessagingConnector(joynrMessageSender, subscriptionManager, domain, getInterfaceName(), proxyParticipantId, providerParticipantId, qosSettings, cache, cached, reqCacheDataFreshness_ms)
		{
		}
		
		bool «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::usesClusterController() const{
			return joynr::AbstractJoynrMessagingConnector::usesClusterController(); 
		}
		
		«FOR attribute: getAttributes(serviceInterface)»
			«val returnType = getMappedDatatypeOrList(attribute)»
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::get«attribute.name.toFirstUpper»(joynr::RequestStatus& status, «getMappedDatatypeOrList(attribute)»& «attribute.name») {
			    QSharedPointer<joynr::Future<«getMappedDatatypeOrList(attribute)»> > future = QSharedPointer<joynr::Future<«getMappedDatatypeOrList(attribute)»> >(new joynr::Future<«getMappedDatatypeOrList(attribute)»>());
			    QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<«getMappedDatatypeOrList(attribute)»>(future));
			    
			    // check cache here
			    attributeRequest<«getMappedDatatypeOrList(attribute)»>(QString("get«attribute.name.toFirstUpper»"), status, replyCaller);
			    status = future->waitForFinished();
			    if (status.successful()) {
			        «attribute.name» = future->getValue();
			        // add result to caching
			    }
			}
			
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::get«attribute.name.toFirstUpper»(QSharedPointer<joynr::Future<«getMappedDatatypeOrList(attribute)»> > future, QSharedPointer< joynr::ICallback<«getMappedDatatypeOrList(attribute)»> > callback) {
				assert(!future.isNull());
				future->setCallback(callback);
				QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<«getMappedDatatypeOrList(attribute)»>(future));
			    
				// check cache here
				attributeRequest<«getMappedDatatypeOrList(attribute)»>(QString("get«attribute.name.toFirstUpper»"), future->getStatus(), replyCaller);
			}
			
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::get«attribute.name.toFirstUpper»(QSharedPointer<joynr::Future<«getMappedDatatypeOrList(attribute)»> > future) {
				assert(!future.isNull());
				QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<«getMappedDatatypeOrList(attribute)»>(future));
			    
				// check cache here
				attributeRequest<«getMappedDatatypeOrList(attribute)»>(QString("get«attribute.name.toFirstUpper»"), future->getStatus(), replyCaller);
			}
			
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::get«attribute.name.toFirstUpper»(QSharedPointer< joynr::ICallback<«getMappedDatatypeOrList(attribute)»> > callback) {
				QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<«getMappedDatatypeOrList(attribute)»>(callback));
				joynr::RequestStatus status;

				// check cache here
				attributeRequest<«getMappedDatatypeOrList(attribute)»>(QString("get«attribute.name.toFirstUpper»"), status, replyCaller);
			}
			
			
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::set«attribute.name.toFirstUpper»(QSharedPointer< joynr::ICallback<void> > callBack, «getMappedDatatypeOrList(attribute)» «attribute.name») {
				joynr::Request internalRequestObject;
				internalRequestObject.setMethodName(QString("set«attribute.name.toFirstUpper»"));
				«IF isArray(attribute)»
					QList<QVariant> «attribute.name»QVarList = joynr::Util::convertListToVariantList(«attribute.name»);
					internalRequestObject.addParam(QVariant::fromValue(«attribute.name»QVarList), "«getJoynrTypeName(attribute)»");
				«ELSE»
					internalRequestObject.addParam(QVariant::fromValue(«attribute.name»), "«getJoynrTypeName(attribute)»");
				«ENDIF»
				
				QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<void>(callBack));
				joynr::RequestStatus status;
				
				operationRequest(status, replyCaller, internalRequestObject);
			}
			
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::set«attribute.name.toFirstUpper»(QSharedPointer< joynr::Future<void> > future, QSharedPointer< joynr::ICallback<void> > callBack, «getMappedDatatypeOrList(attribute)» «attribute.name»){
				assert(!future.isNull());
				joynr::Request internalRequestObject;
				internalRequestObject.setMethodName(QString("set«attribute.name.toFirstUpper»"));
				«IF isArray(attribute)»
					QList<QVariant> «attribute.name»QVarList = joynr::Util::convertListToVariantList(«attribute.name»);
					internalRequestObject.addParam(QVariant::fromValue(«attribute.name»QVarList), "«getJoynrTypeName(attribute)»");
				«ELSE»
					internalRequestObject.addParam(QVariant::fromValue(«attribute.name»), "«getJoynrTypeName(attribute)»");
				«ENDIF»
				QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<void>(callBack));
				operationRequest(future->getStatus(),  replyCaller, internalRequestObject);
			}
			
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::set«attribute.name.toFirstUpper»(QSharedPointer<joynr::Future<void> > future, «getMappedDatatypeOrList(attribute)» «attribute.name»){
				assert(!future.isNull());
				joynr::Request internalRequestObject;
				internalRequestObject.setMethodName(QString("set«attribute.name.toFirstUpper»"));
				«IF isArray(attribute)»
					QList<QVariant> «attribute.name»QVarList = joynr::Util::convertListToVariantList(«attribute.name»);
					internalRequestObject.addParam(QVariant::fromValue(«attribute.name»QVarList), "«getJoynrTypeName(attribute)»");
				«ELSE»
					internalRequestObject.addParam(QVariant::fromValue(«attribute.name»), "«getJoynrTypeName(attribute)»");
				«ENDIF»
				QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<void>(future));
				operationRequest(future->getStatus(), replyCaller, internalRequestObject);
			}
			
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::set«attribute.name.toFirstUpper»(joynr::RequestStatus& status, const «getMappedDatatypeOrList(attribute)»& «attribute.name») {
			    joynr::Request internalRequestObject;
				internalRequestObject.setMethodName(QString("set«attribute.name.toFirstUpper»"));
				«IF isArray(attribute)»
					QList<QVariant> «attribute.name»QVarList = joynr::Util::convertListToVariantList(«attribute.name»);
					internalRequestObject.addParam(QVariant::fromValue(«attribute.name»QVarList), "«getJoynrTypeName(attribute)»");
				«ELSE»
					internalRequestObject.addParam(QVariant::fromValue(«attribute.name»), "«getJoynrTypeName(attribute)»");
				«ENDIF»
			
			    QSharedPointer<joynr::Future<void> > future = QSharedPointer<joynr::Future<void> >( new joynr::Future<void>());
			    QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<void>(future));
			    operationRequest(status, replyCaller, internalRequestObject);
			    status = future->waitForFinished();
			}
			
			QString «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::subscribeTo«attribute.name.toFirstUpper»(
					QSharedPointer<joynr::ISubscriptionListener<«returnType» > > subscriptionListener,
					QSharedPointer<joynr::SubscriptionQos> subscriptionQos
			) {
			    LOG_DEBUG(logger, "Subscribing to «attribute.name».");
			    QString attributeName = "«attribute.name»";
			    joynr::SubscriptionCallback<«returnType»>* subscriptionCallback = new joynr::SubscriptionCallback<«returnType»>(subscriptionListener);
			    joynr::SubscriptionRequest subscriptionRequest;
			    subscriptionManager->registerAttributeSubscription(
			    			attributeName,
			    			subscriptionCallback,
			    			subscriptionQos,
			    			subscriptionRequest);
			    LOG_DEBUG(logger, subscriptionRequest.toQString());
			    joynrMessageSender->sendSubscriptionRequest(
			                proxyParticipantId,
			                providerParticipantId,
			                qosSettings,
			                subscriptionRequest
			    );
			    return subscriptionRequest.getSubscriptionId();
			}
			
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::unsubscribeFrom«attribute.name.toFirstUpper»(
					QString& subscriptionId
			) {
			    joynr::SubscriptionStop subscriptionStop;
			    subscriptionStop.setSubscriptionId(subscriptionId);
			    
			    subscriptionManager->unregisterAttributeSubscription(subscriptionId);
				joynrMessageSender->sendSubscriptionStop(
			                proxyParticipantId,
							providerParticipantId,
							qosSettings,
			                subscriptionStop
			    );
			}

		«ENDFOR»
		«FOR method: getMethods(serviceInterface)»
			«val outputParameter = getMappedOutputParameter(method)»
			«IF outputParameter.head == "void"»
				void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::«method.name»(joynr::RequestStatus& status«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))») {
					«produceParameterSetters(method)»
					QSharedPointer<joynr::Future<«outputParameter.head»> > future = QSharedPointer<joynr::Future<«outputParameter.head»> >(new joynr::Future<«outputParameter.head»>());
					QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<«outputParameter.head»>(future));
					operationRequest(status, replyCaller, internalRequestObject);
					status = future->waitForFinished();
				}
			«ELSE»
				void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::«method.name»(joynr::RequestStatus& status, «getCommaSeperatedTypedOutputParameterList(method)»«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))») {
					«produceParameterSetters(method)»
					QSharedPointer<joynr::Future<«outputParameter.head»> > future = QSharedPointer<joynr::Future<«outputParameter.head»> >(new joynr::Future<«outputParameter.head»>());
					QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<«outputParameter.head»>(future));
					operationRequest(status, replyCaller, internalRequestObject);
					status = future->waitForFinished();
					if (status.successful()) {
						«getOutputParameters(method).head.name» = future->getValue();
					}
				}
			«ENDIF»
		
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::«method.name»(QSharedPointer<joynr::Future<«outputParameter.head»> > future, QSharedPointer< joynr::ICallback<«outputParameter.head»> > callback «prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»){
				«produceParameterSetters(method)»
				assert(!future.isNull());
				future->setCallback(callback);
				QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<«outputParameter.head»>(future));
				operationRequest(future->getStatus(), replyCaller, internalRequestObject);
			}
			
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::«method.name»(QSharedPointer<joynr::Future<«outputParameter.head»> > future «prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))») {
				«produceParameterSetters(method)»
				QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<«outputParameter.head»>(future));
				operationRequest(future->getStatus(), replyCaller, internalRequestObject);
			}
			
			void «serviceInterface.name.toFirstUpper»JoynrMessagingConnector::«method.name»(QSharedPointer< joynr::ICallback<«outputParameter.head»> > callback«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))») {
				«produceParameterSetters(method)»
				QSharedPointer<joynr::IReplyCaller> replyCaller = QSharedPointer<joynr::IReplyCaller>(new joynr::ReplyCaller<«outputParameter.head»>(callback));
				joynr::RequestStatus status(joynr::RequestStatusCode::NOT_STARTED);
				operationRequest(status, replyCaller, internalRequestObject);
			}
		
		«ENDFOR»
		«getNamespaceEnder(serviceInterface)»
	'''
}
	
