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

import com.google.inject.assistedinject.Assisted
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.MapTemplate
import io.joynr.generator.templates.util.NamingUtil
import javax.inject.Inject
import org.franca.core.franca.FMapType

class MapCppTemplate extends MapTemplate {

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
«warning()»

#include «type.getIncludeOf(generateVersion)»

«getNamespaceStarter(type, true, generateVersion)»

const std::int32_t «typeName»::MAJOR_VERSION = «majorVersion»;
const std::int32_t «typeName»::MINOR_VERSION = «minorVersion»;

«getNamespaceEnder(type, true, generateVersion)»

'''

}
