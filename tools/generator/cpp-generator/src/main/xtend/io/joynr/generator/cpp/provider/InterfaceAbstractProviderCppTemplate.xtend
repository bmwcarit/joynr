package io.joynr.generator.cpp.provider
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
import com.google.inject.assistedinject.Assisted
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceAbstractProviderCppTemplate extends InterfaceTemplate {

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension BroadcastUtil

	@Inject
	new(@Assisted FInterface francaIntf) {
		super(francaIntf)
	}

	override generate()
'''
«warning()»
«val interfaceName = francaIntf.joynrName»
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»AbstractProvider.h"
#include "joynr/InterfaceRegistrar.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»RequestInterpreter.h"
#include "joynr/TypeUtil.h"

«FOR parameterType: getRequiredIncludesFor(francaIntf)»
	#include «parameterType»
«ENDFOR»

«getNamespaceStarter(francaIntf)»
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

«FOR attribute : francaIntf.attributes»
	«IF attribute.notifiable»
		«var attributeType = attribute.type.resolveTypeDef»
		«var attributeName = attribute.joynrName»
		void «interfaceName»AbstractProvider::«attributeName»Changed(
				const «attribute.typeName»& «attributeName»
		) {
			onAttributeValueChanged(
					"«attributeName»",
					«IF attributeType.isEnum && attribute.isArray»
						joynr::TypeUtil::toVariant(util::convertEnumVectorToVariantVector<«getTypeNameOfContainingClass(attributeType.derived)»>(«attributeName»))
					«ELSEIF attributeType.isEnum»
						Variant::make<«attribute.typeName»>(«attributeName»)
					«ELSEIF attribute.isArray»
						joynr::TypeUtil::toVariant<«attributeType.typeName»>(«attributeName»)
					«ELSEIF attributeType.isCompound»
						Variant::make<«attribute.typeName»>(«attributeName»)
					«ELSEIF attributeType.isMap»
						Variant::make<«attribute.typeName»>(«attributeName»)
					«ELSEIF attributeType.isByteBuffer»
						joynr::TypeUtil::toVariant(«attributeName»)
					«ELSE»
						Variant::make<«attribute.typeName»>(«attributeName»)
					«ENDIF»
			);
		}
	«ENDIF»
«ENDFOR»

«FOR broadcast : francaIntf.broadcasts»
	«var broadcastName = broadcast.joynrName»
	void «interfaceName»AbstractProvider::fire«broadcastName.toFirstUpper»(
			«IF !broadcast.outputParameters.empty»
			«broadcast.commaSeperatedTypedConstOutputParameterList»
			«ENDIF»
	) {
		std::vector<Variant> broadcastValues;
		«FOR param: getOutputParameters(broadcast)»
			«var paramType = param.type.resolveTypeDef»
			«var paramName = param.joynrName»
			broadcastValues.push_back(
					«IF paramType.isEnum && param.isArray»
						joynr::TypeUtil::toVariant(util::convertEnumVectorToVariantVector<«getTypeNameOfContainingClass(paramType.derived)»>(«paramName»))
					«ELSEIF paramType.isEnum»
						Variant::make<«param.typeName»>(«paramName»)
					«ELSEIF param.isArray»
						joynr::TypeUtil::toVariant<«paramType.typeName»>(«paramName»)
					«ELSEIF paramType.isCompound»
						Variant::make<«param.typeName»>(«paramName»)
					«ELSEIF paramType.isMap»
						Variant::make<«param.typeName»>(«paramName»)
					«ELSEIF paramType.isByteBuffer»
						joynr::TypeUtil::toVariant(«paramName»)
					«ELSE»
						Variant::make<«param.typeName»>(«paramName»)
					«ENDIF»
			);
		«ENDFOR»
		fireBroadcast("«broadcastName»", broadcastValues);
	}
«ENDFOR»
«getNamespaceEnder(francaIntf)»
'''
}