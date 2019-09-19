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
import io.joynr.generator.templates.EnumTemplate
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FEnumerationType
import com.google.inject.assistedinject.Assisted

class EnumHTemplate extends EnumTemplate {

	@Inject extension TemplateBase

	@Inject extension JoynrCppGeneratorExtensions

	@Inject extension CppStdTypeUtil

	@Inject extension NamingUtil

	@Inject
	new(@Assisted FEnumerationType type) {
		super(type)
	}

	override generate()
'''
«val typeName = type.joynrName»
«val headerGuard = (getPackagePathWithJoynrPrefix(type, "_", true)+"_"+typeName+"_h").toUpperCase»
«warning»
#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»
#include <cstdint>
#include <ostream>
#include <string>

#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"

«IF type.hasExtendsDeclaration»
	#include «type.extendedType.includeOf»

«ENDIF»
«getNamespaceStarter(type, true)»

/**
 * @brief Enumeration wrapper class «typeName»
 *
 * @version «majorVersion».«minorVersion»
 */
struct «getDllExportMacro()»«typeName» : public joynr::exceptions::ApplicationExceptionError {
	«IF type.hasExtendsDeclaration»
		// This enum inherits enumeration values from «type.extendedType.typeName».
	«ENDIF»

	using ApplicationExceptionError::ApplicationExceptionError;
	«typeName»() = default;
	~«typeName»() override = default;

	/**
	«appendDoxygenSummaryAndWriteSeeAndDescription(type, " *")»
	 * @version «majorVersion».«minorVersion»
	 */
	enum «getNestedEnumName()» : std::uint32_t {
		«var ordinal = -1»
		«FOR enumtype : getEnumElementsAndBaseEnumElements(type) SEPARATOR ','»
			/**
			 * @brief «appendDoxygenComment(enumtype, "* ")»
			 */
			«{
				ordinal = if (enumtype.value.enumeratorValue === null)
							ordinal+1
						else
							Integer::valueOf(enumtype.value.enumeratorValue);
				""
			}»
			«enumtype.joynrName» = «ordinal»
		«ENDFOR»
	};

	/**
	 * @brief MAJOR_VERSION The major version of this struct as specified in the
	 * type collection or interface in the Franca model.
	 */
	static const std::int32_t MAJOR_VERSION;
	/**
	 * @brief MINOR_VERSION The minor version of this struct as specified in the
	 * type collection or interface in the Franca model.
	 */
	static const std::int32_t MINOR_VERSION;

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
	static std::string getLiteral(const «typeName»::«getNestedEnumName()»& «typeName.toFirstLower»Value);

	/**
	 * @brief Get the matching enum for a string
	 * @param «typeName.toFirstLower»String The string representing the enum value
	 * @return The enum value representing the string
	 */
	static «typeName»::«getNestedEnumName()» getEnum(const std::string& «typeName.toFirstLower»String);

	/**
	 * @brief Get the matching ordinal number for an enum
	 * @param «typeName.toFirstLower»Value The enum
	 * @return The ordinal number representing the enum
	 */
	static std::uint32_t getOrdinal(«typeName»::«getNestedEnumName()» «typeName.toFirstLower»Value);

	/**
	 * @brief Get the typeName of the enumeration type
	 * @return The typeName of the enumeration type
	 */
	static std::string getTypeName();
};

// Printing «typeName» with google-test and google-mock.
/**
 * @brief Print values of MessagingQos object
 * @param messagingQos The current object instance
 * @param os The output stream to send the output to
 */
void PrintTo(const «type.typeName»& «typeName.toFirstLower»Value, ::std::ostream* os);

«getNamespaceEnder(type, true)»

namespace std {

/**
 * @brief Function object that implements a hash function for «type.buildPackagePath("::", true)»::«typeName».
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template<>
struct hash<«type.buildPackagePath("::", true)»::«typeName»::«getNestedEnumName()»> {

	/**
	 * @brief method overriding default implementation of operator ()
	 * @param «typeName.toFirstLower»Value the operators argument
	 * @return the ordinal number representing the enum value
	 */
	std::size_t operator()(const «type.buildPackagePath("::", true)»::«typeName»::«getNestedEnumName()»& «typeName.toFirstLower»Value) const {
		return «type.buildPackagePath("::", true)»::«typeName»::getOrdinal(«typeName.toFirstLower»Value);
	}
};
} // namespace std

MUESLI_REGISTER_POLYMORPHIC_TYPE(«type.typeNameOfContainingClass», joynr::exceptions::ApplicationExceptionError, "«type.typeNameOfContainingClass.replace("::", ".")»")

namespace muesli
{
template <>
struct EnumTraits<«type.typeName»>
{
	using Wrapper = «type.typeNameOfContainingClass»;
};
} // namespace muesli

#endif // «headerGuard»
'''
}
