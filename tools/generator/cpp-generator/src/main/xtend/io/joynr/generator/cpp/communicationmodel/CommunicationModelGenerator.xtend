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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.templates.util.TypeUtil
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FModel

class CommunicationModelGenerator {

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension TypeUtil

	@Inject
	private extension NamingUtil

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

				if (type.isPartOfTypeCollection) {
					headerpath += type.typeCollectionName + File::separator
					sourcepath += type.typeCollectionName + File::separator
				}

				generateFile(
					headerFileSystem,
					headerpath + type.joynrNameQt + ".h",
					typeH,
					type
				)
				
				generateFile(
					sourceFileSystem,
					sourcepath + type.joynrNameQt + ".cpp",
					typeCpp,
					type
				)

				generateFile(
					headerFileSystem,
					headerpath + type.joynrName + ".h",
					stdTypeH,
					type
				)

				generateFile(
					sourceFileSystem,
					sourcepath + type.joynrName + ".cpp",
					stdTypeCpp,
					type
				)
			}
		}

		for (type : getEnumDataTypes(fModel)) {
			var sourcepath = dataTypePath + getPackageSourceDirectory(type) + File::separator
			var headerpath = headerDataTypePath + getPackagePathWithJoynrPrefix(type, File::separator) + File::separator
			if (type.isPartOfTypeCollection) {
				headerpath += type.typeCollectionName + File::separator
				sourcepath += type.typeCollectionName + File::separator
			}

			generateFile(
				headerFileSystem,
				headerpath + type.joynrNameQt + ".h",
				enumh,
				type as FEnumerationType
			)

			generateFile(
				headerFileSystem,
				headerpath + type.joynrName + ".h",
				stdEnumH,
				type as FEnumerationType
			)
			generateFile(
				sourceFileSystem,
				sourcepath + type.joynrName + ".cpp",
				stdEnumCpp,
				type as FEnumerationType
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
		}
		
	}
}