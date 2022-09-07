package io.joynr.generator.cpp.proxy
/*
 * !!!
 *
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
import io.joynr.generator.cpp.util.CppInterfaceUtil
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.NamingUtil

class InterfaceAsyncProxyHTemplate extends InterfaceTemplate {
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil

	@Inject extension CppInterfaceUtil
	@Inject extension NamingUtil

	override generate(boolean generateVersion)
'''
«val interfaceName =  francaIntf.joynrName»
«val className = interfaceName + "Proxy"»
«val asyncClassName = interfaceName + "AsyncProxy"»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_", generateVersion)+
	"_"+interfaceName+"AsyncProxy_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

#include "joynr/PrivateCopyAssign.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«className»Base.h"
«IF hasFireAndForgetMethods(francaIntf)»
	#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«interfaceName»FireAndForgetProxy.h"
«ENDIF»

namespace joynr
{
	template <class ... Ts> class Future;

namespace exceptions
{
	class JoynrException;
	class JoynrRuntimeException;
} // namespace exceptions
} // namespace joynr

«FOR parameterType: getDataTypeIncludesFor(francaIntf, generateVersion).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»

#include <memory>

«getNamespaceStarter(francaIntf, generateVersion)»
/** @brief proxy class for asynchronous calls of interface «interfaceName»
 *
 * @version «majorVersion».«minorVersion»
 */
class «asyncClassName» :
		virtual public «className»Base,
		virtual public I«interfaceName»Async«IF hasFireAndForgetMethods(francaIntf)»,
		virtual public «interfaceName»FireAndForgetProxy«ENDIF»
{
public:

	/**
	 * @brief Parameterized constructor
	 * @param connectorFactory The connector factory
	 * @param domain The provider domain
	 * @param qosSettings The quality of service settings
	 */
	«asyncClassName»(
			std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
			std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
			const std::string& domain,
			const joynr::MessagingQos& qosSettings
	);

	«produceAsyncGetterDeclarations(francaIntf, false, generateVersion)»
	«produceAsyncSetterDeclarations(francaIntf, false, generateVersion)»
	«produceAsyncMethodDeclarations(francaIntf, false, true, generateVersion)»

	friend class «className»;

private:
	DISALLOW_COPY_AND_ASSIGN(«asyncClassName»);
};
«getNamespaceEnder(francaIntf, generateVersion)»
#endif // «headerGuard»

'''
}
