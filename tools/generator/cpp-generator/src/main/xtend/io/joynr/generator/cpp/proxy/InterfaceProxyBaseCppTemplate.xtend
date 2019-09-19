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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil

class InterfaceProxyBaseCppTemplate extends InterfaceTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension InterfaceSubscriptionUtil
	@Inject extension NamingUtil
	@Inject extension AttributeUtil
	@Inject extension InterfaceUtil

	override generate()
'''
«val serviceName =  francaIntf.joynrName»
«val className = serviceName + "ProxyBase"»
«warning()»

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«className».h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«serviceName»JoynrMessagingConnector.h"

«getNamespaceStarter(francaIntf)»
«className»::«className»(
		std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
		std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
		const std::string &domain,
		const joynr::MessagingQos &qosSettings
) :
		joynr::ProxyBase(std::move(runtime), std::move(connectorFactory), domain, qosSettings),
		connector()
{
}

void «className»::handleArbitrationFinished(
		const joynr::types::DiscoveryEntryWithMetaInfo& providerDiscoveryEntry
) {
	connector = _connectorFactory->create<«getPackagePathWithJoynrPrefix(francaIntf, "::")»::I«serviceName»Connector>(
				_domain,
				_proxyParticipantId,
				_qosSettings,
				providerDiscoveryEntry
	);

	joynr::ProxyBase::handleArbitrationFinished(providerDiscoveryEntry);
}

«FOR attribute: getAttributes(francaIntf).filter[attribute | attribute.notifiable]»
	«var attributeName = attribute.joynrName»
	«produceUnsubscribeFromAttributeSignature(attribute, className)»
	{
		auto runtimeSharedPtr = _runtime.lock();
		if (!runtimeSharedPtr || !connector) {
			if (!runtimeSharedPtr) {
				JOYNR_LOG_WARN(logger(), "proxy cannot unsubscribe from «className».«attributeName», "
						 "because the required runtime has been already destroyed.");
				return;
			} else {
				JOYNR_LOG_WARN(logger(), "proxy cannot unsubscribe from «className».«attributeName», "
						 "because the communication end partner is not (yet) known");
				return;
			}
		}
		connector->unsubscribeFrom«attributeName.toFirstUpper»(subscriptionId);
	}

	«produceUpdateAttributeSubscriptionSignature(attribute, className)» {
		auto runtimeSharedPtr = _runtime.lock();
		if (!runtimeSharedPtr || !connector) {
			std::string errorMsg;
			if (!runtimeSharedPtr) {
				errorMsg = "proxy cannot subscribe to «className».«attributeName», "
						"because the required runtime has been already destroyed.";
			} else {
				errorMsg = "proxy cannot subscribe to «className».«attributeName», "
						"because the communication end partner is not (yet) known";
			}
			JOYNR_LOG_WARN(logger(), errorMsg);
			auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorMsg);
			auto future = std::make_shared<Future<std::string>>();
			future->onError(error);
			subscriptionListener->onError(*error);
			return future;
		}
		return connector->subscribeTo«attributeName.toFirstUpper»(
					subscriptionListener,
					subscriptionQos,
					subscriptionId);
	}

	«produceSubscribeToAttributeSignature(attribute, className)» {
		auto runtimeSharedPtr = _runtime.lock();
		if (!runtimeSharedPtr || !connector) {
			std::string errorMsg;
			if (!runtimeSharedPtr) {
				errorMsg = "proxy cannot subscribe to «className».«attributeName», "
						 "because the required runtime has been already destroyed.";
			} else {
				errorMsg = "proxy cannot subscribe to «className».«attributeName», "
						 "because the communication end partner is not (yet) known";
			}
			JOYNR_LOG_WARN(logger(), errorMsg);
			auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorMsg);
			auto future = std::make_shared<Future<std::string>>();
			future->onError(error);
			subscriptionListener->onError(*error);
			return future;
		}
		return connector->subscribeTo«attributeName.toFirstUpper»(
					subscriptionListener,
					subscriptionQos);
	}

«ENDFOR»

«FOR broadcast: francaIntf.broadcasts»
	«var broadcastName = broadcast.joynrName»
	«produceUnsubscribeFromBroadcastSignature(broadcast, className)»
	{
		auto runtimeSharedPtr = _runtime.lock();
		if (!runtimeSharedPtr) {
			JOYNR_LOG_WARN(logger(), "proxy cannot unsubscribe from «className».«broadcastName» broadcast, "
					 "because the required runtime has been already destroyed.");
			return;
		}
		if (!connector){
			JOYNR_LOG_WARN(logger(), "proxy cannot unsubscribe from «className».«broadcastName» broadcast, "
					 "because the communication end partner is not (yet) known");
			return;
		}
		connector->unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(subscriptionId);
	}

	«produceSubscribeToBroadcastSignature(broadcast, francaIntf, className)» {
		std::string errorMsg;
		auto runtimeSharedPtr = _runtime.lock();
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot subscribe to «className».«broadcastName» broadcast, "
					 "because the required runtime has been already destroyed.";
			JOYNR_LOG_WARN(logger(), errorMsg);
			auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorMsg);
			auto future = std::make_shared<Future<std::string>>();
			future->onError(error);
			subscriptionListener->onError(*error);
			return future;
		}
		if (!connector){
			errorMsg = "proxy cannot subscribe to «className».«broadcastName» broadcast, "
				"because the communication end partner is not (yet) known";
		}

		«IF !broadcast.selective»
			try {
				util::validatePartitions(partitions, true);
			} catch (const std::invalid_argument& exception) {
				errorMsg = "invalid argument:\n" + std::string(exception.what());
			}
		«ENDIF»

		if (!errorMsg.empty()) {
			JOYNR_LOG_WARN(logger(), errorMsg);
			auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorMsg);
			auto future = std::make_shared<Future<std::string>>();
			future->onError(error);
			subscriptionListener->onError(*error);
			return future;
		}

		«IF broadcast.selective»
			return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
					filterParameters,
					subscriptionListener,
					subscriptionQos);
		«ELSE»
			return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
					subscriptionListener,
					subscriptionQos,
					partitions);
		«ENDIF»
	}

	«produceUpdateBroadcastSubscriptionSignature(broadcast, francaIntf, className)» {
		std::string errorMsg;
		auto runtimeSharedPtr = _runtime.lock();
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot subscribe to «className».«broadcastName» broadcast, "
					 "because the required runtime has been already destroyed.";
			JOYNR_LOG_WARN(logger(), errorMsg);
			auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorMsg);
			auto future = std::make_shared<Future<std::string>>();
			future->onError(error);
			subscriptionListener->onError(*error);
			return future;
		}
		if (!connector){
			errorMsg = "proxy cannot subscribe to «className».«broadcastName» broadcast, "
				"because the communication end partner is not (yet) known";
		}

		«IF !broadcast.selective»
			try {
				util::validatePartitions(partitions, true);
			} catch (const std::invalid_argument& exception) {
				errorMsg = "invalid argument:\n" + std::string(exception.what());
			}
		«ENDIF»
		if (!errorMsg.empty()) {
			JOYNR_LOG_WARN(logger(), errorMsg);
			auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorMsg);
			auto future = std::make_shared<Future<std::string>>();
			future->onError(error);
			subscriptionListener->onError(*error);
			return future;
		}

		«IF broadcast.selective»
			return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
						subscriptionId,
						filterParameters,
						subscriptionListener,
						subscriptionQos);
		«ELSE»
			return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
						subscriptionId,
						subscriptionListener,
						subscriptionQos,
						partitions);
		«ENDIF»
	}
«ENDFOR»

«getNamespaceEnder(francaIntf)»
'''
}
