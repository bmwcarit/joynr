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

import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.TypeDefTemplate
import io.joynr.generator.templates.util.NamingUtil
import com.google.inject.Inject
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FType
import org.franca.core.franca.FBasicTypeId

class TypeDefHTemplate implements TypeDefTemplate{

	@Inject extension JoynrCppGeneratorExtensions

	@Inject extension CppStdTypeUtil

	@Inject extension NamingUtil

	@Inject extension TemplateBase

	override generate(FTypeDef type, boolean generateVersion)
'''
«val typeName = type.joynrName»
«val headerGuard = ("GENERATED_TYPE_"+getPackagePathWithJoynrPrefix(type, "_", true, generateVersion)+"_"+typeName+"_H").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»


// include required datatype headers.
«val typeDependencies = type.typeDependencies»
«FOR member: typeDependencies.filter(typeof(FBasicTypeId)).includesFor»
	#include «member»
«ENDFOR»
«FOR member: typeDependencies.filter(typeof(FType))»
	#include «member.getIncludeOf(generateVersion)»
«ENDFOR»

«getNamespaceStarter(type, true, generateVersion)»

«type.getTypeDefinition(generateVersion)»

«getNamespaceEnder(type, true, generateVersion)»

#endif // «headerGuard»
'''

private def getTypeDefinition(FTypeDef type, boolean generateVersion)'''
«IF isEnum(type)»
	typedef «type.actualType.getTypeName(generateVersion).substring(0, type.actualType.getTypeName(generateVersion).length-6)» «type.joynrName»;
«ELSE»
	typedef «type.actualType.getTypeName(generateVersion)» «type.joynrName»;
«ENDIF»
'''

}
