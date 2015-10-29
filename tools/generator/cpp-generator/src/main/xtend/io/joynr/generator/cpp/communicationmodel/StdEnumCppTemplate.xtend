package io.joynr.generator.cpp.communicationmodel
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
import io.joynr.generator.templates.EnumTemplate
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FEnumerationType

class StdEnumCppTemplate implements EnumTemplate {

	@Inject
	private extension TemplateBase

	@Inject
	private extension CppStdTypeUtil

	@Inject
	private extension NamingUtil

	@Inject
	private extension JoynrCppGeneratorExtensions

	override generate(FEnumerationType type)
'''
«val typeName = type.joynrName»
«warning»
«getDllExportIncludeStatement()»

#include "«type.includeOfStd»"

«getNamespaceStarter(type, true)»

std::string «typeName»::getLiteral(«typeName»::«getNestedEnumName()» «typeName.toFirstLower»Value) {
	std::string literal;
	switch («typeName.toFirstLower»Value) {
	«FOR literal : getEnumElementsAndBaseEnumElements(type)»
		case «literal.joynrName»:
			literal = std::string("«literal.joynrName»");
			break;
	«ENDFOR»
	}
	return literal;
}

uint32_t «typeName»::getOrdinal(«typeName»::«getNestedEnumName()» «typeName.toFirstLower»Value) {
	return static_cast<uint32_t>(«typeName.toFirstLower»Value);
}

// Printing «typeName» with google-test and google-mock.
void PrintTo(const «typeName»::«getNestedEnumName()»& «typeName.toFirstLower»Value, ::std::ostream* os) {
	*os << "«typeName»::" << «typeName»::getLiteral(«typeName.toFirstLower»Value)
			<< " (" << «typeName»::getOrdinal(«typeName.toFirstLower»Value) << ")";
}

«getNamespaceEnder(type, true)»
'''
}