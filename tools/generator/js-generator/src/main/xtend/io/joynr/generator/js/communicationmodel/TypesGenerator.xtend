package io.joynr.generator.js.communicationmodel

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
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import io.joynr.generator.templates.util.NamingUtil
import java.io.File
import java.util.HashSet
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FType
import org.franca.core.franca.FMapType

class TypesGenerator {

	@Inject extension JoynrJSGeneratorExtensions
	@Inject extension EnumTypeGenerator
	@Inject extension CompoundTypeGenerator
	@Inject extension MapTypeGenerator
	@Inject private extension NamingUtil

	def generateTypes(Iterable<FType> types, IFileSystemAccess fsa) {
		var generatedTypes = new HashSet<Object>;

		for (type : types) {
			val path = type.buildPackagePath(File::separator, true)

			val fileName = path + File::separator + type.joynrName + ".js"
			if (clean) {
				fsa.deleteFile(fileName);
			}
			if (generate) {
				fsa.generateFile(
					fileName,
					generateType(type, generatedTypes)
				)
			}
		}
	}

	def generateType(FType type, HashSet<Object> generatedTypes) {
		if (type instanceof FEnumerationType) {
			generateEnumType(type)
		} else if (type instanceof FCompoundType) {
			generateCompoundType(type, generatedTypes)
		} else if (type instanceof FMapType) {
			generateMapType(type)
		}
	}
}
