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
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceSyncProxyCppTemplate  implements InterfaceTemplate{
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension MethodUtil
	@Inject private extension InterfaceUtil

	override generate(FInterface fInterface)
'''
«val interfaceName =  fInterface.joynrName»
«val className = interfaceName + "Proxy"»
«val syncClassName = interfaceName + "SyncProxy"»
«warning()»

#include "«getPackagePathWithJoynrPrefix(fInterface, "/")»/«syncClassName».h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/Dispatcher.h"
#include "joynr/DispatcherUtils.h"

«FOR datatype: getRequiredIncludesFor(fInterface)»
	#include «datatype»
«ENDFOR»

«getNamespaceStarter(fInterface)»
// The proxies will contain all arbitration checks
// the connectors will contain the JSON related code

«syncClassName»::«syncClassName»(
		std::shared_ptr<joynr::system::RoutingTypes::Address> messagingAddress,
		joynr::ConnectorFactory* connectorFactory,
		joynr::IClientCache *cache,
		const std::string &domain,
		const joynr::MessagingQos &qosSettings,
		bool cached
) :
		joynr::ProxyBase(connectorFactory, cache, domain, INTERFACE_NAME(), qosSettings, cached),
		«className»Base(messagingAddress, connectorFactory, cache, domain, qosSettings, cached)
{
}

«FOR attribute: getAttributes(fInterface)»
	«var attributeName = attribute.joynrName»
	«var attributeType = attribute.typeName»
	«var getAttribute = "get" + attributeName.toFirstUpper»
	«var setAttribute = "set" + attributeName.toFirstUpper»
	«IF attribute.readable»
		void «syncClassName»::«getAttribute»(«attributeType»& result)
		{
			if (connector==nullptr){
				«val errorMsg = "proxy cannot invoke " + getAttribute + " because the communication end partner is not (yet) known"»
				JOYNR_LOG_WARN(logger) << "«errorMsg»";
				exceptions::JoynrRuntimeException error("«errorMsg»");
				throw error;
			}
			else{
				return connector->«getAttribute»(result);
			}
		}
	«ENDIF»
	«IF attribute.writable»
		void «syncClassName»::«setAttribute»(const «attributeType»& value)
		{
			if (connector==nullptr){
				«val errorMsg = "proxy cannot invoke " + setAttribute + " because the communication end partner is not (yet) known"»
				JOYNR_LOG_WARN(logger) << "«errorMsg»";
				exceptions::JoynrRuntimeException error("«errorMsg»");
				throw error;
			}
			else{
				return connector->«setAttribute»(value);
			}
		}
	«ENDIF»

«ENDFOR»
«FOR method: getMethods(fInterface)»
	«var methodName = method.name»
	«var inputTypedParamList = method.commaSeperatedTypedConstInputParameterList»
	«val outputTypedParamList = method.commaSeperatedTypedOutputParameterList»
	«val outputUntypedParamList = getCommaSeperatedUntypedOutputParameterList(method)»
	«var params = getCommaSeperatedUntypedInputParameterList(method)»
	/*
	 * «methodName»
	 */

	void «syncClassName»::«methodName»(
		«outputTypedParamList»«IF method.outputParameters.size > 0 && method.inputParameters.size > 0», «ENDIF»«inputTypedParamList»)
	{
		if (connector==nullptr){
			«val errorMsg = "proxy cannot invoke " + methodName + " because the communication end partner is not (yet) known"»
			JOYNR_LOG_WARN(logger) << "«errorMsg»";
				exceptions::JoynrRuntimeException error("«errorMsg»");
				throw error;
		}
		else{
			return connector->«methodName»(«outputUntypedParamList»«IF method.outputParameters.size > 0 && method.inputParameters.size > 0», «ENDIF»«params»);
		}
	}

«ENDFOR»
«getNamespaceEnder(fInterface)»
'''
}
