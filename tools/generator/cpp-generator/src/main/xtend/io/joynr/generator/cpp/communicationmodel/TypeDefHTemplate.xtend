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
import javax.inject.Inject
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FType
import org.franca.core.franca.FBasicTypeId

class TypeDefHTemplate implements TypeDefTemplate{

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension CppStdTypeUtil

	@Inject
	private extension NamingUtil

	@Inject
	private extension TemplateBase

	override generate(FTypeDef type)
'''
«val typeName = type.joynrName»
«val headerGuard = ("GENERATED_TYPE_"+getPackagePathWithJoynrPrefix(type, "_", true)+"_"+typeName+"_H").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»

// include required datatype headers.
«val typeDependencies = type.typeDependencies»
«FOR member: typeDependencies.filter(typeof(FBasicTypeId)).includesFor»
	#include «member»
«ENDFOR»
«FOR member: typeDependencies.filter(typeof(FType))»
	#include «member.includeOf»
«ENDFOR»

«getNamespaceStarter(type, true)»

«type.typeDefinition»

«getNamespaceEnder(type, true)»

#endif // «headerGuard»
'''

private def getTypeDefinition(FTypeDef type)'''
«IF isEnum(type)»
	typedef «type.actualType.typeName.substring(0, type.actualType.typeName.length-6)» «type.joynrName»;
«ELSE»
	typedef «type.actualType.typeName» «type.joynrName»;
«ENDIF»
'''

}
