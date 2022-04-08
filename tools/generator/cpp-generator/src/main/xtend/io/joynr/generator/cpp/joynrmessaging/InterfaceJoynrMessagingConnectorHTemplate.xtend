package io.joynr.generator.cpp.joynrmessaging
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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil

class InterfaceJoynrMessagingConnectorHTemplate extends InterfaceTemplate{

	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject extension NamingUtil
	@Inject extension AttributeUtil
	@Inject extension CppInterfaceUtil
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension InterfaceSubscriptionUtil

	override generate(boolean generateVersion)
'''
«val interfaceName = francaIntf.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_", generateVersion)+
	"_"+interfaceName+"JoynrMessagingConnector_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»
«FOR parameterType: getDataTypeIncludesFor(francaIntf, generateVersion).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»

#include <memory>
#include <functional>
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/I«interfaceName»Connector.h"
#include "joynr/AbstractJoynrMessagingConnector.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/SubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/MulticastSubscriptionQos.h"

namespace joynr {
	class MessagingQos;
	class IMessageSender;
	class ISubscriptionManager;
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
	template <typename T, typename... Ts> class ISubscriptionListener;

namespace types
{
	class DiscoveryEntryWithMetaInfo;
} // namespace types

namespace exceptions
{
	class JoynrException;
	class JoynrRuntimeException;
} // namespace exceptions
}

«getNamespaceStarter(francaIntf, generateVersion)»


/** @brief JoynrMessagingConnector for interface «interfaceName» */
class «getDllExportMacro()» «interfaceName»JoynrMessagingConnector : public I«interfaceName»Connector, virtual public joynr::AbstractJoynrMessagingConnector {
private:
	«FOR attribute: getAttributes(francaIntf)»
		«val returnType = attribute.getTypeName(generateVersion)»
		«val attributeName = attribute.joynrName»
		«IF attribute.notifiable»
			/**
			 * @brief creates a new subscription or updates an existing subscription to attribute 
			 * «attributeName.toFirstUpper»
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @param subscriptionRequest The subscription request
			 * @return a future representing the result (subscription id) as string. It provides methods to wait for
			 			 * completion, to get the subscription id or the request status object. The subscription id will be available
			 			 * when the subscription is successfully registered at the provider.
			 */
			std::shared_ptr<joynr::Future<std::string>> subscribeTo«attributeName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					std::shared_ptr<joynr::SubscriptionQos> subscriptionQos,
					SubscriptionRequest& subscriptionRequest);
		«ENDIF»
	«ENDFOR»
	«FOR broadcast: francaIntf.broadcasts»
		«val returnTypes = broadcast.getCommaSeparatedOutputParameterTypes(generateVersion)»
		«val broadcastName = broadcast.joynrName»
		/**
		 * @brief subscribes to broadcast «broadcastName.toFirstUpper»
		 * @param subscriptionListener The listener callback providing methods to call on publication and failure
		 * @param subscriptionQos The subscription quality of service settings
		 * @param subscriptionRequest The subscription request
		 * @return a future representing the result (subscription id) as string. It provides methods to wait for
		 			 * completion, to get the subscription id or the request status object. The subscription id will be available
		 			 * when the subscription is successfully registered at the provider.
		 */
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
	 * @param messageSender The message sender
	 * @param subscriptionManager Pointer to subscription manager instance
	 * @param domain the provider domain
	 * @param proxyParticipantId The participant id of the proxy
	 * @param providerParticipantId The participant id of the provider
	 * @param qosSettings The quality of service settings
	 */
	«interfaceName»JoynrMessagingConnector(
		std::weak_ptr<joynr::IMessageSender> messageSender,
		std::weak_ptr<joynr::ISubscriptionManager> subscriptionManager,
		const std::string& domain,
		const std::string& proxyParticipantId,
		const joynr::MessagingQos &qosSettings,
		const joynr::types::DiscoveryEntryWithMetaInfo& providerDiscoveryInfo);

	/** @brief Destructor */
	~«interfaceName»JoynrMessagingConnector() override = default;

	«produceSyncGetterDeclarations(francaIntf, false, generateVersion)»
	«produceAsyncGetterDeclarations(francaIntf, false, generateVersion)»
	«produceSyncSetterDeclarations(francaIntf, false, generateVersion)»
	«produceAsyncSetterDeclarations(francaIntf, false, generateVersion)»

	«produceSyncMethodDeclarations(francaIntf, false, generateVersion)»
	«produceAsyncMethodDeclarations(francaIntf, false, true, generateVersion)»
	«produceFireAndForgetMethodDeclarations(francaIntf, false, generateVersion)»

	«produceSubscribeUnsubscribeMethodDeclarations(francaIntf, false, true, generateVersion)»
};
«getNamespaceEnder(francaIntf, generateVersion)»

«var packagePrefix = getPackagePathWithJoynrPrefix(francaIntf, "::", generateVersion)»

namespace joynr {

// specialization of traits class JoynrMessagingTraits
// this links I«interfaceName»Connector with «interfaceName»JoynrMessagingConnector
template <>
struct JoynrMessagingTraits<«packagePrefix»::I«interfaceName»Connector>
{
	using Connector = «packagePrefix»::«interfaceName»JoynrMessagingConnector;
};

} // namespace joynr
#endif // «headerGuard»
'''
}
