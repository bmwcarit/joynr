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

import com.google.inject.Inject
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.CppTemplateFactory
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FModel

class CommunicationModelGenerator {

	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension CppStdTypeUtil

	@Inject TypeDefHTemplate typeDefH;

	@Inject CppTemplateFactory templateFactory;

	def doGenerate(FModel fModel,
		IFileSystemAccess sourceFileSystem,
		IFileSystemAccess headerFileSystem,
		String sourceContainerPath,
		String headerContainerPath
	){
		val dataTypePath = sourceContainerPath + "datatypes" + File::separator
		val headerDataTypePath =
			if (sourceFileSystem == headerFileSystem)
				headerContainerPath + "datatypes" + File::separator
			else
				headerContainerPath

		for( type: getCompoundDataTypes(fModel)){
			var sourcepath = dataTypePath + getPackageSourceDirectory(type) + File::separator
			var headerpath = headerDataTypePath + getPackagePathWithJoynrPrefix(type, File::separator) + File::separator
			if (type.isPartOfNamedTypeCollection) {
				headerpath += type.typeCollectionName + File::separator
				sourcepath += type.typeCollectionName + File::separator
			}

			var typeHTemplate = templateFactory.createTypeHTemplate(type)
			generateFile(
				headerFileSystem,
				headerpath + getGenerationTypeName(type) + ".h",
				typeHTemplate
			)

			var typeCppTemplate = templateFactory.createTypeCppTemplate(type)
			generateFile(
				sourceFileSystem,
				sourcepath + getGenerationTypeName(type) + ".cpp",
				typeCppTemplate
			)
		}

		for (type : getEnumDataTypes(fModel)) {
			var sourcepath = dataTypePath + getPackageSourceDirectory(type) + File::separator
			var headerpath = headerDataTypePath + getPackagePathWithJoynrPrefix(type, File::separator) + File::separator
			if (type.isPartOfNamedTypeCollection) {
				headerpath += type.typeCollectionName + File::separator
				sourcepath += type.typeCollectionName + File::separator
			}

			generateEnum(
				headerFileSystem,
				sourceFileSystem,
				type,
				headerpath + getGenerationTypeName(type),
				sourcepath + getGenerationTypeName(type)
			);

		}

		for (type : getMapDataTypes(fModel)) {
			var sourcepath = dataTypePath + getPackageSourceDirectory(type) + File::separator
			var headerpath = headerDataTypePath + getPackagePathWithJoynrPrefix(type, File::separator) + File::separator
			if (type.isPartOfNamedTypeCollection) {
				headerpath += type.typeCollectionName + File::separator
				sourcepath += type.typeCollectionName + File::separator
			}
			val headerFilename = headerpath + getGenerationTypeName(type)
			val sourceFilename = sourcepath + getGenerationTypeName(type)

			var mapHTemplate = templateFactory.createMapHTemplate(type)
			generateFile(
				headerFileSystem,
				headerFilename + ".h",
				mapHTemplate
			)

			var mapCppTemplate = templateFactory.createMapCppTemplate(type)
			generateFile(
				sourceFileSystem,
				sourceFilename + ".cpp",
				mapCppTemplate
			)
		}


		for (type : getTypeDefDataTypes(fModel)) {
			var headerpath = headerDataTypePath + getPackagePathWithJoynrPrefix(type, File::separator) + File::separator
			if (type.isPartOfNamedTypeCollection) {
				headerpath += type.typeCollectionName + File::separator
			}
			val headerFilename = headerpath + getGenerationTypeName(type)

			generateFile(
				headerFileSystem,
				headerFilename + ".h",
				typeDefH,
				type
			)
		}
	}

	def generateEnum (
		IFileSystemAccess headerFileSystem,
		IFileSystemAccess sourceFileSystem,
		FEnumerationType enumType,
		String headerFilename,
		String sourceFilename
	) {
		var enumHTemplate = templateFactory.createEnumHTemplate(enumType)
		generateFile(
			headerFileSystem,
			headerFilename + ".h",
			enumHTemplate
		)

		var enumCppTemplate = templateFactory.createEnumCppTemplate(enumType)
		generateFile(
			sourceFileSystem,
			sourceFilename + ".cpp",
			enumCppTemplate
		)
	}
}
