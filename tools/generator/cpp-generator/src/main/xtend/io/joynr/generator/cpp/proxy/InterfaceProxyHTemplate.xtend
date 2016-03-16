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

class InterfaceProxyHTemplate extends InterfaceTemplate {
	@Inject extension JoynrCppGeneratorExtensions
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
«val interfaceName =  francaIntf.joynrName»
«val className = interfaceName + "Proxy"»
«val asyncClassName = interfaceName + "AsyncProxy"»
«val syncClassName = interfaceName + "SyncProxy"»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_")+
	"_"+interfaceName+"Proxy_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

#include "joynr/PrivateCopyAssign.h"
«FOR parameterType: getRequiredIncludesFor(francaIntf).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»
#include <memory>

«getDllExportIncludeStatement()»
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«syncClassName».h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«asyncClassName».h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/I«interfaceName».h"

#ifdef _MSC_VER
	// Visual C++ gives a warning which is caused by diamond inheritance, but this is
	// not relevant when using pure virtual methods:
	// http://msdn.microsoft.com/en-us/library/6b3sy7ae(v=vs.80).aspx
	#pragma warning( disable : 4250 )
#endif

«getNamespaceStarter(francaIntf)»
/**
 * @brief Proxy class for interface «interfaceName»
 *
 * @version «majorVersion».«minorVersion»
 */
class «getDllExportMacro()» «className» : virtual public I«interfaceName», virtual public «syncClassName», virtual public «asyncClassName» {
public:
	/**
	 * @brief Parameterized constructor
	 * @param messagingAddress The address
	 * @param connectorFactory The connector factory
	 * @param cache The client cache
	 * @param domain The provider domain
	 * @param qosSettings The quality of service settings
	 * @param cached True, if cached, false otherwise
	 */
	«className»(
			std::shared_ptr<joynr::system::RoutingTypes::Address> messagingAddress,
			joynr::ConnectorFactory* connectorFactory,
			joynr::IClientCache* cache,
			const std::string& domain,
			const joynr::MessagingQos& qosSettings,
			bool cached
	);
	«FOR attribute: getAttributes(francaIntf).filter[attribute | attribute.notifiable]»
		«var attributeName = attribute.joynrName»
		«val returnType = attribute.typeName»

		/**
		 * @brief unsubscribes from attribute «attributeName.toFirstUpper»
		 * @param subscriptionId The subscription id returned earlier on creation of the subscription
		 */
		void unsubscribeFrom«attributeName.toFirstUpper»(std::string &subscriptionId) override {
			«className»Base::unsubscribeFrom«attributeName.toFirstUpper»(subscriptionId);
		}

		/**
		 * @brief creates a new subscription to attribute «attributeName.toFirstUpper»
		 * @param subscriptionListener The listener callback providing methods to call on publication and failure
		 * @param subscriptionQos The subscription quality of service settings
		 * @return the subscription id as string
		 */
		std::string subscribeTo«attributeName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					const joynr::SubscriptionQos& subscriptionQos) override {
			return «className»Base::subscribeTo«attributeName.toFirstUpper»(
						subscriptionListener,
						subscriptionQos);
		}

		/**
		 * @brief updates an existing subscription to attribute «attributeName.toFirstUpper»
		 * @param subscriptionListener The listener callback providing methods to call on publication and failure
		 * @param subscriptionQos The subscription quality of service settings
		 * @param subscriptionId The subscription id returned earlier on creation of the subscription
		 * @return the subscription id as string
		 */
		std::string subscribeTo«attributeName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					const joynr::SubscriptionQos& subscriptionQos,
					std::string& subscriptionId) override{
			return «className»Base::subscribeTo«attributeName.toFirstUpper»(
						subscriptionListener,
						subscriptionQos,
						subscriptionId);
		}
	«ENDFOR»

	«FOR broadcast: francaIntf.broadcasts»
		«var broadcastName = broadcast.joynrName»
		«val returnTypes = broadcast.commaSeparatedOutputParameterTypes»

		/**
		 * @brief unsubscribes from broadcast «broadcastName.toFirstUpper»
		 * @param subscriptionId The subscription id returned earlier on creation of the subscription
		 */
		void unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(std::string &subscriptionId) override {
			«className»Base::unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(subscriptionId);
		}

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
						const joynr::OnChangeSubscriptionQos& subscriptionQos) override {
				return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(
							filterParameters,
							subscriptionListener,
							subscriptionQos);
			}

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
						std::string& subscriptionId) override {
				return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(
							filterParameters,
							subscriptionListener,
							subscriptionQos,
							subscriptionId);
			}
		«ELSE»
			/**
			 * @brief subscribes to broadcast «broadcastName.toFirstUpper»
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @return the subscription id as string
			 */
			std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::OnChangeSubscriptionQos& subscriptionQos) override {
				return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(
							subscriptionListener,
							subscriptionQos);
			}

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
						std::string& subscriptionId) override {
				return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(
							subscriptionListener,
							subscriptionQos,
							subscriptionId);
			}
		«ENDIF»
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

	// operations
	«FOR methodName: getUniqueMethodNames(francaIntf)»
		using «asyncClassName»::«methodName»Async;
		using «syncClassName»::«methodName»;

	«ENDFOR»
private:
	DISALLOW_COPY_AND_ASSIGN(«className»);
};

«getNamespaceEnder(francaIntf)»

#endif // «headerGuard»
'''
}
