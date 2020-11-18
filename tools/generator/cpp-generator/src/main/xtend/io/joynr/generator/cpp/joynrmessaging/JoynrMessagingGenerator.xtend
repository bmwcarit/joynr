package io.joynr.generator.cpp.joynrmessaging
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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.templates.util.NamingUtil
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FModel
import io.joynr.generator.cpp.util.CppTemplateFactory

class JoynrMessagingGenerator {

	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension NamingUtil
	@Inject CppTemplateFactory templateFactory;

	def doGenerate(FModel model,
		IFileSystemAccess sourceFileSystem,
		IFileSystemAccess headerFileSystem,
		String sourceContainerPath,
		String headerContainerPath
	){

		for(serviceInterface: model.interfaces){
			val generateVersioning = !commentContainsNoVersionGeneration(serviceInterface)
			val sourcePath = sourceContainerPath + getPackageSourceDirectory(serviceInterface, generateVersioning) + File::separator
			val headerPath = headerContainerPath + getPackagePathWithJoynrPrefix(serviceInterface, File::separator, generateVersioning) + File::separator
			val serviceName = serviceInterface.joynrName

			var interfaceJoynrMessagingConnectorHTemplate = templateFactory.createInterfaceJoynrMessagingConnectorHTemplate(serviceInterface)
			generateFile(
				headerFileSystem,
				headerPath + serviceName + "JoynrMessagingConnector.h",
				interfaceJoynrMessagingConnectorHTemplate,
				generateVersioning
			);

			var interfaceJoynrMessagingConnectorCppTemplate = templateFactory.createInterfaceJoynrMessagingConnectorCppTemplate(serviceInterface)
			generateFile(
				sourceFileSystem,
				sourcePath + serviceName + "JoynrMessagingConnector.cpp",
				interfaceJoynrMessagingConnectorCppTemplate,
				generateVersioning
			);
		}
	}
}
