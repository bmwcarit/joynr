package io.joynr.generator.cpp.proxy
/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

class InterfaceProxyHTemplate  {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase

	def generate(FInterface serviceInterface) {
		val interfaceName =  serviceInterface.joynrName
		val className = interfaceName + "Proxy"
		val asyncClassName = interfaceName + "AsyncProxy"
		val syncClassName = interfaceName + "SyncProxy"
		val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+"_"+interfaceName+"Proxy_h").toUpperCase
		'''
		«warning()»

		#ifndef «headerGuard»
		#define «headerGuard»

		#include "joynr/PrivateCopyAssign.h"
		«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
		#include "«parameterType»"
		«ENDFOR»
		«getDllExportIncludeStatement()»
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«syncClassName».h"
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«asyncClassName».h"
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName».h"

		#ifdef _MSC_VER
			// Visual C++ gives a warning which is caused by diamond inheritance, but this is 
			// not relevant when using pure virtual methods:
			// http://msdn.microsoft.com/en-us/library/6b3sy7ae(v=vs.80).aspx
			#pragma warning( disable : 4250 )
		#endif

		«getNamespaceStarter(serviceInterface)» 
		class «getDllExportMacro()» «className» : virtual public I«interfaceName», virtual public «syncClassName», virtual public «asyncClassName» {
		public:
		    «className»(
		            QSharedPointer<joynr::system::Address> messagingAddress,
		            joynr::ConnectorFactory* connectorFactory,
		            joynr::IClientCache* cache,
		            const QString& domain,
		            const joynr::ProxyQos& proxyQos,
		            const joynr::MessagingQos& qosSettings,
		            bool cached
		    );

			«FOR attribute: getAttributes(serviceInterface)»
				«var attributeName = attribute.joynrName»
				«val returnType = getMappedDatatypeOrList(attribute)»
				void unsubscribeFrom«attributeName.toFirstUpper»(QString &subscriptionId) {
					«className»Base::unsubscribeFrom«attributeName.toFirstUpper»(subscriptionId);
				}

				QString subscribeTo«attributeName.toFirstUpper»(QSharedPointer<joynr::ISubscriptionListener<«returnType»> > subscriptionListener, QSharedPointer<joynr::SubscriptionQos> subscriptionQos){
					return «className»Base::subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos);
				}
			«ENDFOR»

			«FOR broadcast: serviceInterface.broadcasts»
				«var broadcastName = broadcast.joynrName»
				«val returnTypes = getMappedOutputParametersCommaSeparated(broadcast)»
				void unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(QString &subscriptionId) {
				    «className»Base::unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(subscriptionId);
				}

				«IF isSelective(broadcast)»
				QString subscribeTo«broadcastName.toFirstUpper»Broadcast(
				            «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters,
				            QSharedPointer<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
				            QSharedPointer<joynr::SubscriptionQos> subscriptionQos){
				    return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				                filterParameters,
				                subscriptionListener,
				                subscriptionQos);
				}
				«ELSE»
				QString subscribeTo«broadcastName.toFirstUpper»Broadcast(
				            QSharedPointer<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
				            QSharedPointer<joynr::SubscriptionQos> subscriptionQos){
				    return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				                subscriptionListener,
				                subscriptionQos);
				}
				«ENDIF»
			«ENDFOR»

			virtual ~«className»();

			// attributes
			«FOR attribute: getAttributes(serviceInterface)»
				«var attributeName = attribute.joynrName»
				using «asyncClassName»::get«attributeName.toFirstUpper»;
				using «asyncClassName»::set«attributeName.toFirstUpper»;
				using «syncClassName»::get«attributeName.toFirstUpper»;
				using «syncClassName»::set«attributeName.toFirstUpper»;

			«ENDFOR»

		    // operations
			«FOR methodName: getUniqueMethodNames(serviceInterface)»
				using «asyncClassName»::«methodName»;
				using «syncClassName»::«methodName»;

			«ENDFOR»
		private:
		    DISALLOW_COPY_AND_ASSIGN(«className»);
		};

		«getNamespaceEnder(serviceInterface)»

		#endif // «headerGuard»
		'''
	}

}