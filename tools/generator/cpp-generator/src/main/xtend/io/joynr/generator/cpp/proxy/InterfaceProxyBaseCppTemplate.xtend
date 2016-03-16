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
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceProxyBaseCppTemplate extends InterfaceTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension BroadcastUtil
	@Inject private extension InterfaceUtil

	@Inject
	new(@Assisted FInterface francaIntf) {
		super(francaIntf)
	}

	override generate()
'''
«val serviceName =  francaIntf.joynrName»
«val className = serviceName + "ProxyBase"»
«warning()»

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«className».h"
#include "joynr/ConnectorFactory.h"
#include "joynr/ISubscriptionListener.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«serviceName»InProcessConnector.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«serviceName»JoynrMessagingConnector.h"

«getNamespaceStarter(francaIntf)»
«className»::«className»(
		std::shared_ptr<joynr::system::RoutingTypes::Address> messagingAddress,
		joynr::ConnectorFactory* connectorFactory,
		joynr::IClientCache *cache,
		const std::string &domain,
		const joynr::MessagingQos &qosSettings,
		bool cached
) :
		joynr::ProxyBase(connectorFactory, cache, domain, qosSettings, cached),
		messagingAddress(messagingAddress),
		connector()
{
}

//tm todo: this could probably moved into async proxy, by setting the IArbitrationListener in the ProxyBase
void «className»::handleArbitrationFinished(
		const std::string &providerParticipantId,
		const joynr::types::CommunicationMiddleware::Enum& connection
) {
	connector = connectorFactory->create<«getPackagePathWithJoynrPrefix(francaIntf, "::")»::I«serviceName»Connector>(
				domain,
				proxyParticipantId,
				providerParticipantId,
				qosSettings,
				cache,
				cached,
				connection
	);

	joynr::ProxyBase::handleArbitrationFinished(providerParticipantId, connection);
}

«FOR attribute: getAttributes(francaIntf).filter[attribute | attribute.notifiable]»
	«var attributeName = attribute.joynrName»
	«val returnType = attribute.typeName»
	void «className»::unsubscribeFrom«attributeName.toFirstUpper»(std::string& subscriptionId)
	{
		if (!connector){
			JOYNR_LOG_WARN(logger, "proxy cannot subscribe to «className».«attributeName», \
					 because the communication end partner is not (yet) known");
		}
		else{
			connector->unsubscribeFrom«attributeName.toFirstUpper»(subscriptionId);
		}
	}

	std::string «className»::subscribeTo«attributeName.toFirstUpper»(
				std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
				const joynr::SubscriptionQos& subscriptionQos,
				std::string& subscriptionId) {
		if (!connector){
			JOYNR_LOG_WARN(logger, "proxy cannot subscribe to «className».«attributeName», \
					 because the communication end partner is not (yet) known");
			return "";
		}
		else{
			return connector->subscribeTo«attributeName.toFirstUpper»(
						subscriptionListener,
						subscriptionQos,
						subscriptionId);
		}
	}

	std::string «className»::subscribeTo«attributeName.toFirstUpper»(
				std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
				const joynr::SubscriptionQos& subscriptionQos) {
		if (!connector){
			JOYNR_LOG_WARN(logger, "proxy cannot subscribe to «className».«attributeName», \
					 because the communication end partner is not (yet) known");
			return "";
		}
		else{
			return connector->subscribeTo«attributeName.toFirstUpper»(
						subscriptionListener,
						subscriptionQos);
		}
	}

«ENDFOR»

«FOR broadcast: francaIntf.broadcasts»
	«var broadcastName = broadcast.joynrName»
	«val returnTypes = broadcast.commaSeparatedOutputParameterTypes»
	void «className»::unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(std::string& subscriptionId)
	{
		if (!connector){
			JOYNR_LOG_WARN(logger, "proxy cannot unsubscribe from «className».«broadcastName» broadcast, \
					 because the communication end partner is not (yet) known");
			return;
		}
		else{
			connector->unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(subscriptionId);
		}
	}

	«IF isSelective(broadcast)»
		std::string «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					const «francaIntf.name.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
					std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
					const joynr::OnChangeSubscriptionQos& subscriptionQos) {
	«ELSE»
		std::string «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
					const joynr::OnChangeSubscriptionQos& subscriptionQos) {
	«ENDIF»
		if (!connector){
			JOYNR_LOG_WARN(logger, "proxy cannot subscribe to «className».«broadcastName» broadcast, \
					 because the communication end partner is not (yet) known");
			return "";
		}
		else{
			«IF isSelective(broadcast)»
				return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
						filterParameters,
						subscriptionListener,
						subscriptionQos);
			«ELSE»
				return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
						subscriptionListener,
						subscriptionQos);
			«ENDIF»
		}
	}

	«IF isSelective(broadcast)»
		std::string «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					const «francaIntf.name.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
					std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
					const joynr::OnChangeSubscriptionQos& subscriptionQos,
					std::string& subscriptionId) {
	«ELSE»
		std::string «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
					std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
					const joynr::OnChangeSubscriptionQos& subscriptionQos,
					std::string& subscriptionId) {
	«ENDIF»
		if (!connector){
			JOYNR_LOG_WARN(logger, "proxy cannot subscribe to «className».«broadcastName» broadcast, \
					 because the communication end partner is not (yet) known");
			return "";
		}
		else{
			«IF isSelective(broadcast)»
				return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
							filterParameters,
							subscriptionListener,
							subscriptionQos,
							subscriptionId);
			«ELSE»
				return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
							subscriptionListener,
							subscriptionQos,
							subscriptionId);
			«ENDIF»
		}
	}
«ENDFOR»

«getNamespaceEnder(francaIntf)»
'''
}
