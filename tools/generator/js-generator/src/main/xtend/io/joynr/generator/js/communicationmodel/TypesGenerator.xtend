package io.joynr.generator.js.communicationmodel

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
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import io.joynr.generator.js.util.JsTemplateFactory
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.templates.util.TypeUtil
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FMapType
import org.franca.core.franca.FType

class TypesGenerator {

	@Inject extension JoynrJSGeneratorExtensions
	@Inject extension TypeUtil
	@Inject extension NamingUtil
	@Inject JsTemplateFactory templateFactory

	def generateTypes(Iterable<FType> types, IFileSystemAccess fsa, boolean generateVersion) {
		for (type : filterComplex(types)) {
			val path = type.buildPackagePath("/", true, generateVersion)

			val fileName = path + "/" + type.joynrName + ".ts"
			if (clean) {
				fsa.deleteFile(fileName);
			}
			if (generate) {
				fsa.generateFile(
					fileName,
					generateType(type, generateVersion)
				)
			}
		}
	}

	def generateType(FType type, boolean generateVersion) {
		if (type instanceof FEnumerationType) {
			var enumTypeGenerator = templateFactory.createEnumTypeGenerator(type)
			enumTypeGenerator.generate(generateVersion)
		} else if (type instanceof FCompoundType) {
			var compoundTypeGenerator = templateFactory.createCompoundTypeGenerator(type)
			compoundTypeGenerator.generate(generateVersion)
		} else if (type instanceof FMapType) {
			var mapTypeGenerator = templateFactory.createMapTypeGenerator(type)
			mapTypeGenerator.generate(generateVersion)
		}
	}
}
