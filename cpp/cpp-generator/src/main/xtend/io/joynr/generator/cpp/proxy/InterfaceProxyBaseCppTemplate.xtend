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
import org.franca.core.franca.FInterface
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.util.InterfaceTemplate

class InterfaceProxyBaseCppTemplate  implements InterfaceTemplate{
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase

	override generate(FInterface fInterface) {
		val serviceName =  fInterface.joynrName
		val className = serviceName + "ProxyBase"

		'''
		«warning()»

		#include "«getPackagePathWithJoynrPrefix(fInterface, "/")»/«className».h"
		#include "joynr/exceptions.h"
		#include "joynr/ConnectorFactory.h"
		#include "joynr/ISubscriptionListener.h"
		#include "«getPackagePathWithJoynrPrefix(fInterface, "/")»/«serviceName»InProcessConnector.h"
		#include "«getPackagePathWithJoynrPrefix(fInterface, "/")»/«serviceName»JoynrMessagingConnector.h"


		«getNamespaceStarter(fInterface)»
		«className»::«className»(
		        QSharedPointer<joynr::system::Address> messagingAddress,
		        joynr::ConnectorFactory* connectorFactory,
		        joynr::IClientCache *cache,
		        const QString &domain,
		        const joynr::ProxyQos &proxyQos,
		        const joynr::MessagingQos &qosSettings,
		        bool cached
		) :
		        joynr::ProxyBase(connectorFactory, cache, domain, interfaceName, proxyQos, qosSettings, cached),
		        messagingAddress(messagingAddress),
		        connector(NULL)
		{
		}

		//tm todo: this could probably moved into async proxy, by setting the IArbitrationListener in the ProxyBase
		void «className»::handleArbitrationFinished(
		        const QString &providerParticipantId,
		        const joynr::system::CommunicationMiddleware::Enum& connection
		) {
		    if (connector != NULL){
		        delete connector;
		    }
		    connector = connectorFactory->create<«getPackagePathWithJoynrPrefix(fInterface, "::")»::I«serviceName»Connector>(
		                domain,
		                proxyParticipantId,
		                providerParticipantId,
		                qosSettings,
		                cache,
		                cached,
		                proxyQos.getReqCacheDataFreshness_ms(),
		                connection
		    );

		    joynr::ProxyBase::handleArbitrationFinished(providerParticipantId, connection);
		}

		«FOR attribute: getAttributes(fInterface).filter[attribute | attribute.notifiable]»
			«var attributeName = attribute.joynrName»
			«val returnType = getMappedDatatypeOrList(attribute)»
			void «className»::unsubscribeFrom«attributeName.toFirstUpper»(QString& subscriptionId)
			{
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot subscribe to «className».«attributeName» \
			                     , because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->unsubscribeFrom«attributeName.toFirstUpper»(subscriptionId);
			    }
			}

			QString «className»::subscribeTo«attributeName.toFirstUpper»(
			            QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
			            QSharedPointer<joynr::SubscriptionQos> subscriptionQos,
			            QString& subscriptionId) {
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot subscribe to «className».«attributeName» \
			                     , because the communication end partner is not (yet) known");
			        return "";
			    }
			    else{
			        return connector->subscribeTo«attributeName.toFirstUpper»(
			                    subscriptionListener,
			                    subscriptionQos,
			                    subscriptionId);
			    }
			}

			QString «className»::subscribeTo«attributeName.toFirstUpper»(
			            QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
			            QSharedPointer<joynr::SubscriptionQos> subscriptionQos) {
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot subscribe to «className».«attributeName» \
			                     , because the communication end partner is not (yet) known");
			        return "";
			    }
			    else{
			        return connector->subscribeTo«attributeName.toFirstUpper»(
			                    subscriptionListener,
			                    subscriptionQos);
			    }
			}

		«ENDFOR»

		«FOR broadcast: fInterface.broadcasts»
			«var broadcastName = broadcast.joynrName»
			«val returnTypes = getMappedOutputParameterTypesCommaSeparated(broadcast)»
			void «className»::unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(QString& subscriptionId)
			{
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot unsubscribe from «className».«broadcastName» broadcast, because the communication end partner is not (yet) known");
			        return;
			    }
			    else{
			        connector->unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(subscriptionId);
			    }
			}

			«IF isSelective(broadcast)»
			QString «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
			            «fInterface.name.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters,
			            QSharedPointer<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
			            QSharedPointer<joynr::SubscriptionQos> subscriptionQos) {
			«ELSE»
			QString «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
			            QSharedPointer<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
			            QSharedPointer<joynr::SubscriptionQos> subscriptionQos) {
			«ENDIF»
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot subscribe to «className».«broadcastName» broadcast, because the communication end partner is not (yet) known");
			        return "";
			    }
			    else{
			        «IF isSelective(broadcast)»
			        return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(filterParameters, subscriptionListener, subscriptionQos);
			        «ELSE»
			        return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(subscriptionListener, subscriptionQos);
			        «ENDIF»
			    }
			}

		«ENDFOR»

		«className»::~«className»(){
		    if (connector != NULL){
		        delete connector;
		    }
		}
		«getNamespaceEnder(fInterface)»
		'''
	}
}
