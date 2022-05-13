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
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface
import org.franca.core.franca.FType
import io.joynr.generator.js.util.JsTemplateFactory

class ErrorEnumTypesGenerator {

	@Inject extension JoynrJSGeneratorExtensions
	@Inject extension NamingUtil
	@Inject extension InterfaceUtil

	@Inject JsTemplateFactory templateFactory

	def generateErrorEnumTypes(FInterface fInterface, Iterable<FType> types, IFileSystemAccess fsa, boolean generateVersion){
		var methodToErrorEnumName = fInterface.methodToErrorEnumName

		for (method: getMethods(fInterface)) {
			var enumType = method.errors;
			if (enumType !== null) {
				enumType.name = methodToErrorEnumName.get(method);
				val path = getPackagePathWithJoynrPrefix(enumType, "/", generateVersion)
				val fileName = path + "/" + enumType.joynrName + ".ts"
				if (clean) {
					fsa.deleteFile(fileName)
				}
				if (generate) {
					var enumTypeGenerator = templateFactory.createEnumTypeGenerator(enumType)
					fsa.generateFile(
						fileName,
						enumTypeGenerator.generate(generateVersion)
					)
				}
			}
		}
	}
}
