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
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceAbstractProviderCppTemplate implements InterfaceTemplate {

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil
	@Inject private extension BroadcastUtil

	override generate(FInterface serviceInterface)
'''
«warning()»
«val interfaceName = serviceInterface.joynrName»
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»AbstractProvider.h"
#include "joynr/InterfaceRegistrar.h"
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestInterpreter.h"
#include "joynr/RequestStatus.h"
#include "joynr/TypeUtil.h"

«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
	#include «parameterType»
«ENDFOR»

«getNamespaceStarter(serviceInterface)»
«interfaceName»AbstractProvider::«interfaceName»AbstractProvider()
{
	// Register a request interpreter to interpret requests to this interface
	joynr::InterfaceRegistrar::instance().registerRequestInterpreter<«interfaceName»RequestInterpreter>(getInterfaceName());
}

«interfaceName»AbstractProvider::~«interfaceName»AbstractProvider()
{
	// Unregister the request interpreter
	joynr::InterfaceRegistrar::instance().unregisterRequestInterpreter(getInterfaceName());
}

std::string «interfaceName»AbstractProvider::getInterfaceName() const {
	return I«interfaceName»Base::INTERFACE_NAME();
}

«FOR attribute : serviceInterface.attributes»
	«var attributeType = attribute.typeName»
	«var attributeName = attribute.joynrName»
	void «interfaceName»AbstractProvider::«attributeName»Changed(
			const «attributeType»& «attributeName»
	) {
		onAttributeValueChanged(
				"«attributeName»",
				«IF isArray(attribute)»
					«IF isEnum(attribute.type)»
						Variant::make<std::vector<Variant>>(Util::convertEnumVectorToVariantVector<«getTypeNameOfContainingClass(attribute.type.derived)»>(«attribute.joynrName»))
					«ELSE»
						Variant::make<std::vector<Variant>>(TypeUtil::toVectorOfVariants(«attribute.joynrName»))
					«ENDIF»
				«ELSE»
					Variant::make<«getTypeName(attribute.type)»>(«attribute.joynrName»)
				«ENDIF»
		);
	}
«ENDFOR»

«FOR broadcast : serviceInterface.broadcasts»
	«var broadcastName = broadcast.joynrName»
	void «interfaceName»AbstractProvider::fire«broadcastName.toFirstUpper»(
			«broadcast.commaSeperatedTypedConstOutputParameterList.substring(1)»
	) {
		std::vector<Variant> broadcastValues;
		«FOR parameter: getOutputParameters(broadcast)»
			«IF isArray(parameter)»
				«IF isEnum(parameter.type)»
					broadcastValues.push_back(Variant::make<std::vector<Variant>>(Util::convertEnumVectorToVariantVector<«getTypeNameOfContainingClass(parameter.type.derived)»>(«parameter.joynrName»)));
				«ELSE»
					broadcastValues.push_back(Variant::make<std::vector<Variant>>(TypeUtil::toVectorOfVariants(«parameter.joynrName»)));
				«ENDIF»
			«ELSE»
				broadcastValues.push_back(Variant::make<«getTypeName(parameter.type)»>(«parameter.joynrName»));
			«ENDIF»
		«ENDFOR»
		fireBroadcast("«broadcastName»", broadcastValues);
	}
«ENDFOR»
«getNamespaceEnder(serviceInterface)»
'''
}