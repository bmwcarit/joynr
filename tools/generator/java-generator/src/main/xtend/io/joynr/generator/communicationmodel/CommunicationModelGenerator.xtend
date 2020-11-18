package io.joynr.generator.communicationmodel
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
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.templates.util.TypeUtil
import io.joynr.generator.util.JavaTemplateFactory
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FModel

class CommunicationModelGenerator {

	@Inject
	extension JoynrJavaGeneratorExtensions

	@Inject extension TypeUtil

	@Inject extension NamingUtil

	@Inject JavaTemplateFactory templateFactory

	def doGenerate(FModel fModel, IFileSystemAccess fsa, boolean generateVersion){
		for( type: getCompoundDataTypes(fModel)){
			var path = getPackagePathWithJoynrPrefix(type, File::separator, generateVersion) + File::separator
			if (type.isPartOfNamedTypeCollection) {
				path += type.typeCollectionName + File::separator
			}
			var complexTypeTemplate = templateFactory.createComplexTypeTemplate(type)
			generateFile(
				fsa,
				path + type.joynrName + ".java",
				complexTypeTemplate,
				generateVersion
			)
		}

		for( type: getEnumDataTypes(fModel)){
			var path = getPackagePathWithJoynrPrefix(type, File::separator, generateVersion) + File::separator
			if (type.isPartOfNamedTypeCollection) {
				path += type.typeCollectionName + File::separator
			}
			var enumTypeTemplate = templateFactory.createEnumTypeTemplate(type)
			generateFile(
				fsa,
				path + type.joynrName + ".java",
				enumTypeTemplate,
				generateVersion
			)
		}

		for( type: getMapDataTypes(fModel)){
			var path = getPackagePathWithJoynrPrefix(type, File::separator, generateVersion) + File::separator
			if (type.isPartOfNamedTypeCollection) {
				path += type.typeCollectionName + File::separator
			}
			var mapTypeTemplate = templateFactory.createMapTypeTemplate(type)
			generateFile(
				fsa,
				path + type.joynrName + ".java",
				mapTypeTemplate,
				generateVersion
			)
		}
	}
}
