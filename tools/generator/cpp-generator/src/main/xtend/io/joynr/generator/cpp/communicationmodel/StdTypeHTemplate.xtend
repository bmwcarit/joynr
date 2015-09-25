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

import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.util.CompoundTypeTemplate
import javax.inject.Inject
import org.franca.core.franca.FCompoundType

class StdTypeHTemplate implements CompoundTypeTemplate{

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension CppStdTypeUtil

	@Inject
	private extension TemplateBase

	override generate(FCompoundType type)
'''
«val typeName = type.joynrNameStd»
«val headerGuard = ("GENERATED_TYPE_"+getPackagePathWithJoynrPrefix(type, "_")+"_"+typeName+"_H").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»

#include <string>
#include <vector>

// include complex Datatype headers.
«FOR member: getRequiredIncludesFor(type)»
	#include "«member»"
«ENDFOR»

«getNamespaceStarter(type, true)»

/**
«appendDoxygenSummaryAndWriteSeeAndDescription(type, " *")»
 */
class «getDllExportMacro()» «typeName» «IF hasExtendsDeclaration(type)»: public «getExtendedType(type).joynrNameStd»«ENDIF»{

public:
	// general methods

	// default constructor
	/** @brief Constructor */
	«typeName»();

	// constructor setting all fields
	«IF !getMembersRecursive(type).empty»
	/**
	 * @brief Parameterized constructor
	 «FOR member: getMembersRecursive(type)»
	 «appendDoxygenParameter(member, "*")»
	 «ENDFOR»
	 */
	explicit «typeName»(
			«FOR member: getMembersRecursive(type) SEPARATOR","»
				const «member.typeName» &«member.joynrName»
			«ENDFOR»
	);
	«ENDIF»

	/** @brief Copy constructor */
	«typeName»(const «typeName»& «typeName.toFirstLower»Obj)«IF getMembers(type).size == 0» = default«ENDIF»;

	/** @brief Destructor */
	virtual ~«typeName»() = default;

	/**
	 * @brief Stringifies the class
	 * @return stringified class content
	 */
	virtual std::string toString() const;

	/**
	 * @brief assigns an object
	 * @return reference to the object assigned to
	 */
	«typeName»& operator=(const «typeName»& «typeName.toFirstLower»Obj) = default;

	/**
	 * @brief equality operator
	 * @param «typeName.toFirstLower»Obj reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	virtual bool operator==(const «typeName»& «typeName.toFirstLower»Obj) const;

	/**
	 * @brief unequality operator
	 * @param «typeName.toFirstLower»Obj reference to the object to compare to
	 * @return true if objects are not equal, false otherwise
	 */
	virtual bool operator!=(const «typeName»& «typeName.toFirstLower»Obj) const;

	// getters
	«FOR member: getMembers(type)»
		«val joynrName = member.joynrName»
		/**
		 * @brief Gets «joynrName.toFirstUpper»
		 * @return «appendDoxygenComment(member, "* ")»
		 */
		inline const «member.typeName»& get«joynrName.toFirstUpper»() const { return «joynrName»; }
	«ENDFOR»

	// setters
	«FOR member: getMembers(type)»
		«val joynrName = member.joynrName»
		/**
		 * @brief Sets «joynrName.toFirstUpper»
		 «appendDoxygenParameter(member, "*")»
		 */
		inline void set«joynrName.toFirstUpper»(const «member.typeName»& «joynrName») { this->«joynrName» = «joynrName»; }
	«ENDFOR»

protected:
	// printing «typeName» with google-test and google-mock
	/**
	 * @brief Print values of a «typeName» object
	 * @param «typeName.toFirstLower» The current object instance
	 * @param os The output stream to send the output to
	 */
	friend void PrintTo(const «typeName»& «typeName.toFirstLower», ::std::ostream* os);

private:
	// members
	«FOR member: getMembers(type)»
		«member.typeName» «member.joynrName»;
		«IF isEnum(member.type)»
			std::string get«member.joynrName.toFirstUpper»Internal() const;
		«ENDIF»
	«ENDFOR»
};

«getNamespaceEnder(type, true)»

#endif // «headerGuard»
'''
}
