package io.joynr.generator.communicationmodel
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
import io.joynr.generator.util.EnumTemplate
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase
import org.franca.core.franca.FEnumerationType

class EnumTypeTemplate implements EnumTemplate{

	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase

	override generate(FEnumerationType enumType) {
		val packagePath = getPackagePathWithJoynrPrefix(enumType, ".")
		'''
		«warning()»

package «packagePath»;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

«generateEnumCode(enumType)»
'''
	}
}