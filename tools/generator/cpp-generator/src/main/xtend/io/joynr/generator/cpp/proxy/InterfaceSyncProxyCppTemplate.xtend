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
import io.joynr.generator.cpp.util.CppInterfaceUtil
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil

class InterfaceSyncProxyCppTemplate extends InterfaceTemplate {
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject extension NamingUtil
	@Inject extension AttributeUtil
	@Inject extension MethodUtil
	@Inject extension CppInterfaceUtil

	override generate()
'''
«val interfaceName =  francaIntf.joynrName»
«val className = interfaceName + "Proxy"»
«val syncClassName = interfaceName + "SyncProxy"»
«warning()»

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«syncClassName».h"

«FOR datatype: getDataTypeIncludesFor(francaIntf)»
	#include «datatype»
«ENDFOR»

«getNamespaceStarter(francaIntf)»
// The proxies will contain all arbitration checks
// the connectors will contain the JSON related code

«syncClassName»::«syncClassName»(
		std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
		std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
		const std::string &domain,
		const joynr::MessagingQos &qosSettings
) :
		joynr::ProxyBase(runtime, connectorFactory, domain, qosSettings),
		«className»Base(runtime, connectorFactory, domain, qosSettings)«IF hasFireAndForgetMethods(francaIntf)»,
		«interfaceName»FireAndForgetProxy(runtime, connectorFactory, domain, qosSettings)«ENDIF»
{
}

«FOR attribute: getAttributes(francaIntf)»
	«var attributeName = attribute.joynrName»
	«var getAttribute = "get" + attributeName.toFirstUpper»
	«var setAttribute = "set" + attributeName.toFirstUpper»
	«IF attribute.readable»
		«produceSyncGetterSignature(attribute, syncClassName)»
		{
			auto runtimeSharedPtr = _runtime.lock();
			if (!runtimeSharedPtr || (connector==nullptr)) {
				std::string errorMsg;
				if (!runtimeSharedPtr) {
					«val errorMsgRuntime = "proxy cannot invoke " + getAttribute + " because the required runtime has been already destroyed."»
					errorMsg = "«errorMsgRuntime»";
				}
				else {
					«val errorMsgCommunication = "proxy cannot invoke " + getAttribute + " because the communication end partner is not (yet) known"»
					errorMsg = "«errorMsgCommunication»";
				}
				JOYNR_LOG_WARN(logger(), errorMsg);
				exceptions::JoynrRuntimeException error(errorMsg);
				throw error;
			}
			else{
				return connector->«getAttribute»(«attributeName», std::move(qos));
			}
		}
	«ENDIF»
	«IF attribute.writable»
		«produceSyncSetterSignature(attribute, syncClassName)»
		{
			auto runtimeSharedPtr = _runtime.lock();
			if (!runtimeSharedPtr || (connector==nullptr)) {
				std::string errorMsg;
				if (!runtimeSharedPtr) {
					«val errorMsgRuntime = "proxy cannot invoke " + setAttribute + " because the required runtime has been already destroyed."»
					errorMsg = "«errorMsgRuntime»";
				}
				else {
					«val errorMsgCommunication = "proxy cannot invoke " + setAttribute + " because the communication end partner is not (yet) known"»
					errorMsg = "«errorMsgCommunication»";
				}
				JOYNR_LOG_WARN(logger(), errorMsg);
				exceptions::JoynrRuntimeException error(errorMsg);
				throw error;
			}
			else{
				return connector->«setAttribute»(«attributeName», std::move(qos));
			}
		}
	«ENDIF»

«ENDFOR»
«FOR method: getMethods(francaIntf).filter[!fireAndForget]»
	«var methodName = method.name»
	«val outputUntypedParamList = getCommaSeperatedUntypedOutputParameterList(method)»
	«var params = getCommaSeperatedUntypedInputParameterList(method)»
	/*
	 * «methodName»
	 */
	«produceSyncMethodSignature(method, syncClassName)»
	{
		auto runtimeSharedPtr = _runtime.lock();
		if (!runtimeSharedPtr || (connector==nullptr)) {
			std::string errorMsg;
			if (!runtimeSharedPtr) {
				«val errorMsgRuntime = "proxy cannot invoke " + methodName + " because the required runtime has been already destroyed."»
				errorMsg = "«errorMsgRuntime»";
			}
			if (connector==nullptr){
				«val errorMsgCommunication = "proxy cannot invoke " + methodName + " because the communication end partner is not (yet) known"»
				errorMsg = "«errorMsgCommunication»";
			}
			JOYNR_LOG_WARN(logger(), errorMsg);
			exceptions::JoynrRuntimeException error(errorMsg);
			throw error;
		}
		else{
			return connector->«methodName»(«outputUntypedParamList»«IF method.outputParameters.size > 0», «ENDIF»«params»«IF method.inputParameters.size > 0»,«ENDIF» std::move(qos));
		}
	}
«ENDFOR»
«getNamespaceEnder(francaIntf)»
'''
}
