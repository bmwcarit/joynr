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
import io.joynr.generator.cpp.util.InterfaceUtil
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions

class InterfaceAsyncProxyHTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	
	@Inject extension InterfaceUtil
	
	def generate(FInterface serviceInterface) {
		val interfaceName =  serviceInterface.joynrName
		val className = interfaceName + "Proxy"
		val asyncClassName = interfaceName + "AsyncProxy"
		val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+"_"+interfaceName+"AsyncProxy_h").toUpperCase
		
		'''
		«warning()»
		
		#ifndef «headerGuard»
		#define «headerGuard»

		#include "joynr/PrivateCopyAssign.h"
		«getDllExportIncludeStatement()»
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«className»Base.h"
		
		«getNamespaceStarter(serviceInterface)» 
		class «getDllExportMacro()» «asyncClassName»: virtual public «className»Base, virtual public I«interfaceName»Async {
		public:
		    «asyncClassName»(
		            joynr::ICapabilities* capabilitiesStub,
		            QSharedPointer<joynr::system::Address> messagingAddress,
		            joynr::ConnectorFactory* connectorFactory,
		            joynr::IClientCache* cache,
		            const QString& domain,
		            const joynr::ProxyQos& proxyQos,
		            const joynr::MessagingQos& qosSettings,
		            bool cached
		    );

			«produceAsyncGetters(serviceInterface, false)»
			«produceAsyncSetters(serviceInterface, false)»
			«produceAsyncMethods(serviceInterface, false)»
		
		    friend class «className»;
		
		private:
		    DISALLOW_COPY_AND_ASSIGN(«asyncClassName»);
		};
		«getNamespaceEnder(serviceInterface)»
		#endif // «headerGuard»
		
		'''	
	}	
	
			
}