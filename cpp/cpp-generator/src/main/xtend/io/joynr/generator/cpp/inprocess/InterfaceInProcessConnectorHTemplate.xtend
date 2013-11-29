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
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil
import io.joynr.generator.cpp.util.InterfaceUtil
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions

class InterfaceInProcessConnectorHTemplate {
	
	@Inject
	private extension TemplateBase
	
	@Inject
	private extension JoynrCppGeneratorExtensions
	
	@Inject
	private extension InterfaceUtil
	
	@Inject
	private extension InterfaceSubscriptionUtil
	
	def generate(FInterface serviceInterface){
		val interfaceName = serviceInterface.joynrName
		val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+"_"+interfaceName+"InProcessConnector_h").toUpperCase
		'''
		«warning()»
		
		#ifndef «headerGuard»
		#define «headerGuard»
		
		#include "joynr/PrivateCopyAssign.h"
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName»Connector.h"
		#include "joynr/InProcessPublicationSender.h"
		#include "joynr/InProcessConnectorFactory.h"
		
		#include <QString>
		#include <QSharedPointer>

		namespace joynr {
			class RequestStatus;
			class InProcessEndpointAddress;
			class SubscriptionManager;
			class PublicationManager;
		}

		«getNamespaceStarter(serviceInterface)»
		
		class «interfaceName»InProcessConnector : public I«interfaceName»Connector {
		public:
		
		    «interfaceName»InProcessConnector(
		    			joynr::SubscriptionManager* subscriptionManager,
		    			joynr::PublicationManager* publicationManager,
		    			joynr::InProcessPublicationSender* inProcessPublicationSender,
		    			const QString& proxyParticipantId,
		    			const QString& providerParticipantId,
		    			QSharedPointer<joynr::InProcessEndpointAddress> endpointAddress
		    );
		
			virtual bool usesClusterController() const;
			
			«produceSyncGetters(serviceInterface, false)»
			«produceSyncSetters(serviceInterface, false)»
			«produceSyncMethods(serviceInterface, false)»
			«produceAsyncGetters(serviceInterface, false)»
			«produceAsyncSetters(serviceInterface, false)»
			«produceAsyncMethods(serviceInterface, false)»
		
			«produceSubscribeUnsubscribeMethods(serviceInterface, false)»
		    
		
		private:
		    static joynr::joynr_logging::Logger* logger;
		
		    DISALLOW_COPY_AND_ASSIGN(«interfaceName»InProcessConnector);
		    QString proxyParticipantId;
		    QString providerParticipantId;
		    QSharedPointer<joynr::InProcessEndpointAddress> endpointAddress;
		    joynr::SubscriptionManager* subscriptionManager;
		    joynr::PublicationManager* publicationManager;
		    joynr::InProcessPublicationSender* inProcessPublicationSender;
		};
		«getNamespaceEnder(serviceInterface)»

		namespace joynr {
			
		«var packagePrefix = getPackagePathWithJoynrPrefix(serviceInterface, "::")»
		
		// Helper class for use by the InProcessConnectorFactory
		// This class creates instances of «interfaceName»InProcessConnector
		template <>
		class InProcessConnectorFactoryHelper <«packagePrefix»::I«interfaceName»Connector> {
		public:
		    «packagePrefix»::«interfaceName»InProcessConnector* create(
		    		SubscriptionManager* subscriptionManager,
		    		PublicationManager* publicationManager,
		    		InProcessPublicationSender* inProcessPublicationSender,
		    		const QString& proxyParticipantId,
		    		const QString& providerParticipantId,
		    		QSharedPointer<InProcessEndpointAddress> endpointAddress
			   	) {
		        return new «packagePrefix»::«interfaceName»InProcessConnector(
		        		subscriptionManager,
		        		publicationManager,
		        		inProcessPublicationSender,
		        		proxyParticipantId,
		        		providerParticipantId,
		        		endpointAddress
		        );
		    }
		};
		} // namespace joynr

		#endif // «headerGuard»
		'''
	}
	
}