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
import io.joynr.generator.cpp.communicationmodel.serializer.EnumSerializerCppTemplate
import io.joynr.generator.cpp.communicationmodel.serializer.EnumSerializerHTemplate
import io.joynr.generator.cpp.communicationmodel.serializer.MapSerializerCppTemplate
import io.joynr.generator.cpp.communicationmodel.serializer.MapSerializerHTemplate
import io.joynr.generator.cpp.communicationmodel.serializer.TypeSerializerCppTemplate
import io.joynr.generator.cpp.communicationmodel.serializer.TypeSerializerHTemplate
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModel

class CommunicationModelGenerator {

	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension CppStdTypeUtil
	@Inject private extension NamingUtil
	@Inject private extension InterfaceUtil

	@Inject InterfaceHTemplate interfaceH;
	@Inject InterfaceCppTemplate interfaceCpp;

	@Inject StdEnumHTemplate stdEnumH;
	@Inject StdEnumCppTemplate stdEnumCpp;

	@Inject StdTypeHTemplate stdTypeH;
	@Inject StdTypeCppTemplate stdTypeCpp;

	@Inject MapHTemplate mapH;
	@Inject MapCppTemplate mapCpp;

	@Inject TypeSerializerHTemplate typeSerializerH;
	@Inject TypeSerializerCppTemplate typeSerializerCpp;
	@Inject MapSerializerHTemplate mapSerializerH;
	@Inject MapSerializerCppTemplate mapSerializerCpp;
	@Inject EnumSerializerHTemplate enumSerializerH;
	@Inject EnumSerializerCppTemplate enumSerializerCpp;

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
			if (type.isPartOfTypeCollection) {
				headerpath += type.typeCollectionName + File::separator
				sourcepath += type.typeCollectionName + File::separator
			}

			generateFile(
				headerFileSystem,
				headerpath + getGenerationTypeName(type) + ".h",
				stdTypeH,
				type
			)

			generateFile(
				sourceFileSystem,
				sourcepath + getGenerationTypeName(type) + ".cpp",
				stdTypeCpp,
				type
			)

			generateFile(
				headerFileSystem,
				headerpath + getGenerationTypeName(type) + "Serializer.h",
				typeSerializerH,
				type
			)

			generateFile(
				sourceFileSystem,
				sourcepath + getGenerationTypeName(type) + "Serializer.cpp",
				typeSerializerCpp,
				type
			)
		}

		for (type : getEnumDataTypes(fModel)) {
			var sourcepath = dataTypePath + getPackageSourceDirectory(type) + File::separator
			var headerpath = headerDataTypePath + getPackagePathWithJoynrPrefix(type, File::separator) + File::separator
			if (type.isPartOfTypeCollection) {
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
			if (type.isPartOfTypeCollection) {
				headerpath += type.typeCollectionName + File::separator
				sourcepath += type.typeCollectionName + File::separator
			}
			val headerFilename = headerpath + getGenerationTypeName(type)
			val sourceFilename = sourcepath + getGenerationTypeName(type)

			generateFile(
				headerFileSystem,
				headerFilename + ".h",
				mapH,
				type
			)

			generateFile(
				sourceFileSystem,
				sourceFilename + ".cpp",
				mapCpp,
				type
			)

			generateFile(
				headerFileSystem,
				headerFilename + "Serializer.h",
				mapSerializerH,
				type
			)
			generateFile(
				sourceFileSystem,
				sourceFilename + "Serializer.cpp",
				mapSerializerCpp,
				type
			)
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
				val headerFilename = path + File::separator + getGenerationTypeName(enumType)
				val sourceFilepath = dataTypePath + getPackageSourceDirectory(fInterface) + File::separator + fInterface.joynrName;
				val sourceFilename = sourceFilepath + File::separator + getGenerationTypeName(enumType)

				generateEnum(
					headerFileSystem,
					sourceFileSystem,
					enumType,
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
		String headerFilename,
		String sourceFilename
	) {
		generateFile(
			headerFileSystem,
			headerFilename + ".h",
			stdEnumH,
			enumType
		)

		generateFile(
			sourceFileSystem,
			sourceFilename + ".cpp",
			stdEnumCpp,
			enumType
		)

		generateFile(
			headerFileSystem,
			headerFilename + "Serializer.h",
			enumSerializerH,
			enumType
		)
		generateFile(
			sourceFileSystem,
			sourceFilename + "Serializer.cpp",
			enumSerializerCpp,
			enumType
		)

	}
}
