package io.joynr.generator.cpp.provider
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
import io.joynr.generator.cpp.util.CppMigrateToStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.util.InterfaceTemplate
import org.franca.core.franca.FInterface

class InterfaceRequestCallerCppTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension CppMigrateToStdTypeUtil

	@Inject
	private extension JoynrCppGeneratorExtensions

	override generate(FInterface serviceInterface)
'''
«var interfaceName = serviceInterface.joynrName»
«warning()»
#include <functional>

#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestCaller.h"
«FOR datatype: getRequiredIncludesFor(serviceInterface)»
	#include "«datatype»"
«ENDFOR»
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»Provider.h"
«IF !serviceInterface.methods.empty || !serviceInterface.attributes.empty»
	#include "joynr/RequestStatus.h"
	#include "joynr/TypeUtil.h"
«ENDIF»

«getNamespaceStarter(serviceInterface)»
«interfaceName»RequestCaller::«interfaceName»RequestCaller(std::shared_ptr<«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»Provider> provider)
	: joynr::RequestCaller(provider->getInterfaceName()),
	  provider(provider)
{
}

«IF !serviceInterface.attributes.empty»
	// attributes
«ENDIF»
«FOR attribute : serviceInterface.attributes»
	«var attributeName = attribute.joynrName»
	«val returnType = attribute.typeName»
	«IF attribute.readable»
		void «interfaceName»RequestCaller::get«attributeName.toFirstUpper»(
				std::function<void(
						const joynr::RequestStatus& status,
						const «returnType»& «attributeName.toFirstLower»
				)> callbackFct
		) {
			provider->get«attributeName.toFirstUpper»(callbackFct);
		}
	«ENDIF»
	«IF attribute.writable»
		void «interfaceName»RequestCaller::set«attributeName.toFirstUpper»(
				const «returnType»& «attributeName.toFirstLower»,
				std::function<void(
						const joynr::RequestStatus& status
				)> callbackFct
		) {
			provider->set«attributeName.toFirstUpper»(«attributeName.toFirstLower», callbackFct);
		}
	«ENDIF»

«ENDFOR»
«IF !serviceInterface.methods.empty»
	// methods
«ENDIF»
«FOR method : serviceInterface.methods»
	«val outputTypedParamList = method.commaSeperatedTypedConstOutputParameterList»
	«val inputTypedParamList = method.commaSeperatedTypedConstInputParameterList»
	«val inputUntypedParamList = getCommaSeperatedUntypedInputParameterList(method)»
	«val methodName = method.joynrName»
	void «interfaceName»RequestCaller::«methodName»(
			«IF !method.inputParameters.empty»«inputTypedParamList»,«ENDIF»
			«IF method.outputParameters.empty»
				std::function<void()> onSuccess
			«ELSE»
				std::function<void(
						«outputTypedParamList.substring(1)»
				)> onSuccess
			«ENDIF»
	) {
		provider->«methodName»(
				«IF !method.inputParameters.empty»«inputUntypedParamList»,«ENDIF»
				onSuccess
		);
	}

«ENDFOR»

void «interfaceName»RequestCaller::registerAttributeListener(const std::string& attributeName, joynr::IAttributeListener* attributeListener)
{
	provider->registerAttributeListener(attributeName, attributeListener);
}

void «interfaceName»RequestCaller::unregisterAttributeListener(const std::string& attributeName, joynr::IAttributeListener* attributeListener)
{
	provider->unregisterAttributeListener(attributeName, attributeListener);
}

void «interfaceName»RequestCaller::registerBroadcastListener(const std::string& broadcastName, joynr::IBroadcastListener* broadcastListener)
{
	provider->registerBroadcastListener(broadcastName, broadcastListener);
}

void «interfaceName»RequestCaller::unregisterBroadcastListener(const std::string& broadcastName, joynr::IBroadcastListener* broadcastListener)
{
	provider->unregisterBroadcastListener(broadcastName, broadcastListener);
}

«getNamespaceEnder(serviceInterface)»
'''
}
