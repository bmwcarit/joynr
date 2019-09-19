package io.joynr.generator.cpp.communicationmodel
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
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil.TypeSelector
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import java.util.HashSet
import org.franca.core.franca.FInterface

class InterfaceCppTemplate extends InterfaceTemplate {

	@Inject extension JoynrCppGeneratorExtensions

	@Inject extension CppStdTypeUtil

	@Inject extension NamingUtil

	@Inject extension MethodUtil

	@Inject extension AttributeUtil

	@Inject extension BroadcastUtil

	@Inject extension TemplateBase

	override generate() {
		var selector = TypeSelector::defaultTypeSelector
		selector.transitiveTypes(true)
'''
«val interfaceName = francaIntf.joynrName»
«warning()»

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/I«interfaceName».h"

«FOR parameterType: getDataTypeIncludesFor(francaIntf)»
	#include «parameterType»
«ENDFOR»

«getNamespaceStarter(francaIntf)»

const std::string& I«interfaceName»Base::INTERFACE_NAME()
{
	static const std::string INTERFACE_NAME("«francaIntf.fullyQualifiedName»");
	return INTERFACE_NAME;
}

const std::int32_t I«interfaceName»Base::MAJOR_VERSION = «majorVersion»;
const std::int32_t I«interfaceName»Base::MINOR_VERSION = «minorVersion»;

«getNamespaceEnder(francaIntf)»
'''
}
def getReplyMetatypes(FInterface serviceInterface) {
	var replyMetatypes = new HashSet();
	for (method: serviceInterface.methods) {
		if (!method.outputParameters.empty) {
			replyMetatypes.add(method.commaSeparatedOutputParameterTypes)
		}
	}
	for (attribute: serviceInterface.attributes) {
		if (attribute.readable) {
			replyMetatypes.add(attribute.typeName);
		}
	}
	return replyMetatypes;
}
def getBroadcastMetatypes(FInterface serviceInterface) {
	var broadcastMetatypes = new HashSet();
	for (broadcast: serviceInterface.broadcasts) {
		if (!broadcast.outputParameters.empty) {
			broadcastMetatypes.add(broadcast.commaSeparatedOutputParameterTypes)
		}
	}
	return broadcastMetatypes;
}
}
