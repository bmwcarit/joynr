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

import com.google.inject.Inject
import io.joynr.generator.cpp.communicationmodel.qt.EnumHTemplate
import io.joynr.generator.cpp.communicationmodel.qt.TypeCppTemplate
import io.joynr.generator.cpp.communicationmodel.qt.TypeHTemplate
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.QtTypeUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.templates.util.TypeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModel

class CommunicationModelGenerator {

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension TypeUtil

	@Inject
	private QtTypeUtil qtTypeUtil

	@Inject
	private CppStdTypeUtil stdTypeUtil

	@Inject
	private extension NamingUtil

	@Inject
	private extension InterfaceUtil

	@Inject
	InterfaceHTemplate interfaceH;

	@Inject
	InterfaceCppTemplate interfaceCpp;

	@Inject
	EnumHTemplate enumh;

	@Inject
	StdEnumHTemplate stdEnumH;

	@Inject
	StdEnumCppTemplate stdEnumCpp;

	@Inject
	TypeHTemplate typeH;

	@Inject
	TypeCppTemplate typeCpp;

	@Inject
	StdTypeHTemplate stdTypeH;

	@Inject
	StdTypeCppTemplate stdTypeCpp;

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

		for( type: getComplexDataTypes(fModel)){
			if(type instanceof FCompoundType) {
				var sourcepath = dataTypePath + getPackageSourceDirectory(type) + File::separator
				var headerpath = headerDataTypePath + getPackagePathWithJoynrPrefix(type, File::separator) + File::separator
				var sourcepathQt = sourcepath
				var headerpathQt = headerpath

				if (type.isPartOfTypeCollection) {
					headerpath += type.typeCollectionName + File::separator
					sourcepath += type.typeCollectionName + File::separator
					headerpathQt += type.typeCollectionName + "_"
					sourcepathQt += type.typeCollectionName + "_"
				}

				generateFile(
					headerFileSystem,
					headerpathQt + qtTypeUtil.getGenerationTypeName(type) + ".h",
					typeH,
					type
				)

				generateFile(
					sourceFileSystem,
					sourcepathQt + qtTypeUtil.getGenerationTypeName(type) + ".cpp",
					typeCpp,
					type
				)

				generateFile(
					headerFileSystem,
					headerpath + stdTypeUtil.getGenerationTypeName(type) + ".h",
					stdTypeH,
					type
				)

				generateFile(
					sourceFileSystem,
					sourcepath + stdTypeUtil.getGenerationTypeName(type) + ".cpp",
					stdTypeCpp,
					type
				)
			}
		}

		for (type : getEnumDataTypes(fModel)) {
			var sourcepath = dataTypePath + getPackageSourceDirectory(type) + File::separator
			var headerpath = headerDataTypePath + getPackagePathWithJoynrPrefix(type, File::separator) + File::separator
			var headerpathQt = headerpath
			if (type.isPartOfTypeCollection) {
				headerpath += type.typeCollectionName + File::separator
				sourcepath += type.typeCollectionName + File::separator
				headerpathQt += type.typeCollectionName + "_"
			}

			generateEnum(
				headerFileSystem,
				sourceFileSystem,
				type as FEnumerationType,
				headerpathQt + qtTypeUtil.getGenerationTypeName(type) + ".h",
				headerpath + stdTypeUtil.getGenerationTypeName(type) + ".h",
				sourcepath + stdTypeUtil.getGenerationTypeName(type) + ".cpp"
			);
		}

		val interfacePath = sourceContainerPath + "interfaces" + File::separator
		val headerInterfacePath = 
			if (sourceFileSystem == headerFileSystem) 
				headerContainerPath + "interfaces" + File::separator
			else
				headerContainerPath
		
		for(serviceInterface: fModel.interfaces){
			val sourcepath = interfacePath + getPackageSourceDirectory(serviceInterface) + File::separator 
			val headerpath = headerInterfacePath + getPackagePathWithJoynrPrefix(serviceInterface, File::separator) + File::separator 

			generateFile(
				headerFileSystem,
				headerpath + "I" + serviceInterface.joynrName + ".h",
				interfaceH,
				serviceInterface
			);
			
			generateFile(
				sourceFileSystem,
				sourcepath + "I" + serviceInterface.joynrName + ".cpp",
				interfaceCpp,
				serviceInterface
			);

			generateErrorEnumTypes(
				headerFileSystem,
				sourceFileSystem,
				dataTypePath,
				serviceInterface
				);
		}
	}

	def generateErrorEnumTypes(IFileSystemAccess headerFileSystem, IFileSystemAccess sourceFileSystem, String dataTypePath, FInterface fInterface)
	{
		var methodToErrorEnumName = fInterface.methodToErrorEnumName;
		for (method: getMethods(fInterface)) {
			var enumType = method.errors;
			if (enumType != null) {
				enumType.name = methodToErrorEnumName.get(method);
				val path = getPackagePathWithJoynrPrefix(enumType, File::separator)
				val headerFilename = path + File::separator + stdTypeUtil.getGenerationTypeName(enumType) + ".h"
				val sourceFilepath = dataTypePath + getPackageSourceDirectory(fInterface) + File::separator + fInterface.joynrName;
				val sourceFilename = sourceFilepath + File::separator + stdTypeUtil.getGenerationTypeName(enumType) + ".cpp"
				val headerFilenameQt = path + File::separator + qtTypeUtil.getGenerationTypeName(enumType) + ".h"

				generateEnum(
					headerFileSystem,
					sourceFileSystem,
					enumType,
					headerFilenameQt,
					headerFilename,
					sourceFilename
				)
			}
		}
	}

	def generateEnum (
		IFileSystemAccess headerFileSystem,
		IFileSystemAccess sourceFileSystem,
		FEnumerationType enumType,
		String headerFilenameQt,
		String headerFilename,
		String sourceFilename
	) {
		generateFile(
			headerFileSystem,
			headerFilenameQt,
			enumh,
			enumType
		)

		generateFile(
			headerFileSystem,
			headerFilename,
			stdEnumH,
			enumType
		)

		generateFile(
			sourceFileSystem,
			sourceFilename,
			stdEnumCpp,
			enumType
		)
	}
}
