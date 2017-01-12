package io.joynr.generator.cpp.proxy
/*
 * !!!
 *
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

class InterfaceAsyncProxyCppTemplate extends InterfaceTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension MethodUtil
	@Inject private extension CppInterfaceUtil

	override generate()
'''
«val interfaceName =  francaIntf.joynrName»
«val className = interfaceName + "Proxy"»
«val asyncClassName = interfaceName + "AsyncProxy"»
«warning()»

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«asyncClassName».h"

«FOR parameterType: getDataTypeIncludesFor(francaIntf).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»

#include "joynr/Future.h"
#include "joynr/exceptions/JoynrException.h"

«getNamespaceStarter(francaIntf)»
«asyncClassName»::«asyncClassName»(
		std::shared_ptr<const joynr::system::RoutingTypes::Address> messagingAddress,
		joynr::ConnectorFactory* connectorFactory,
		joynr::IClientCache *cache,
		const std::string &domain,
		const joynr::MessagingQos &qosSettings
) :
	joynr::ProxyBase(connectorFactory, cache, domain, qosSettings),
	«className»Base(messagingAddress, connectorFactory, cache, domain, qosSettings)«IF hasFireAndForgetMethods(francaIntf)»,
	«interfaceName»FireAndForgetProxy(messagingAddress, connectorFactory, cache, domain, qosSettings)«ENDIF»
{
}

«FOR attribute: getAttributes(francaIntf)»
	«var attributeName = attribute.joynrName»
	«var attributeType = attribute.typeName»
	«IF attribute.readable»
		«var getAttribute = "get" + attributeName.toFirstUpper»
		/*
		 * «getAttribute»
		 */

		«produceAsyncGetterSignature(attribute, asyncClassName)»
		{
			if (connector==nullptr){
				«val errorMsg = "proxy cannot invoke " + getAttribute + ", because the communication end partner is not (yet) known"»
				JOYNR_LOG_WARN(logger, "«errorMsg»");
				auto error = std::make_shared<exceptions::JoynrRuntimeException>("«errorMsg»");
				if (onError) {
					onError(*error);
				}
				auto future = std::make_shared<joynr::Future<«attributeType»>>();
				future->onError(error);
				return future;
			}
			else{
				return connector->«getAttribute»Async(std::move(onSuccess), std::move(onError));
			}
		}

	«ENDIF»
	«IF attribute.writable»
		«var setAttribute = "set" + attributeName.toFirstUpper»
		/*
		 * «setAttribute»
		 */
		«produceAsyncSetterSignature(attribute, asyncClassName)»
		{
			if (connector==nullptr){
				«val errorMsg = "proxy cannot invoke " + setAttribute + ", because the communication end partner is not (yet) known"»
				JOYNR_LOG_WARN(logger, "«errorMsg»");
				auto error = std::make_shared<exceptions::JoynrRuntimeException>("«errorMsg»");
				if (onError) {
					onError(*error);
				}
				auto future = std::make_shared<joynr::Future<void>>();
				future->onError(error);
				return future;
			}
			else{
				return connector->«setAttribute»Async(«attributeName», std::move(onSuccess), std::move(onError));
			}
		}

	«ENDIF»
«ENDFOR»
«FOR method: getMethods(francaIntf)»
	«IF !method.fireAndForget»
		«var methodName = method.joynrName»
		«var outputParameters = method.commaSeparatedOutputParameterTypes»
		«var inputParamList = getCommaSeperatedUntypedInputParameterList(method)»
		/*
		 * «methodName»
		 */
		«produceAsyncMethodSignature(francaIntf, method, asyncClassName)»
		{
			if (connector==nullptr){
				«val errorMsg = "proxy cannot invoke " + methodName + ", because the communication end partner is not (yet) known"»
				JOYNR_LOG_WARN(logger, "«errorMsg»");
				auto error = std::make_shared<exceptions::JoynrRuntimeException>("«errorMsg»");
				if (onRuntimeError) {
					onRuntimeError(*error);
				}
				auto future = std::make_shared<joynr::Future<«outputParameters»>>();
				future->onError(error);
				return future;
			}
			else{
				return connector->«methodName»Async(«inputParamList»«IF !method.inputParameters.empty», «ENDIF»std::move(onSuccess), «IF method.hasErrorEnum»std::move(onApplicationError), «ENDIF»std::move(onRuntimeError));
			}
		}
	«ENDIF»
«ENDFOR»
«getNamespaceEnder(francaIntf)»
'''
}
