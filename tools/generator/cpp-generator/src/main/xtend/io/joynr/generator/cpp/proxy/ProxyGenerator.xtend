package io.joynr.generator.cpp.proxy
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
import io.joynr.generator.templates.util.InterfaceUtil

class ProxyGenerator {

	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension NamingUtil
	@Inject CppTemplateFactory templateFactory;
	@Inject extension InterfaceUtil

	def doGenerate(FModel model,
		IFileSystemAccess sourceFileSystem,
		IFileSystemAccess headerFileSystem,
		String sourceContainerPath,
		String headerContainerPath
	) {

		for(fInterface: model.interfaces){
			val generateVersioning = !commentContainsNoVersionGeneration(fInterface)
			val sourcePath = sourceContainerPath + getPackageSourceDirectory(fInterface, generateVersioning) + File::separator
			val headerPath = headerContainerPath + getPackagePathWithJoynrPrefix(fInterface, File::separator, generateVersioning) + File::separator
			var serviceName = fInterface.joynrName

			var iInterfaceConnectorHTemplate = templateFactory.createIInterfaceConnectorHTemplate(fInterface)
			generateFile(
				headerFileSystem,
				headerPath + "I" + serviceName + "Connector.h",
				iInterfaceConnectorHTemplate,
				generateVersioning
			);

			var interfaceProxyBaseHTemplate = templateFactory.createInterfaceProxyBaseHTemplate(fInterface)
			generateFile(
				headerFileSystem,
				headerPath + serviceName + "ProxyBase.h",
				interfaceProxyBaseHTemplate,
				generateVersioning
			);

			var interfaceProxyBaseCppTemplate = templateFactory.createInterfaceProxyBaseCppTemplate(fInterface)
			generateFile(
				sourceFileSystem,
				sourcePath + serviceName + "ProxyBase.cpp",
				interfaceProxyBaseCppTemplate,
				generateVersioning
			);

			var interfaceProxyHTemplate = templateFactory.createInterfaceProxyHTemplate(fInterface)
			generateFile(
				headerFileSystem,
				headerPath + serviceName + "Proxy.h",
				interfaceProxyHTemplate,
				generateVersioning
			);

			var interfaceProxyCppTemplate = templateFactory.createInterfaceProxyCppTemplate(fInterface)
			generateFile(
				sourceFileSystem,
				sourcePath + serviceName + "Proxy.cpp",
				interfaceProxyCppTemplate,
				generateVersioning
			);

			var interfaceSyncProxyHTemplate = templateFactory.createInterfaceSyncProxyHTemplate(fInterface)
			generateFile(
				headerFileSystem,
				headerPath + serviceName + "SyncProxy.h",
				interfaceSyncProxyHTemplate,
				generateVersioning
			);

			var interfaceSyncProxyCppTemplate = templateFactory.createInterfaceSyncProxyCppTemplate(fInterface)
			generateFile(
				sourceFileSystem,
				sourcePath + serviceName + "SyncProxy.cpp",
				interfaceSyncProxyCppTemplate,
				generateVersioning
			);

			var interfaceAsyncProxyHTemplate = templateFactory.createInterfaceAsyncProxyHTemplate(fInterface)
			generateFile(
				headerFileSystem,
				headerPath + serviceName + "AsyncProxy.h",
				interfaceAsyncProxyHTemplate,
				generateVersioning
			);

			var interfaceAsyncProxyCppTemplate = templateFactory.createInterfaceAsyncProxyCppTemplate(fInterface)
			generateFile(
				sourceFileSystem,
				sourcePath + serviceName + "AsyncProxy.cpp",
				interfaceAsyncProxyCppTemplate,
				generateVersioning
			);

			if (hasFireAndForgetMethods(fInterface)) {
				var interfaceFireAndForgetProxyHTemplate = templateFactory.createInterfaceFireAndForgetProxyHTemplate(fInterface)
				generateFile(
					headerFileSystem,
					headerPath + serviceName + "FireAndForgetProxy.h",
					interfaceFireAndForgetProxyHTemplate,
					generateVersioning
				);

				var interfaceFireAndForgetProxyCppTemplate = templateFactory.createInterfaceFireAndForgetProxyCppTemplate(fInterface)
				generateFile(
					sourceFileSystem,
					sourcePath + serviceName + "FireAndForgetProxy.cpp",
					interfaceFireAndForgetProxyCppTemplate,
					generateVersioning
				);
			}
		}
	}

}
