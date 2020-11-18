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
import com.google.inject.assistedinject.Assisted
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.EnumTemplate
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FEnumerationType

class EnumCppTemplate extends EnumTemplate {

	@Inject extension TemplateBase

	@Inject extension CppStdTypeUtil

	@Inject extension NamingUtil

	@Inject extension JoynrCppGeneratorExtensions

	@Inject
	new(@Assisted FEnumerationType type) {
		super(type)
	}

	override generate(boolean generateVersion)
'''
«val typeName = type.joynrName»
«warning»
«getDllExportIncludeStatement()»

#include «type.getIncludeOf(generateVersion)»
#include <sstream>

«getNamespaceStarter(type, true, generateVersion)»

const std::int32_t «typeName»::MAJOR_VERSION = «majorVersion»;
const std::int32_t «typeName»::MINOR_VERSION = «minorVersion»;

std::string «typeName»::getLiteral(const «typeName»::«getNestedEnumName()»& «typeName.toFirstLower»Value) {
	std::string literal;
	switch («typeName.toFirstLower»Value) {
	«FOR literal : getEnumElementsAndBaseEnumElements(type)»
		case «literal.joynrName»:
			literal = std::string("«literal.joynrName»");
			break;
	«ENDFOR»
	}
	if (literal.empty()) {
		throw std::invalid_argument("«typeName»: No literal found for value \"" + std::to_string(«typeName.toFirstLower»Value) + "\"");
	}
	return literal;
}

«typeName»::«getNestedEnumName()» «typeName»::getEnum(const std::string& «typeName.toFirstLower»String) {
	«FOR literal : getEnumElementsAndBaseEnumElements(type)»
		if («typeName.toFirstLower»String == std::string("«literal.joynrName»")) {
			return «literal.joynrName»;
		}
	«ENDFOR»
	std::stringstream errorMessage(«typeName.toFirstLower»String);
    errorMessage << " is unknown literal for «type.joynrName»";
    throw std::invalid_argument(errorMessage.str());
}

std::string «typeName»::getTypeName() {
	return "«type.buildPackagePath(".", true, generateVersion) + "." + type.joynrName»";
}

std::uint32_t «typeName»::getOrdinal(«typeName»::«getNestedEnumName()» «typeName.toFirstLower»Value) {
	return static_cast<std::uint32_t>(«typeName.toFirstLower»Value);
}

// Printing «typeName» with google-test and google-mock.
void PrintTo(const «typeName»::«getNestedEnumName()»& «typeName.toFirstLower»Value, ::std::ostream* os) {
	*os << "«typeName»::" << «typeName»::getLiteral(«typeName.toFirstLower»Value)
			<< " (" << «typeName»::getOrdinal(«typeName.toFirstLower»Value) << ")";
}

«getNamespaceEnder(type, true, generateVersion)»
'''
}
