package io.joynr.generator.cpp.proxy
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
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions

class InterfaceProxyBaseCppTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase

	def generate(FInterface fInterface) {
		val serviceName =  fInterface.name.toFirstUpper
		val className = serviceName + "ProxyBase"
		
		'''
		«warning()»
		
		#include "«getPackagePathWithJoynrPrefix(fInterface, "/")»/«className».h"
		#include "joynr/exceptions.h"
		#include "joynr/ConnectorFactory.h"
		#include "joynr/ISubscriptionListener.h"
		#include "joynr/ICapabilities.h"
		#include "«getPackagePathWithJoynrPrefix(fInterface, "/")»/«serviceName»InProcessConnector.h"
		#include "«getPackagePathWithJoynrPrefix(fInterface, "/")»/«serviceName»JoynrMessagingConnector.h"
		
		
		«getNamespaceStarter(fInterface)»
		«className»::«className»(joynr::ICapabilities* capabilitiesStub, QSharedPointer<joynr::EndpointAddressBase> messagingEndpointAddress, joynr::ConnectorFactory* connectorFactory, joynr::IClientCache *cache, const QString &domain,
		                             const joynr::ProxyQos &proxyQos,const joynr::MessagingQos &qosSettings, bool cached) :
				joynr::ProxyBase(connectorFactory, cache, domain, interfaceName, proxyQos, qosSettings, cached),
				capabilitiesStub(capabilitiesStub),
				messagingEndpointAddress(messagingEndpointAddress),
				connector(NULL)
				
		{
		}
		
		//tm todo: this could probably moved into async proxy, by setting the IArbitrationListener in the ProxyBase
		void «className»::handleArbitrationFinished(const QString &providerParticipantId, QSharedPointer<joynr::EndpointAddressBase> endpointAddress)
		{
		    if (connector != NULL){
		        delete connector;
		    }
		    connector = connectorFactory->create<«getPackagePathWithJoynrPrefix(fInterface, "::")»::I«serviceName»Connector>(domain, proxyParticipantId, providerParticipantId, qosSettings, cache, cached, proxyQos.getReqCacheDataFreshness_ms(), endpointAddress);
		    
		    if (connector->usesClusterController()){
		         capabilitiesStub->addEndpoint(proxyParticipantId, messagingEndpointAddress, joynr::ICapabilities::NO_TIMEOUT());
		    }
		    
		    joynr::ProxyBase::handleArbitrationFinished(providerParticipantId, endpointAddress);
		}
		
		«FOR attribute: getAttributes(fInterface)»
			«var attributeName = attribute.name.toFirstUpper»
			«val returnType = getMappedDatatypeOrList(attribute)»
			void «className»::unsubscribeFrom«attributeName»(QString& subscriptionId)
			{
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot unsubscribe from «className».«attributeName», because the communication end partner is not (yet) known");
			        return;
			    }
			    else{
			        connector->unsubscribeFrom«attributeName»(subscriptionId);
			    }
			}
			
			QString «className»::subscribeTo«attributeName»(QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener, QSharedPointer<joynr::SubscriptionQos> subscriptionQos) {
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot subscribe to «className».«attributeName», because the communication end partner is not (yet) known");
			        return "";
			    }
			    else{
			        return connector->subscribeTo«attributeName»(subscriptionListener, subscriptionQos);
			    }
			}

		«ENDFOR»
		«className»::~«className»(){
		    if (connector != NULL){
		        delete connector;
		    }
		}
		«getNamespaceEnder(fInterface)»
		'''	
	}	
}
