package io.joynr.generator.cpp.proxy
/*
 * !!!
 *
 * Copyright (C) 2017 BMW Car IT GmbH
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
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.NamingUtil

class IInterfaceConnectorHTemplate extends InterfaceTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject private CppStdTypeUtil cppStdTypeUtil
	@Inject private extension NamingUtil

	@Inject extension InterfaceSubscriptionUtil
	override generate()
'''
«val interfaceName = francaIntf.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_")+
	"_I"+interfaceName+"Connector_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»
«FOR parameterType: cppStdTypeUtil.getDataTypeIncludesFor(francaIntf)»
	#include «parameterType»
«ENDFOR»

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/I«interfaceName».h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/SubscriptionCallback.h"
#include <memory>

namespace joynr {
	template <class ... Ts> class ISubscriptionListener;
	class ISubscriptionCallback;
	class SubscriptionQos;
	class OnChangeSubscriptionQos;
	class MulticastSubscriptionQos;
} // namespace joynr

«getNamespaceStarter(francaIntf)»

«FOR forwardDecl: cppStdTypeUtil.getBroadcastFilterParametersClassNames(francaIntf)»
	class «forwardDecl»;
«ENDFOR»

class «getDllExportMacro()» I«interfaceName»Subscription{
	/**
	  * in  - subscriptionListener      std::shared_ptr to a SubscriptionListener which will receive the updates.
	  * in  - subscriptionQos           SubscriptionQos parameters like interval and end date.
	  * out - assignedSubscriptionId    Buffer for the assigned subscriptionId.
	  */
public:
	virtual ~I«interfaceName»Subscription() = default;

	«produceSubscribeUnsubscribeMethodDeclarations(francaIntf, true)»
};

class «getDllExportMacro()» I«interfaceName»Connector: virtual public I«interfaceName», virtual public I«interfaceName»Subscription{

public:
	~I«interfaceName»Connector() override = default;
};

«getNamespaceEnder(francaIntf)»
#endif // «headerGuard»
'''
}
