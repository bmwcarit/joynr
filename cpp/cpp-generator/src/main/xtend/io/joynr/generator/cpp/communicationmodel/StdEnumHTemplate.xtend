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
import io.joynr.generator.util.EnumTemplate
import org.franca.core.franca.FEnumerationType

class StdEnumHTemplate implements EnumTemplate {

	@Inject
	private extension TemplateBase

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension CppStdTypeUtil

	override generate(FEnumerationType type)
'''
«val typeName = type.joynrNameStd»
«val headerGuard = (getPackagePathWithJoynrPrefix(type, "_")+"_"+typeName+"_h").toUpperCase»
«warning»
#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»
#include <cstdint>
#include <ostream>
#include <string>

«IF type.hasExtendsDeclaration»
	#include "«type.extendedType.includeOfStd»"

«ENDIF»
«getNamespaceStarter(type, true)»

/** @brief Enumeration wrapper class «typeName» */
struct «getDllExportMacro()»«typeName» {
	«IF type.hasExtendsDeclaration»
		// This enum inherits enumeration values from «type.extendedType.typeName».
	«ENDIF»
	/**
	«appendDoxygenSummaryAndWriteSeeAndDescription(type, " *")»
	 */
	enum «getNestedEnumName()» : uint32_t {
		«var ordinal = 0»
		«FOR enumtype : getEnumElementsAndBaseEnumElements(type) SEPARATOR ','»
			/**
			 * @brief «appendDoxygenComment(enumtype, "* ")»
			 */
			«enumtype.joynrName» = «ordinal++»
			««« TODO after switch to Franca 0.9.2 we must check for ordinals defined in the fidl
			««««enumtype.joynrName» = «enumtype.value.enumeratorValue»
		«ENDFOR»
	};

	/** @brief Constructor */
	«typeName»() = delete;

	/**
	 * @brief Copy constructor
	 * @param o the object to copy from
	 */
	«typeName»(const «typeName»& o) = delete;

	/**
	 * @brief Get the matching enum name for an ordinal number
	 * @param «typeName.toFirstLower»Value The ordinal number
	 * @return The string representing the enum for the given ordinal number
	 */
	static std::string getLiteral(«typeName»::«getNestedEnumName()» «typeName.toFirstLower»Value);

	/**
	 * @brief Get the matching ordinal number for an enum
	 * @param «typeName.toFirstLower»Value The enum
	 * @return The ordinal number representing the enum
	 */
	static uint32_t getOrdinal(«typeName»::«getNestedEnumName()» «typeName.toFirstLower»Value);
};

// Printing «typeName» with google-test and google-mock.
/**
 * @brief Print values of MessagingQos object
 * @param messagingQos The current object instance
 * @param os The output stream to send the output to
 */
void PrintTo(const «typeName»::«getNestedEnumName()»& «typeName.toFirstLower»Value, ::std::ostream* os);

«getNamespaceEnder(type, true)»

namespace std {
// Function object that implements a hash function for «type.buildPackagePath("::", true)»«typeName».
// Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
// std::unordered_map, std::unordered_multimap as default hash function.

/**
 * @brief Function object that implements a hash function for «type.buildPackagePath("::", true)»«typeName».
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template<>
struct hash<«type.buildPackagePath("::", true)»«typeName»::«getNestedEnumName()»> {

	/**
	 * @brief method overriding default implementation of operator ()
	 * @param «typeName.toFirstLower»Value the operators argument
	 * @return the ordinal number representing the enum value
	 */
	std::size_t operator()(const «type.buildPackagePath("::", true)»«typeName»::«getNestedEnumName()»& «typeName.toFirstLower»Value) const {
		return «type.buildPackagePath("::", true)»«typeName»::getOrdinal(«typeName.toFirstLower»Value);
	}
};
} /* namespace std */

#endif // «headerGuard»
'''
}
