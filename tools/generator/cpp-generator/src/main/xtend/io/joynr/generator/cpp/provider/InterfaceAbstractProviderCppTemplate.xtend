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
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceAbstractProviderCppTemplate implements InterfaceTemplate {

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
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
	«IF attribute.notifiable»
		«var attributeType = attribute.typeName»
		«var attributeName = attribute.joynrName»
		void «interfaceName»AbstractProvider::«attributeName»Changed(
				const «attributeType»& «attributeName»
		) {
			onAttributeValueChanged(
					"«attributeName»",
					«IF isEnum(attribute.type) && isArray(attribute)»
						joynr::TypeUtil::toVariant(Util::convertEnumVectorToVariantVector<«getTypeNameOfContainingClass(attribute.type.derived)»>(«attribute.joynrName»))
					«ELSEIF isEnum(attribute.type)»
						Variant::make<«getTypeName(attribute)»>(«attribute.joynrName»)
					«ELSEIF isArray(attribute)»
						joynr::TypeUtil::toVariant<«getTypeName(attribute.type)»>(«attribute.joynrName»)
					«ELSEIF isCompound(attribute.type)»
						Variant::make<«getTypeName(attribute)»>(«attribute.joynrName»)
					«ELSEIF isMap(attribute.type)»
						Variant::make<«getTypeName(attribute)»>(«attribute.joynrName»)
					«ELSEIF isByteBuffer(attribute.type)»
						joynr::TypeUtil::toVariant(«attribute.joynrName»)
					«ELSE»
						Variant::make<«getTypeName(attribute)»>(«attribute.joynrName»)
					«ENDIF»
			);
		}
	«ENDIF»
«ENDFOR»

«FOR broadcast : serviceInterface.broadcasts»
	«var broadcastName = broadcast.joynrName»
	void «interfaceName»AbstractProvider::fire«broadcastName.toFirstUpper»(
			«broadcast.commaSeperatedTypedConstOutputParameterList.substring(1)»
	) {
		std::vector<Variant> broadcastValues;
		«FOR param: getOutputParameters(broadcast)»
			broadcastValues.push_back(
					«IF isEnum(param.type) && isArray(param)»
						joynr::TypeUtil::toVariant(Util::convertEnumVectorToVariantVector<«getTypeNameOfContainingClass(param.type.derived)»>(«param.joynrName»))
					«ELSEIF isEnum(param.type)»
						Variant::make<«getTypeName(param)»>(«param.joynrName»)
					«ELSEIF isArray(param)»
						joynr::TypeUtil::toVariant<«getTypeName(param.type)»>(«param.joynrName»)
					«ELSEIF isCompound(param.type)»
						Variant::make<«getTypeName(param)»>(«param.joynrName»)
					«ELSEIF isMap(param.type)»
						Variant::make<«getTypeName(param)»>(«param.joynrName»)
					«ELSEIF isByteBuffer(param.type)»
						joynr::TypeUtil::toVariant(«param.joynrName»)
					«ELSE»
						Variant::make<«getTypeName(param)»>(«param.joynrName»)
					«ENDIF»
			);
		«ENDFOR»
		fireBroadcast("«broadcastName»", broadcastValues);
	}
«ENDFOR»
«getNamespaceEnder(serviceInterface)»
'''
}