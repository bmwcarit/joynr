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
import io.joynr.generator.templates.CompoundTypeTemplate
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FCompoundType
import com.google.inject.assistedinject.Assisted

class TypeCppTemplate extends CompoundTypeTemplate {

	@Inject extension TemplateBase

	@Inject extension JoynrCppGeneratorExtensions

	@Inject extension CppStdTypeUtil

	@Inject extension NamingUtil

	@Inject
	new(@Assisted FCompoundType type) {
		super(type)
	}

	override generate(boolean generateVersion) '''
«val typeName = type.joynrName»
«warning»

#include <sstream>
#include <string>

«IF type.hasExtendsDeclaration || getMembers(type).size > 0»
#include <boost/functional/hash.hpp>
«ENDIF»
#include "joynr/HashUtil.h"
#include «type.getIncludeOf(generateVersion)»

«getNamespaceStarter(type, true, generateVersion)»

const std::int32_t «typeName»::MAJOR_VERSION = «majorVersion»;
const std::int32_t «typeName»::MINOR_VERSION = «minorVersion»;

«typeName»::«typeName»()«IF !getMembersRecursive(type).empty»:«ENDIF»
	«IF hasExtendsDeclaration(type)»
		«getExtendedType(type).getTypeName(generateVersion)»()«IF !getMembers(type).empty»,«ENDIF»
	«ENDIF»
	«FOR member: getMembers(type) SEPARATOR ','»
		«member.joynrName»(«member.getDefaultValue(generateVersion)»)
	«ENDFOR»
{
}

«IF !getMembersRecursive(type).empty»
«typeName»::«typeName»(
		«FOR member: getMembersRecursive(type) SEPARATOR ','»
			const «member.getTypeName(generateVersion)»& _«member.joynrName»
		«ENDFOR»
	):
		«IF hasExtendsDeclaration(type)»
			«val extendedType = getExtendedType(type)»
			«extendedType.getTypeName(generateVersion)»(
			«FOR member: getMembersRecursive(extendedType) SEPARATOR ','»
				_«member.joynrName»
			«ENDFOR»
			)
			«IF !getMembers(type).isEmpty()»
				,
			«ENDIF»
		«ENDIF»
		«FOR member: getMembers(type) SEPARATOR ','»
			«member.joynrName»(_«member.joynrName»)
		«ENDFOR»
{
}

«ENDIF»

std::size_t «typeName»::hashCode() const {
	std::size_t seed = 0;

	«FOR member: getMembers(type)»
		«val joynrName = member.joynrName»
		boost::hash_combine(seed, get«joynrName.toFirstUpper»());
	«ENDFOR»

	«IF type.hasExtendsDeclaration»
		boost::hash_combine(seed, «type.extendedType.joynrName»::hashCode());
	«ENDIF»
	return seed;
}

«FOR member: getMembers(type)»
	«val joynrName = member.joynrName»
	«IF isEnum(member.type) && ! isArray(member)»
		std::string «typeName»::get«joynrName.toFirstUpper»Internal() const {
			return «member.getTypeName(generateVersion).substring(0, member.getTypeName(generateVersion).length-6)»::getLiteral(this->«joynrName»);
		}

	«ENDIF»
«ENDFOR»
std::string «typeName»::toString() const {
	std::ostringstream typeAsString;
	typeAsString << "«typeName»{";
	«IF hasExtendsDeclaration(type)»
		typeAsString << «getExtendedType(type).getTypeName(generateVersion)»::toString();
		«IF !getMembers(type).empty»
		typeAsString << ", ";
		«ENDIF»
	«ENDIF»
	«FOR member: getMembers(type) SEPARATOR "\ntypeAsString << \", \";"»
		«val memberName = member.joynrName»
		«val memberType = member.type.resolveTypeDef»
		«IF member.isArray»
			typeAsString << " unprinted List «memberName»  ";
		«ELSEIF memberType.isByteBuffer»
			typeAsString << " unprinted ByteBuffer «memberName»  ";
		«ELSEIF memberType.isString»
			typeAsString << "«memberName»:" + get«memberName.toFirstUpper»();
		«ELSEIF memberType.isEnum»
			typeAsString << "«memberName»:" + get«memberName.toFirstUpper»Internal();
		«ELSEIF memberType.isCompound»
			typeAsString << "«memberName»:" + get«memberName.toFirstUpper»().toString();
		«ELSEIF memberType.isMap»
			typeAsString << " unprinted Map «memberName»  ";
		«ELSE»
			typeAsString << "«memberName»:" + std::to_string(get«memberName.toFirstUpper»());
		«ENDIF»
	«ENDFOR»
	typeAsString << "}";
	return typeAsString.str();
}

// printing «typeName» with google-test and google-mock
void PrintTo(const «typeName»& «typeName.toFirstLower», ::std::ostream* os) {
	*os << "«typeName»::" << «typeName.toFirstLower».toString();
}

std::size_t hash_value(const «typeName»& «typeName.toFirstLower»Value)
{
	return «typeName.toFirstLower»Value.hashCode();
}

«IF isPolymorphic(type)»
std::unique_ptr<«getRootType(type).getTypeName(generateVersion)»> «typeName»::clone() const {
	return std::make_unique<«typeName»>(const_cast<«typeName»&>(*this));
}
«ENDIF»

«getNamespaceEnder(type, true, generateVersion)»
'''
}
