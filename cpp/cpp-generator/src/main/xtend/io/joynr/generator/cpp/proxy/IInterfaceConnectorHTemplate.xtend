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
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions

class IInterfaceConnectorHTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	
	@Inject extension InterfaceSubscriptionUtil
	def generate(FInterface serviceInterface) {
		val interfaceName = serviceInterface.joynrName
		val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+"_I"+interfaceName+"Connector_h").toUpperCase
		'''
		«warning()»
		
		#ifndef «headerGuard»
		#define «headerGuard»
		
		«getDllExportIncludeStatement()»
		«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
		#include "«parameterType»"
		«ENDFOR»
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName».h"
		#include "joynr/ISubscriptionListener.h"
		#include "joynr/IConnector.h"

		namespace joynr {
			template <class T, class... Ts> class ISubscriptionListener;
			class ISubscriptionCallback;
			class SubscriptionQos;
		}
		
		«getNamespaceStarter(serviceInterface)»
		class «getDllExportMacro()» I«interfaceName»Subscription{
		    /**
		      * in  - subscriptionListener      QSharedPointer to a SubscriptionListener which will receive the updates.
		      * in  - subscriptionQos           SubscriptionQos parameters like interval and end date.
		      * out - assignedSubscriptionId    Buffer for the assigned subscriptionId.
		      */
		public:
		    virtual ~I«interfaceName»Subscription() {}
		    
		    «produceSubscribeUnsubscribeMethods(serviceInterface, true)»
		};
		
		class «getDllExportMacro()» I«interfaceName»Connector: virtual public I«interfaceName», public joynr::IConnector, virtual public I«interfaceName»Subscription{
		
		public:
		    virtual ~I«interfaceName»Connector() {}
		};
		
		«getNamespaceEnder(serviceInterface)»
		#endif // «headerGuard»
		'''
	}
}
