package io.joynr.generator.cpp.proxy
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
import com.google.inject.assistedinject.Assisted
import io.joynr.generator.cpp.util.CppInterfaceUtil
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceAsyncProxyHTemplate extends InterfaceTemplate {
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil

	@Inject extension CppInterfaceUtil
	@Inject private extension NamingUtil

	@Inject
	new(@Assisted FInterface francaIntf) {
		super(francaIntf)
	}

	override generate()
'''
«val interfaceName =  serviceInterface.joynrName»
«val className = interfaceName + "Proxy"»
«val asyncClassName = interfaceName + "AsyncProxy"»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+
	"_"+interfaceName+"AsyncProxy_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

#include "joynr/PrivateCopyAssign.h"
«getDllExportIncludeStatement()»
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«className»Base.h"

namespace joynr
{
	template <class ... Ts> class Future;

namespace exceptions
{
	class JoynrException;
	class JoynrRuntimeException;
} // namespace exceptions
} // namespace joynr

«FOR parameterType: getRequiredIncludesFor(serviceInterface).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»

#include <memory>

«getNamespaceStarter(serviceInterface)»
/** @brief proxy class for asynchronous calls of interface «interfaceName»
 *
 * @version «majorVersion».«minorVersion»
 */
class «getDllExportMacro()» «asyncClassName»: virtual public «className»Base, virtual public I«interfaceName»Async {
public:

	/**
	 * @brief Parameterized constructor
	 * @param messagingAddress The messaging address
	 * @param connectorFactory The connector factory
	 * @param cache The client cache
	 * @param domain The provider domain
	 * @param qosSettings The quality of service settings
	 * @param cached True, if cached, false otherwise
	 */
	«asyncClassName»(
			std::shared_ptr<joynr::system::RoutingTypes::Address> messagingAddress,
			joynr::ConnectorFactory* connectorFactory,
			joynr::IClientCache* cache,
			const std::string& domain,
			const joynr::MessagingQos& qosSettings,
			bool cached
	);

	«produceAsyncGetterDeclarations(serviceInterface, false)»
	«produceAsyncSetterDeclarations(serviceInterface, false)»
	«produceAsyncMethodDeclarations(serviceInterface, false, true)»

	friend class «className»;

private:
	DISALLOW_COPY_AND_ASSIGN(«asyncClassName»);
};
«getNamespaceEnder(serviceInterface)»
#endif // «headerGuard»

'''
}
