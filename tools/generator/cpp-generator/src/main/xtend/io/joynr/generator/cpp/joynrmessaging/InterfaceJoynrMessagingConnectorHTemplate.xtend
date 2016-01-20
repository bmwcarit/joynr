package io.joynr.generator.cpp.joynrmessaging
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
import io.joynr.generator.cpp.util.CppInterfaceUtil
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceJoynrMessagingConnectorHTemplate implements InterfaceTemplate{

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension BroadcastUtil
	@Inject private extension CppInterfaceUtil
	@Inject private extension JoynrCppGeneratorExtensions

	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+
	"_"+interfaceName+"JoynrMessagingConnector_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»
«FOR parameterType: getRequiredIncludesFor(serviceInterface).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»

#include <memory>
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName»Connector.h"
#include "joynr/AbstractJoynrMessagingConnector.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/SubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"

namespace joynr {
	class MessagingQos;
	class IJoynrMessageSender;
	class ISubscriptionManager;
	template <class ... Ts> class Future;

namespace exceptions
{
	class JoynrException;
	class JoynrRuntimeException;
} // namespace exceptions
}

«getNamespaceStarter(serviceInterface)»


/** @brief JoynrMessagingConnector for interface «interfaceName» */
class «getDllExportMacro()» «interfaceName»JoynrMessagingConnector : public I«interfaceName»Connector, virtual public joynr::AbstractJoynrMessagingConnector {
private:
	«FOR attribute: getAttributes(serviceInterface)»
		«val returnType = attribute.typeName»
		«val attributeName = attribute.joynrName»
		«IF attribute.notifiable»
			/**
			 * @brief creates a new subscription or updates an existing subscription to attribute 
			 * «attributeName.toFirstUpper»
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @param subscriptionRequest The subscription request
			 * @return the subscription id as string
			 */
			std::string subscribeTo«attributeName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					const joynr::SubscriptionQos& subscriptionQos,
					SubscriptionRequest& subscriptionRequest);
		«ENDIF»
	«ENDFOR»
	«FOR broadcast: serviceInterface.broadcasts»
		«val returnTypes = broadcast.commaSeparatedOutputParameterTypes»
		«val broadcastName = broadcast.joynrName»
		/**
		 * @brief subscribes to broadcast «broadcastName.toFirstUpper»
		 * @param subscriptionListener The listener callback providing methods to call on publication and failure
		 * @param subscriptionQos The subscription quality of service settings
		 * @param subscriptionRequest The subscription request
		 * @return the subscription id as string
		 */
		std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
				std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				const joynr::OnChangeSubscriptionQos& subscriptionQos,
				BroadcastSubscriptionRequest& subscriptionRequest);
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
	 * @param cache Pointer to the client cache instance
	 * @param cached True, if entries are cached, false otherwise
	 */
	«interfaceName»JoynrMessagingConnector(
		joynr::IJoynrMessageSender* messageSender,
		joynr::ISubscriptionManager* subscriptionManager,
		const std::string &domain,
		const std::string proxyParticipantId,
		const std::string& providerParticipantId,
		const joynr::MessagingQos &qosSettings,
		joynr::IClientCache *cache,
		bool cached);

	/**
	 * @brief Checks whether cluster controller is used
	 * @return true, if cluster controller is used
	 */
	bool usesClusterController() const override;

	/** @brief Destructor */
	~«interfaceName»JoynrMessagingConnector() override = default;

	«produceSyncGetterDeclarations(serviceInterface, false)»
	«produceAsyncGetterDeclarations(serviceInterface, false)»
	«produceSyncSetterDeclarations(serviceInterface, false)»
	«produceAsyncSetterDeclarations(serviceInterface, false)»
	«FOR attribute: getAttributes(serviceInterface)»
		«val returnType = attribute.typeName»
		«val attributeName = attribute.joynrName»
		«IF attribute.notifiable»
			/**
			 * @brief creates a new subscription subscription to attribute 
			 * «attributeName.toFirstUpper»
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @return the subscription id as string
			 */
			std::string subscribeTo«attributeName.toFirstUpper»(
						std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
						const joynr::SubscriptionQos& subscriptionQos) override;

			/**
			 * @brief updates an existing subscription to attribute 
			 * «attributeName.toFirstUpper»
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @return the subscription id as string
			 */
			std::string subscribeTo«attributeName.toFirstUpper»(
						std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
						const joynr::SubscriptionQos& subscriptionQos,
						std::string& subscriptionId) override;

			/**
			 * @brief unsubscribes from attribute «attributeName.toFirstUpper»
			 * @param subscriptionId The subscription id returned earlier on creation of the subscription
			 */
			void unsubscribeFrom«attributeName.toFirstUpper»(std::string& subscriptionId) override;
		«ENDIF»
	«ENDFOR»

	«produceSyncMethodDeclarations(serviceInterface, false)»
	«produceAsyncMethodDeclarations(serviceInterface, false, true)»

	«FOR broadcast: serviceInterface.broadcasts»

		«val returnTypes = broadcast.commaSeparatedOutputParameterTypes»
		«val broadcastName = broadcast.joynrName»
		«IF isSelective(broadcast)»
			/**
			 * @brief subscribes to selective broadcast «broadcastName.toFirstUpper» with filter parameters
			 * @param filterParameters The filter parameters for selection of suitable broadcasts
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @return the subscription id as string
			 */
			std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						const «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::OnChangeSubscriptionQos& subscriptionQos) override;

			/**
			 * @brief updates an existing subscription to selective broadcast «broadcastName.toFirstUpper» with filter parameters
			 * @param filterParameters The filter parameters for selection of suitable broadcasts
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @param subscriptionId The subscription id returned earlier on creation of the subscription
			 * @return the subscription id as string
			 */
			std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						const «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::OnChangeSubscriptionQos& subscriptionQos,
						std::string& subscriptionId) override;
		«ELSE»
			/**
			 * @brief subscribes to broadcast «broadcastName.toFirstUpper»
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @return the subscription id as string
			 */
			std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::OnChangeSubscriptionQos& subscriptionQos) override;

			/**
			 * @brief updates an existing subscription to broadcast «broadcastName.toFirstUpper»
			 * @param filterParameters The filter parameters for selection of suitable broadcasts
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @param subscriptionId The subscription id returned earlier on creation of the subscription
			 * @return the subscription id as string
			 */
			std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::OnChangeSubscriptionQos& subscriptionQos,
						std::string& subscriptionId) override;
		«ENDIF»

		/**
		 * @brief unsubscribes from broadcast «broadcastName.toFirstUpper»
		 * @param subscriptionId The subscription id returned earlier on creation of the subscription
		 */
		void unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(std::string& subscriptionId) override;
	«ENDFOR»
};
«getNamespaceEnder(serviceInterface)»

namespace joynr {

// Helper class for use by the JoynrMessagingConnectorFactory
// This class creates instances of «interfaceName»JoynrMessagingConnector
template <>
class JoynrMessagingConnectorFactoryHelper <«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::I«interfaceName»Connector> {
public:
	«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»JoynrMessagingConnector* create(
			joynr::IJoynrMessageSender* messageSender,
			joynr::ISubscriptionManager* subscriptionManager,
			const std::string &domain,
			const std::string proxyParticipantId,
			const std::string& providerParticipantId,
			const joynr::MessagingQos &qosSettings,
			joynr::IClientCache *cache,
			bool cached
	) {
		return new «getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»JoynrMessagingConnector(
					messageSender,
					subscriptionManager,
					domain,
					proxyParticipantId,
					providerParticipantId,
					qosSettings,
					cache,
					cached
		);
	}
};

} // namespace joynr
#endif // «headerGuard»
'''
}
