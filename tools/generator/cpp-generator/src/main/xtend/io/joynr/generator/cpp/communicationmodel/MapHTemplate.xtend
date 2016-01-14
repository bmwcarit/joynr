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
import io.joynr.generator.templates.MapTemplate
import io.joynr.generator.templates.util.NamingUtil
import javax.inject.Inject
import org.franca.core.franca.FMapType

class MapHTemplate implements MapTemplate{

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension CppStdTypeUtil

	@Inject
	private extension NamingUtil

	@Inject
	private extension TemplateBase

	override generate(FMapType type)
'''
«val typeName = type.joynrName»
«val headerGuard = ("GENERATED_TYPE_"+getPackagePathWithJoynrPrefix(type, "_", true)+"_"+typeName+"_H").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»

#include <string>
#include <map>
#include "joynr/HashUtil.h"
#include "joynr/Util.h"

// include complex Datatype headers.
«FOR member: getRequiredIncludesFor(type)»
	#include «member»
«ENDFOR»

«getNamespaceStarter(type, true)»

«type.typeDefinition»

«getNamespaceEnder(type, true)»

#endif // «headerGuard»
'''

private def getTypeDefinition(FMapType type)'''
struct «type.joynrName» : public std::map<«type.keyType.typeName», «type.valueType.typeName»> {};
'''
}
