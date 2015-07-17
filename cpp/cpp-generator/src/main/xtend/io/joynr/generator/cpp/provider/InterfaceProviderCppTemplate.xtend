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
import io.joynr.generator.cpp.util.QtTypeUtil
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.util.InterfaceTemplate
import org.franca.core.franca.FInterface

class InterfaceProviderCppTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension CppMigrateToStdTypeUtil

	@Inject
	private QtTypeUtil qtTypeUtil

	@Inject
	private extension JoynrCppGeneratorExtensions
	override generate(FInterface serviceInterface)
'''
«warning()»
«val interfaceName = serviceInterface.joynrName»
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»Provider.h"
#include "joynr/InterfaceRegistrar.h"
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestInterpreter.h"
#include "joynr/RequestStatus.h"
#include "joynr/TypeUtil.h"

«getNamespaceStarter(serviceInterface)»
«interfaceName»Provider::«interfaceName»Provider()«IF !serviceInterface.attributes.empty» :«ENDIF»
	«FOR attribute: getAttributes(serviceInterface) SEPARATOR ","»
		«attribute.joynrName»()
	«ENDFOR»
{
	// Register a request interpreter to interpret requests to this interface
	joynr::InterfaceRegistrar::instance().registerRequestInterpreter<«interfaceName»RequestInterpreter>(getInterfaceName());
}

«interfaceName»Provider::~«interfaceName»Provider()
{
	// Unregister the request interpreter
	joynr::InterfaceRegistrar::instance().unregisterRequestInterpreter(getInterfaceName());
}

const std::string& «interfaceName»Provider::INTERFACE_NAME()
{
	static const std::string INTERFACE_NAME("«getPackagePathWithoutJoynrPrefix(serviceInterface, "/")»/«interfaceName.toLowerCase»");
	return INTERFACE_NAME;
}

std::string «interfaceName»Provider::getInterfaceName() const {
	return INTERFACE_NAME();
}

«FOR attribute: getAttributes(serviceInterface)»
	«var attributeType = attribute.typeName»
	«var attributeName = attribute.joynrName»
	void «interfaceName»Provider::get«attributeName.toFirstUpper»(
			std::function<void(
					const joynr::RequestStatus&,
					const «attribute.typeName»&)> callbackFct) {
		callbackFct(joynr::RequestStatus(joynr::RequestStatusCode::OK), «attributeName»);
	}

	void «interfaceName»Provider::set«attributeName.toFirstUpper»(
			const «attribute.typeName»& «attributeName»,
			std::function<void(const joynr::RequestStatus&)> callbackFct) {
		«attributeName»Changed(«attributeName»);
		callbackFct(joynr::RequestStatus(joynr::RequestStatusCode::OK));
	}

	void «interfaceName»Provider::«attributeName»Changed(const «attributeType»& «attributeName») {
		if(this->«attributeName» == «attributeName») {
			// the value didn't change, no need for notification
			return;
		}
		this->«attributeName» = «attributeName»;
		onAttributeValueChanged("«attributeName»", QVariant::fromValue(«qtTypeUtil.fromStdTypeToQTType(attribute, attributeName)»));
	}
«ENDFOR»

«FOR broadcast: serviceInterface.broadcasts»
	«var broadcastName = broadcast.joynrName»
	void «interfaceName»Provider::fire«broadcastName.toFirstUpper»(«broadcast.commaSeperatedTypedConstOutputParameterList») {
		QList<QVariant> broadcastValues;
		«FOR parameter: getOutputParameters(broadcast)»
			broadcastValues.append(QVariant::fromValue(«qtTypeUtil.fromStdTypeToQTType(parameter, parameter.name)»));
		«ENDFOR»
		fireBroadcast("«broadcastName»", broadcastValues);
	}
«ENDFOR»
«getNamespaceEnder(serviceInterface)»
'''
}