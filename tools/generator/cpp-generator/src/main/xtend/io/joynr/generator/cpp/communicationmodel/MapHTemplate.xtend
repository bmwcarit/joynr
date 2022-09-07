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
import io.joynr.generator.templates.MapTemplate
import io.joynr.generator.templates.util.NamingUtil
import javax.inject.Inject
import org.franca.core.franca.FMapType
import org.franca.core.franca.FType
import org.franca.core.franca.FBasicTypeId
import com.google.inject.assistedinject.Assisted

class MapHTemplate extends MapTemplate {

	@Inject extension JoynrCppGeneratorExtensions

	@Inject extension CppStdTypeUtil

	@Inject extension NamingUtil

	@Inject extension TemplateBase

	@Inject
	new(@Assisted FMapType type) {
		super(type)
	}

	override generate(boolean generateVersion)
'''
«val typeName = type.joynrName»
«val headerGuard = ("GENERATED_TYPE_"+getPackagePathWithJoynrPrefix(type, "_", true, generateVersion)+"_"+typeName+"_H").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»


#include <map>
#include "joynr/HashUtil.h"

// include complex Datatype headers.
«val typeDependencies = type.typeDependencies»
«FOR member: typeDependencies.filter(typeof(FBasicTypeId)).includesFor»
	#include «member»
«ENDFOR»
«FOR member: typeDependencies.filter(typeof(FType))»
	#include «member.getIncludeOf(generateVersion)»
«ENDFOR»

#include "joynr/serializer/Serializer.h"

«getNamespaceStarter(type, true, generateVersion)»

«type.getTypeDefinition(generateVersion)»

«getNamespaceEnder(type, true, generateVersion)»

MUESLI_REGISTER_TYPE(«type.getTypeName(generateVersion)», "«type.getTypeName(generateVersion).replace("::", ".")»")

#endif // «headerGuard»
'''

private def getTypeDefinition(FMapType type, boolean generateVersion)
'''
«val mapType = "std::map<"  + type.keyType.getTypeName(generateVersion) + ", " + type.valueType.getTypeName(generateVersion) + ">"»
/**
 * @brief Map class «type.joynrName»
 *
 * @version «majorVersion».«minorVersion»
 */
class «type.joynrName» : public «mapType»
{
public:
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

private:
	using «mapType»::map;
};
'''
}
