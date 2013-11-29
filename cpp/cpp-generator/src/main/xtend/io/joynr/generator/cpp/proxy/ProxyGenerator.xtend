package io.joynr.generator.cpp.proxy
/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FModel
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions

class ProxyGenerator {
	
	@Inject
	extension JoynrCppGeneratorExtensions	
	
	@Inject
	IInterfaceConnectorHTemplate iInterfaceConnectorHTemplate
	
	@Inject
	InterfaceProxyBaseCppTemplate interfaceProxyBaseCpp

	@Inject
	InterfaceProxyBaseHTemplate interfaceProxyBaseH
	
	@Inject
	InterfaceProxyCppTemplate interfaceProxyCpp;

	@Inject
	InterfaceProxyHTemplate interfaceProxyH;
	
	@Inject
	InterfaceSyncProxyCppTemplate interfaceSyncProxyCpp;

	@Inject
	InterfaceSyncProxyHTemplate interfaceSyncProxyH;

	@Inject
	InterfaceAsyncProxyCppTemplate interfaceAsyncProxyCpp;

	@Inject
	InterfaceAsyncProxyHTemplate interfaceAsyncProxyH;
	
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
						
			headerFileSystem.generateFile(
				headerPath + "I" + serviceName + "Connector.h",
				iInterfaceConnectorHTemplate.generate(fInterface).toString
			);
						
			headerFileSystem.generateFile(
				headerPath + serviceName + "ProxyBase.h",
				interfaceProxyBaseH.generate(fInterface).toString
			);			
			sourceFileSystem.generateFile(
				sourcePath + serviceName + "ProxyBase.cpp",
				interfaceProxyBaseCpp.generate(fInterface).toString
			);
			headerFileSystem.generateFile(
				headerPath + serviceName + "Proxy.h",
				interfaceProxyH.generate(fInterface).toString
			);
			sourceFileSystem.generateFile(
				sourcePath + serviceName + "Proxy.cpp",
				interfaceProxyCpp.generate(fInterface).toString
			);
			
			headerFileSystem.generateFile(
				headerPath + serviceName + "SyncProxy.h",
				interfaceSyncProxyH.generate(fInterface).toString
			);
	
			sourceFileSystem.generateFile(
				sourcePath + serviceName + "SyncProxy.cpp",
				interfaceSyncProxyCpp.generate(fInterface).toString
			);
	
			headerFileSystem.generateFile(
				headerPath + serviceName + "AsyncProxy.h",
				interfaceAsyncProxyH.generate(fInterface).toString
			);
	
			sourceFileSystem.generateFile(
				sourcePath + serviceName + "AsyncProxy.cpp",
				interfaceAsyncProxyCpp.generate(fInterface).toString
			);		
        }
	}
}
