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
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.util.InterfaceTemplate
import org.franca.core.franca.FInterface

class InterfaceProxyHTemplate implements InterfaceTemplate{
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil

	override generate(FInterface serviceInterface)
'''
«val interfaceName =  serviceInterface.joynrName»
«val className = interfaceName + "Proxy"»
«val asyncClassName = interfaceName + "AsyncProxy"»
«val syncClassName = interfaceName + "SyncProxy"»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+
	"_"+interfaceName+"Proxy_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

#include "joynr/PrivateCopyAssign.h"
«FOR parameterType: getRequiredIncludesFor(serviceInterface).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»
#include <memory>

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
			const std::string& domain,
			const joynr::MessagingQos& qosSettings,
			bool cached
	);

	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.notifiable]»
		«var attributeName = attribute.joynrName»
		«val returnType = attribute.typeName»
		void unsubscribeFrom«attributeName.toFirstUpper»(std::string &subscriptionId) {
			«className»Base::unsubscribeFrom«attributeName.toFirstUpper»(subscriptionId);
		}

		std::string subscribeTo«attributeName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					const joynr::StdSubscriptionQos& subscriptionQos){
			return «className»Base::subscribeTo«attributeName.toFirstUpper»(
						subscriptionListener,
						subscriptionQos);
		}

		std::string subscribeTo«attributeName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					const joynr::StdSubscriptionQos& subscriptionQos,
					std::string& subscriptionId){
			return «className»Base::subscribeTo«attributeName.toFirstUpper»(
						subscriptionListener,
						subscriptionQos,
						subscriptionId);
		}
	«ENDFOR»

	«FOR broadcast: serviceInterface.broadcasts»
		«var broadcastName = broadcast.joynrName»
		«val returnTypes = broadcast.commaSeparatedOutputParameterTypes»
		void unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(std::string &subscriptionId) {
			«className»Base::unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(subscriptionId);
		}

		«IF isSelective(broadcast)»
			std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						const «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::StdOnChangeSubscriptionQos& subscriptionQos){
				return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(
							filterParameters,
							subscriptionListener,
							subscriptionQos);
			}

			std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						const «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::StdOnChangeSubscriptionQos& subscriptionQos,
						std::string& subscriptionId){
				return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(
							filterParameters,
							subscriptionListener,
							subscriptionQos,
							subscriptionId);
			}
		«ELSE»
			std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::StdOnChangeSubscriptionQos& subscriptionQos){
				return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(
							subscriptionListener,
							subscriptionQos);
			}

			std::string subscribeTo«broadcastName.toFirstUpper»Broadcast(
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::StdOnChangeSubscriptionQos& subscriptionQos,
						std::string& subscriptionId){
				return «className»Base::subscribeTo«broadcastName.toFirstUpper»Broadcast(
							subscriptionListener,
							subscriptionQos,
							subscriptionId);
			}
		«ENDIF»
	«ENDFOR»

	virtual ~«className»();

	// attributes
	«FOR attribute: getAttributes(serviceInterface)»
		«var attributeName = attribute.joynrName»
		«IF attribute.readable»
			using «asyncClassName»::get«attributeName.toFirstUpper»;
			using «syncClassName»::get«attributeName.toFirstUpper»;
		«ENDIF»
		«IF attribute.writable»
			using «asyncClassName»::set«attributeName.toFirstUpper»;
			using «syncClassName»::set«attributeName.toFirstUpper»;
		«ENDIF»
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