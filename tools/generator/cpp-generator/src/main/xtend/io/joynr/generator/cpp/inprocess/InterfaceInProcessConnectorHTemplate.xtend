package io.joynr.generator.cpp.inprocess
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
import io.joynr.generator.cpp.util.CppInterfaceUtil
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.NamingUtil

class InterfaceInProcessConnectorHTemplate extends InterfaceTemplate{

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension CppInterfaceUtil
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension InterfaceSubscriptionUtil

	override generate()
'''
«val interfaceName = francaIntf.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_")+
	"_"+interfaceName+"InProcessConnector_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

#include "joynr/PrivateCopyAssign.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/I«interfaceName»Connector.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/InProcessConnectorFactory.h"
#include "joynr/SubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Logger.h"

«FOR parameterType: getDataTypeIncludesFor(francaIntf).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»
#include <memory>
#include <functional>

namespace joynr {
	class InProcessAddress;
	class ISubscriptionManager;
	class PublicationManager;
	class IPlatformSecurityManager;
	«IF !francaIntf.attributes.empty»
		class SubscriptionRequest;
	«ENDIF»
	«IF !francaIntf.broadcasts.filter[selective].empty»
		class BroadcastSubscriptionRequest;
	«ENDIF»
	«IF !francaIntf.broadcasts.filter[!selective].empty»
		class MulticastSubscriptionRequest;
	«ENDIF»
	template <class ... Ts> class Future;
	template <typename... Ts> class ISubscriptionListener;

namespace exceptions
{
	class JoynrException;
	class JoynrRuntimeException;
} // namespace exceptions

} // namespace joynr

«getNamespaceStarter(francaIntf)»

/** @brief InProcessConnector class for interface «interfaceName» */
class «interfaceName»InProcessConnector : public I«interfaceName»Connector {
private:
«FOR attribute: getAttributes(francaIntf).filter[attribute | attribute.notifiable]»
	«val returnType = attribute.typeName»
	std::shared_ptr<joynr::Future<std::string>> subscribeTo«attribute.joynrName.toFirstUpper»(
				std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
				std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
				SubscriptionRequest& subscriptionRequest);
«ENDFOR»
«FOR broadcast: francaIntf.broadcasts»
	«val returnTypes = broadcast.commaSeparatedOutputParameterTypes»
	«val broadcastName = broadcast.joynrName»
	std::shared_ptr<joynr::Future<std::string>> subscribeTo«broadcastName.toFirstUpper»Broadcast(
			std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
			«IF broadcast.selective»
				std::shared_ptr<joynr::OnChangeSubscriptionQos> subscriptionQos,
				BroadcastSubscriptionRequest& subscriptionRequest);
			«ELSE»
				std::shared_ptr<joynr::MulticastSubscriptionQos> subscriptionQos,
				std::shared_ptr<MulticastSubscriptionRequest> subscriptionRequest,
				const std::vector<std::string>& partitions);
			«ENDIF»
«ENDFOR»
public:

	/**
	 * @brief Parameterized constructor
	 * @param subscriptionManager Subscription manager instance
	 * @param publicationManager Publication manager instance
	 * @param inProcessPublicationSender InProcessPublicationSender instance,
	 * used to transfer publications from the PublicationManager to the (local) SubscriptionManager.
	 * @param proxyParticipantId The participant id of the proxy
	 * @param providerParticipantId The participant id of the provider
	 * @param address The address
	 */
	«interfaceName»InProcessConnector(
				joynr::ISubscriptionManager* subscriptionManager,
				joynr::PublicationManager* publicationManager,
				joynr::InProcessPublicationSender* inProcessPublicationSender,
				std::shared_ptr<joynr::IPlatformSecurityManager> securityManager,
				const std::string& proxyParticipantId,
				const std::string& providerParticipantId,
				std::shared_ptr<joynr::InProcessAddress> address
	);

	«produceSyncGetterDeclarations(francaIntf, false)»
	«produceSyncSetterDeclarations(francaIntf, false)»
	«produceSyncMethodDeclarations(francaIntf, false)»
	«produceAsyncGetterDeclarations(francaIntf, false)»
	«produceAsyncSetterDeclarations(francaIntf, false)»
	«produceAsyncMethodDeclarations(francaIntf, false, true)»
	«produceFireAndForgetMethodDeclarations(francaIntf, false)»
	«produceSubscribeUnsubscribeMethodDeclarations(francaIntf, false)»

private:
	ADD_LOGGER(«interfaceName»InProcessConnector);

	DISALLOW_COPY_AND_ASSIGN(«interfaceName»InProcessConnector);
	std::string proxyParticipantId;
	std::string providerParticipantId;
	std::shared_ptr<joynr::InProcessAddress> address;
	joynr::ISubscriptionManager* subscriptionManager;
	joynr::PublicationManager* publicationManager;
	joynr::InProcessPublicationSender* inProcessPublicationSender;
	std::shared_ptr<joynr::IPlatformSecurityManager> securityManager;
};
«getNamespaceEnder(francaIntf)»

«var packagePrefix = getPackagePathWithJoynrPrefix(francaIntf, "::")»

namespace joynr {

// specialization of traits class InProcessTraits
// this links I«interfaceName»Connector with «interfaceName»InProcessConnector
template <>
struct InProcessTraits<«packagePrefix»::I«interfaceName»Connector>
{
	using Connector = «packagePrefix»::«interfaceName»InProcessConnector;
};

} // namespace joynr

#endif // «headerGuard»
'''

}
