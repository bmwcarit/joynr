package io.joynr.generator.cpp.proxy
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
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FModel

class ProxyGenerator {

	@Inject extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil

	@Inject
	IInterfaceConnectorHTemplate iInterfaceConnectorHTemplate

	@Inject
	InterfaceProxyBaseCppTemplate interfaceProxyBaseCpp

	@Inject
	InterfaceProxyBaseHTemplate interfaceProxyBaseH

	@Inject
	InterfaceProxyCppTemplate interfaceProxyCpp

	@Inject
	InterfaceProxyHTemplate interfaceProxyH

	@Inject
	InterfaceSyncProxyCppTemplate interfaceSyncProxyCpp

	@Inject
	InterfaceSyncProxyHTemplate interfaceSyncProxyH

	@Inject
	InterfaceAsyncProxyCppTemplate interfaceAsyncProxyCpp

	@Inject
	InterfaceAsyncProxyHTemplate interfaceAsyncProxyH

	def doGenerate(FModel model,
		IFileSystemAccess sourceFileSystem,
		IFileSystemAccess headerFileSystem,
		String sourceContainerPath,
		String headerContainerPath
	) {

		for(fInterface: model.interfaces){
			val sourcePath = sourceContainerPath + getPackageSourceDirectory(fInterface) + File::separator
			val headerPath = headerContainerPath + getPackagePathWithJoynrPrefix(fInterface, File::separator) + File::separator
			var serviceName = fInterface.joynrName

			generateFile(
				headerFileSystem,
				headerPath + "I" + serviceName + "Connector.h",
				iInterfaceConnectorHTemplate,
				fInterface
			);

			generateFile(
				headerFileSystem,
				headerPath + serviceName + "ProxyBase.h",
				interfaceProxyBaseH,
				fInterface
			);

			generateFile(
				sourceFileSystem,
				sourcePath + serviceName + "ProxyBase.cpp",
				interfaceProxyBaseCpp,
				fInterface
			);

			generateFile(
				headerFileSystem,
				headerPath + serviceName + "Proxy.h",
				interfaceProxyH,
				fInterface
			);

			generateFile(
				sourceFileSystem,
				sourcePath + serviceName + "Proxy.cpp",
				interfaceProxyCpp,
				fInterface
			);

			generateFile(
				headerFileSystem,
				headerPath + serviceName + "SyncProxy.h",
				interfaceSyncProxyH,
				fInterface
			);

			generateFile(
				sourceFileSystem,
				sourcePath + serviceName + "SyncProxy.cpp",
				interfaceSyncProxyCpp,
				fInterface
			);

			generateFile(
				headerFileSystem,
				headerPath + serviceName + "AsyncProxy.h",
				interfaceAsyncProxyH,
				fInterface
			);

			generateFile(
				sourceFileSystem,
				sourcePath + serviceName + "AsyncProxy.cpp",
				interfaceAsyncProxyCpp,
				fInterface
			);
		}
	}

}
