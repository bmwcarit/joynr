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
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil

class InterfaceProxyHTemplate extends InterfaceTemplate {
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject extension InterfaceSubscriptionUtil
	@Inject extension NamingUtil
	@Inject extension AttributeUtil
	@Inject extension InterfaceUtil

	override generate(boolean generateVersion)
'''
«val interfaceName =  francaIntf.joynrName»
«val className = interfaceName + "Proxy"»
«val asyncClassName = interfaceName + "AsyncProxy"»
«val syncClassName = interfaceName + "SyncProxy"»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_", generateVersion)+
	"_"+interfaceName+"Proxy_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

#include "joynr/PrivateCopyAssign.h"
«FOR parameterType: getDataTypeIncludesFor(francaIntf, generateVersion).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»
#include <memory>

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«syncClassName».h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«asyncClassName».h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/I«interfaceName».h"

#ifdef _MSC_VER
	// Visual C++ gives a warning which is caused by diamond inheritance, but this is
	// not relevant when using pure virtual methods:
	// http://msdn.microsoft.com/en-us/library/6b3sy7ae(v=vs.80).aspx
	#pragma warning( disable : 4250 )
#endif

«getNamespaceStarter(francaIntf, generateVersion)»
/**
 * @brief Proxy class for interface «interfaceName»
 *
 * @version «majorVersion».«minorVersion»
 */
class «className» : virtual public I«interfaceName», virtual public «syncClassName», virtual public «asyncClassName» {
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
	«FOR attribute: getAttributes(francaIntf).filter[attribute | attribute.notifiable]»
		«var attributeName = attribute.joynrName»

		/**
		 * @brief unsubscribes from attribute «attributeName.toFirstUpper»
		 * @param subscriptionId The subscription id returned earlier on creation of the subscription
		 */
		void unsubscribeFrom«attributeName.toFirstUpper»(const std::string &subscriptionId) override {
			«className»Base::unsubscribeFrom«attributeName.toFirstUpper»(subscriptionId);
		}

		«produceSubscribeToAttributeComments(attribute)»
		«produceSubscribeToAttributeSignature(attribute, generateVersion)» override {
			return «className»Base::subscribeTo«attributeName.toFirstUpper»(
						subscriptionListener,
						subscriptionQos);
		}

		«produceUpdateAttributeSubscriptionComments(attribute)»
		«produceUpdateAttributeSubscriptionSignature(attribute, generateVersion)» override{
			return «className»Base::subscribeTo«attributeName.toFirstUpper»(
						subscriptionListener,
						subscriptionQos,
						subscriptionId);
		}
	«ENDFOR»

	«FOR broadcast: francaIntf.broadcasts»
		«var broadcastName = broadcast.joynrName»

		/**
		 * @brief unsubscribes from broadcast «broadcastName.toFirstUpper»
		 * @param subscriptionId The subscription id returned earlier on creation of the subscription
		 */
		void unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(const std::string &subscriptionId) override {
			«className»Base::unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(subscriptionId);
		}

		«produceSubscribeToBroadcastComments(broadcast)»
		«produceSubscribeToBroadcastSignature(broadcast, francaIntf ,true, generateVersion)» override {
			return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(«IF broadcast.selective»
						filterParameters,«ENDIF»
						subscriptionListener,
						subscriptionQos«
						»«IF !broadcast.selective»«
						»,
						partitions«
						»«ENDIF»
			);
		}

		«produceUpdateBroadcastSubscriptionComments(broadcast)»
		«produceUpdateBroadcastSubscriptionSignature(broadcast, francaIntf, true, generateVersion)» override {
			return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(
						subscriptionId,
						«IF broadcast.selective»
						filterParameters,
						«ENDIF»
						subscriptionListener,
						subscriptionQos«
						»«IF !broadcast.selective»«
						»,
						partitions«
						»«ENDIF»
			);
		}
	«ENDFOR»

	/** @brief Destructor */
	~«className»() override = default;

	// attributes
	«FOR attribute: getAttributes(francaIntf)»
		«var attributeName = attribute.joynrName»
		«IF attribute.readable»
			using «asyncClassName»::get«attributeName.toFirstUpper»Async;
			using «syncClassName»::get«attributeName.toFirstUpper»;
		«ENDIF»
		«IF attribute.writable»
			using «asyncClassName»::set«attributeName.toFirstUpper»Async;
			using «syncClassName»::set«attributeName.toFirstUpper»;
		«ENDIF»
	«ENDFOR»

	«FOR methodName : getUniqueMethodNames(getMethods(francaIntf).filter[!fireAndForget])»
		using I«interfaceName»Sync::«methodName»;
		using I«interfaceName»Async::«methodName»Async;
	«ENDFOR»
	«FOR methodName : getUniqueMethodNames(getMethods(francaIntf).filter[fireAndForget])»
		using I«interfaceName»FireAndForget::«methodName»;
	«ENDFOR»private:
	DISALLOW_COPY_AND_ASSIGN(«className»);
};

«getNamespaceEnder(francaIntf, generateVersion)»

#endif // «headerGuard»
'''
}
