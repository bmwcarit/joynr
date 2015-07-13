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
import org.franca.core.franca.FInterface
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.util.InterfaceTemplate
import io.joynr.generator.cpp.util.QtTypeUtil

class InterfaceRequestCallerCppTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension QtTypeUtil

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
«ENDIF»

«getNamespaceStarter(serviceInterface)»
«interfaceName»RequestCaller::«interfaceName»RequestCaller(std::shared_ptr<«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»Provider> provider)
	: joynr::RequestCaller(I«interfaceName»::getInterfaceName()),
	  provider(provider)
{
}

«FOR attribute: getAttributes(serviceInterface)»
	«var attributeName = attribute.joynrName»
	«val returnType = attribute.typeName»
	void «interfaceName»RequestCaller::get«attributeName.toFirstUpper»(
			std::function<void(const joynr::RequestStatus& status, const «returnType»& «attributeName.toFirstLower»)> callbackFct){
		provider->get«attributeName.toFirstUpper»(callbackFct);
	}

	void «interfaceName»RequestCaller::set«attributeName.toFirstUpper»(
			«returnType» «attributeName.toFirstLower»,
			std::function<void(const joynr::RequestStatus& status)> callbackFct){
		provider->set«attributeName.toFirstUpper»(«attributeName.toFirstLower», callbackFct);
	}

«ENDFOR»
«FOR method: getMethods(serviceInterface)»
	«val outputTypedParamList = method.commaSeperatedTypedConstOutputParameterList»
	«val inputTypedParamList = method.commaSeperatedTypedConstInputParameterList»
	«val inputUntypedParamList = method.commaSeperatedUntypedInputParameterList»
	«val methodName = method.joynrName»
	void «interfaceName»RequestCaller::«methodName»(
			«IF !method.inputParameters.empty»«inputTypedParamList»,«ENDIF»
			std::function<void(
					const joynr::RequestStatus& joynrInternalStatus«IF !method.outputParameters.empty»,«ENDIF»
					«outputTypedParamList»)> callbackFct){
			provider->«methodName»(
					«IF !method.inputParameters.empty»«inputUntypedParamList»,«ENDIF»
					callbackFct);
	}
«ENDFOR»

void «interfaceName»RequestCaller::registerAttributeListener(const QString& attributeName, joynr::IAttributeListener* attributeListener)
{
	provider->registerAttributeListener(attributeName, attributeListener);
}

void «interfaceName»RequestCaller::unregisterAttributeListener(const QString& attributeName, joynr::IAttributeListener* attributeListener)
{
	provider->unregisterAttributeListener(attributeName, attributeListener);
}

void «interfaceName»RequestCaller::registerBroadcastListener(const QString& broadcastName, joynr::IBroadcastListener* broadcastListener)
{
	provider->registerBroadcastListener(broadcastName, broadcastListener);
}

void «interfaceName»RequestCaller::unregisterBroadcastListener(const QString& broadcastName, joynr::IBroadcastListener* broadcastListener)
{
	provider->unregisterBroadcastListener(broadcastName, broadcastListener);
}

«getNamespaceEnder(serviceInterface)»
'''
}
