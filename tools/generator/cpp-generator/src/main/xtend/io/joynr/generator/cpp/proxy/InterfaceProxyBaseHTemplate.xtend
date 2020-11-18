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
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil

class InterfaceProxyBaseHTemplate extends InterfaceTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject extension InterfaceSubscriptionUtil
	@Inject extension NamingUtil

	override generate(boolean generateVersion)
'''
«val interfaceName =  francaIntf.joynrName»
«val className = interfaceName + "ProxyBase"»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_", generateVersion)+
	"_"+interfaceName+"ProxyBase_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

#include "joynr/PrivateCopyAssign.h"
«FOR parameterType: getDataTypeIncludesFor(francaIntf, generateVersion).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»
#include <memory>

«getDllExportIncludeStatement()»
#include "joynr/ProxyBase.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/I«interfaceName»Connector.h"

namespace joynr
{
namespace types
{
	class DiscoveryEntryWithMetaInfo;
} // namespace types
} // namespace joynr

«getNamespaceStarter(francaIntf, generateVersion)»
/**
 * @brief Proxy base class for interface «interfaceName»
 *
 * @version «majorVersion».«minorVersion»
 */
class «getDllExportMacro()» «className»: virtual public joynr::ProxyBase, virtual public «getPackagePathWithJoynrPrefix(francaIntf, "::", generateVersion)»::I«interfaceName»Subscription {
public:
	/**
	 * @brief Parameterized constructor
	 * @param connectorFactory The connector factory
	 * @param domain The provider domain
	 * @param qosSettings The quality of service settings
	 */
	«className»(
			std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
			std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
			const std::string& domain,
			const joynr::MessagingQos& qosSettings
	);

	/**
	 * @brief Called when arbitration is finished
	 * @param participantId The id of the participant
	 * @param connection The kind of connection
	 */
	void handleArbitrationFinished(
			const joynr::types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry
	) override;

	«produceSubscribeUnsubscribeMethodDeclarations(francaIntf, false, generateVersion)»

protected:
	/** @brief The kind of connector */
	std::unique_ptr<I«interfaceName»Connector> connector;

private:
	DISALLOW_COPY_AND_ASSIGN(«className»);
};
«getNamespaceEnder(francaIntf, generateVersion)»
#endif // «headerGuard»
'''
}
