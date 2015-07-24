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
import io.joynr.generator.cpp.util.CppMigrateToStdTypeUtil
import io.joynr.generator.cpp.util.InterfaceUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.util.InterfaceTemplate
import org.franca.core.franca.FInterface

class InterfaceJoynrMessagingConnectorHTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension CppMigrateToStdTypeUtil

	@Inject
	private extension InterfaceUtil

	@Inject
	private extension JoynrCppGeneratorExtensions

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
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName»Connector.h"
#include "joynr/AbstractJoynrMessagingConnector.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/StdSubscriptionQos.h"
#include "joynr/StdOnChangeSubscriptionQos.h"

namespace joynr {
	class MessagingQos;
	class IJoynrMessageSender;
	class ISubscriptionManager;
}

«getNamespaceStarter(serviceInterface)»


class «getDllExportMacro()» «interfaceName»JoynrMessagingConnector : public I«interfaceName»Connector, virtual public joynr::AbstractJoynrMessagingConnector {
private:
	«FOR attribute: getAttributes(serviceInterface)»
		«val returnType = attribute.typeName»
		«val attributeName = attribute.joynrName»
		«IF attribute.notifiable»
			std::string subscribeTo«attributeName.toFirstUpper»(
					QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					QSharedPointer<joynr::StdSubscriptionQos> subscriptionQos,
					SubscriptionRequest& subscriptionRequest);
		«ENDIF»
	«ENDFOR»
	«FOR broadcast: serviceInterface.broadcasts»
		«val returnTypes = broadcast.commaSeparatedOutputParameterTypes»
		«val broadcastName = broadcast.joynrName»
		std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
				QSharedPointer<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				QSharedPointer<joynr::StdOnChangeSubscriptionQos> subscriptionQos,
				BroadcastSubscriptionRequest& subscriptionRequest);
	«ENDFOR»
public:
	«interfaceName»JoynrMessagingConnector(
		joynr::IJoynrMessageSender* messageSender,
		joynr::ISubscriptionManager* subscriptionManager,
		const std::string &domain,
		const std::string proxyParticipantId,
		const std::string& providerParticipantId,
		const joynr::MessagingQos &qosSettings,
		joynr::IClientCache *cache,
		bool cached);

	virtual bool usesClusterController() const;

	virtual ~«interfaceName»JoynrMessagingConnector(){}

	«produceSyncGetters(serviceInterface, false)»
	«produceAsyncGetters(serviceInterface, false)»
	«produceSyncSetters(serviceInterface, false)»
	«produceAsyncSetters(serviceInterface, false)»
	«FOR attribute: getAttributes(serviceInterface)»
		«val returnType = attribute.typeName»
		«val attributeName = attribute.joynrName»
		«IF attribute.notifiable»
			virtual std::string subscribeTo«attributeName.toFirstUpper»(
						QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
						QSharedPointer<joynr::StdSubscriptionQos> subscriptionQos);
			virtual std::string subscribeTo«attributeName.toFirstUpper»(
						QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
						QSharedPointer<joynr::StdSubscriptionQos> subscriptionQos,
						std::string& subscriptionId);
			virtual void unsubscribeFrom«attributeName.toFirstUpper»(std::string& subscriptionId);
		«ENDIF»
	«ENDFOR»

	«produceSyncMethods(serviceInterface, false)»
	«produceAsyncMethods(serviceInterface, false)»

	«FOR broadcast: serviceInterface.broadcasts»

		«val returnTypes = broadcast.commaSeparatedOutputParameterTypes»
		«val broadcastName = broadcast.joynrName»
		«IF isSelective(broadcast)»
			virtual std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						«interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters,
						QSharedPointer<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						QSharedPointer<joynr::StdOnChangeSubscriptionQos> subscriptionQos);
			virtual std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						«interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters,
						QSharedPointer<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						QSharedPointer<joynr::StdOnChangeSubscriptionQos> subscriptionQos,
						std::string& subscriptionId);
		«ELSE»
			virtual std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						QSharedPointer<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						QSharedPointer<joynr::StdOnChangeSubscriptionQos> subscriptionQos);
			virtual std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						QSharedPointer<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						QSharedPointer<joynr::StdOnChangeSubscriptionQos> subscriptionQos,
						std::string& subscriptionId);
		«ENDIF»
		virtual void unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(std::string& subscriptionId);
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
