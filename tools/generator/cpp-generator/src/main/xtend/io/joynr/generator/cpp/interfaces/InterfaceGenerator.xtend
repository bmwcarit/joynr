package io.joynr.generator.cpp.interfaces
/*
 * !!!
 *
 * Copyright (C) 2020 BMW Car IT GmbH
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
import com.google.inject.name.Named
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.CppTemplateFactory
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModel

class InterfaceGenerator {

	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension CppStdTypeUtil
	@Inject extension NamingUtil
	@Inject extension InterfaceUtil

	@Inject
	@Named(NamingUtil.JOYNR_GENERATOR_NOVERSIONGENERATION_COMMENT)
	public boolean versioningComment;

	@Inject
	@Named(NamingUtil.JOYNR_GENERATOR_PACKAGEWITHVERSION)
	public boolean packageWithVersion;

	@Inject CppTemplateFactory templateFactory;

	def doGenerate(FModel fModel,
		IFileSystemAccess sourceFileSystem,
		IFileSystemAccess headerFileSystem,
		String sourceContainerPath,
		String headerContainerPath
	){
		val dataTypePath = sourceContainerPath + "datatypes" + File::separator
		val interfacePath = sourceContainerPath + "interfaces" + File::separator
		val headerInterfacePath =
			if (sourceFileSystem == headerFileSystem)
				headerContainerPath + "interfaces" + File::separator
			else
				headerContainerPath

		for(serviceInterface: fModel.interfaces){
			val generateVersioning = if (versioningComment) !commentContainsNoVersionGeneration(serviceInterface) else packageWithVersion
			val sourcepath = interfacePath + getPackageSourceDirectory(serviceInterface, generateVersioning) + File::separator 
			val headerpath = headerInterfacePath + getPackagePathWithJoynrPrefix(serviceInterface, File::separator, generateVersioning) + File::separator 

			var interfaceHTemplate = templateFactory.createInterfaceHTemplate(serviceInterface)
			generateFile(
				headerFileSystem,
				headerpath + "I" + serviceInterface.joynrName + ".h",
				interfaceHTemplate,
				generateVersioning
			);

			var interfaceCppTemplate = templateFactory.createInterfaceCppTemplate(serviceInterface)
			generateFile(
				sourceFileSystem,
				sourcepath + "I" + serviceInterface.joynrName + ".cpp",
				interfaceCppTemplate,
				generateVersioning
			);

			generateErrorEnumTypes(
				headerFileSystem,
				sourceFileSystem,
				dataTypePath,
				serviceInterface,
				generateVersioning
				);
		}
	}

	def generateErrorEnumTypes(IFileSystemAccess headerFileSystem, IFileSystemAccess sourceFileSystem, String dataTypePath, FInterface fInterface, boolean generateVersioning)
	{
		var methodToErrorEnumName = fInterface.methodToErrorEnumName;
		for (method: getMethods(fInterface)) {
			var enumType = method.errors;
			if (enumType !== null) {
				enumType.name = methodToErrorEnumName.get(method);
				val path = getPackagePathWithJoynrPrefix(enumType, File::separator, generateVersioning)
				val headerFilename = path + File::separator + getGenerationTypeName(enumType)
				val sourceFilepath = dataTypePath + getPackageSourceDirectory(fInterface, generateVersioning) + File::separator + fInterface.joynrName;
				val sourceFilename = sourceFilepath + File::separator + getGenerationTypeName(enumType)

				generateEnum(
					headerFileSystem,
					sourceFileSystem,
					enumType,
					headerFilename,
					sourceFilename,
					generateVersioning
				)
			}
		}
	}

	def generateEnum (
		IFileSystemAccess headerFileSystem,
		IFileSystemAccess sourceFileSystem,
		FEnumerationType enumType,
		String headerFilename,
		String sourceFilename,
		boolean generateVersioning
	) {
		var enumHTemplate = templateFactory.createEnumHTemplate(enumType)
		generateFile(
			headerFileSystem,
			headerFilename + ".h",
			enumHTemplate,
			generateVersioning
		)

		var enumCppTemplate = templateFactory.createEnumCppTemplate(enumType)
		generateFile(
			sourceFileSystem,
			sourceFilename + ".cpp",
			enumCppTemplate,
			generateVersioning
		)
	}
}
