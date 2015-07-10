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

import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import javax.inject.Inject
import org.franca.core.franca.FCompoundType
import io.joynr.generator.util.CompoundTypeTemplate

class StdTypeHTemplate implements CompoundTypeTemplate{

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension TemplateBase

	override generate(FCompoundType type)
'''
«val typeName = "Std" + type.joynrName»
«val headerGuard = ("GENERATED_TYPE_"+getPackagePathWithJoynrPrefix(type, "_")+"_"+typeName+"_H").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»

#include <string>
#include <vector>

// include complex Datatype headers.
«FOR member: getRequiredIncludesForStd(type)»
	#include "«member»"
«ENDFOR»

«getNamespaceStarter(type)»

class «getDllExportMacro()» «typeName» «IF hasExtendsDeclaration(type)»: public Std«getExtendedType(type).joynrName»«ENDIF»{

public:
	// general methods

	// default constructor
	«typeName»() = default;

	// constructor setting all fields
	«IF !getMembersRecursive(type).empty»
	explicit «typeName»(
			«FOR member: getMembersRecursive(type) SEPARATOR","»
				const «getMappedDatatypeOrListStd(member)» &«member.joynrName»
			«ENDFOR»
	);
	«ENDIF»

	«typeName»(const «typeName»& «typeName.toFirstLower»Obj)«IF getMembers(type).size == 0» = default«ENDIF»;

	virtual ~«typeName»() = default;

	virtual std::string toString() const;
	«typeName»& operator=(const «typeName»& «typeName.toFirstLower»Obj) = default;
	virtual bool operator==(const «typeName»& «typeName.toFirstLower»Obj) const;
	virtual bool operator!=(const «typeName»& «typeName.toFirstLower»Obj) const;

	// getters
	«FOR member: getMembers(type)»
		«val joynrName = member.joynrName»
		inline const «getMappedDatatypeOrListStd(member)»& get«joynrName.toFirstUpper»() const { return «joynrName»; }
	«ENDFOR»

	// setters
	«FOR member: getMembers(type)»
		«val joynrName = member.joynrName»
		inline void set«joynrName.toFirstUpper»(const «getMappedDatatypeOrListStd(member)»& «joynrName») { this->«joynrName» = «joynrName»; }
	«ENDFOR»

protected:
	// printing «typeName» with google-test and google-mock
	friend void PrintTo(const «typeName»& «typeName.toFirstLower», ::std::ostream* os);

private:
	// members
	«FOR member: getMembers(type)»
		«getMappedDatatypeOrListStd(member)» «member.joynrName»;
		«IF isEnum(member.type)»
			std::string get«member.joynrName.toFirstUpper»Internal() const;
		«ENDIF»
	«ENDFOR»
};

«getNamespaceEnder(type)»

#endif // «headerGuard»
'''
}
