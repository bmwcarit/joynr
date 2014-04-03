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

class InterfaceProxyBaseHTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase

	def generate(FInterface serviceInterface) {
		val interfaceName =  serviceInterface.joynrName
		val className = interfaceName + "ProxyBase"
		val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+"_"+interfaceName+"ProxyBase_h").toUpperCase
		'''
		«warning()»
		
		#ifndef «headerGuard»
		#define «headerGuard»

		#include "joynr/PrivateCopyAssign.h"
		«getDllExportIncludeStatement()»
		#include "joynr/ProxyBase.h"
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName»Connector.h"
		
		namespace joynr {
			class ICapabilities;
		}
		
		«getNamespaceStarter(serviceInterface)» 
		class «getDllExportMacro()» «className»: virtual public joynr::ProxyBase, virtual public «getPackagePathWithJoynrPrefix(serviceInterface, "::")»::I«interfaceName»Subscription {
		public:
		    «className»(
		            joynr::ICapabilities* capabilitiesStub,
		            QSharedPointer<joynr::system::Address> messagingAddress,
		            joynr::ConnectorFactory* connectorFactory,
		            joynr::IClientCache* cache,
		            const QString& domain,
		            const joynr::ProxyQos& proxyQos,
		            const joynr::MessagingQos& qosSettings,
		            bool cached
		    );
		
		    ~«className»();
		
		    void handleArbitrationFinished(const QString &participantId, QSharedPointer<joynr::system::Address> endpointAddress);
			«FOR attribute: getAttributes(serviceInterface)»
				«val returnType = getMappedDatatypeOrList(attribute)»
				«var attributeName = attribute.joynrName»
				QString subscribeTo«attributeName.toFirstUpper»(QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener, QSharedPointer<joynr::SubscriptionQos> subscriptionQos);
				void unsubscribeFrom«attributeName.toFirstUpper»(QString& subscriptionId);
			«ENDFOR»

		protected:
			joynr::ICapabilities* capabilitiesStub;
			QSharedPointer<joynr::system::Address> messagingAddress; 
		    I«interfaceName»Connector* connector;

		private:
		    DISALLOW_COPY_AND_ASSIGN(«className»);
		};
		«getNamespaceEnder(serviceInterface)»
		#endif // «headerGuard»
		'''	
	}	
	
			
}