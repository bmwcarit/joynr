package io.joynr.generator.interfaces
/*
 * !!!
 *
 * Copyright (C) 2011 - 2020 BMW Car IT GmbH
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
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.util.JavaTemplateFactory
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface

class InterfaceGenerator {

	@Inject
	extension JoynrJavaGeneratorExtensions
	@Inject
	extension NamingUtil
	@Inject JavaTemplateFactory templateFactory

	@Inject
	@Named("generateProxyCode")
	public boolean generateProxyCode;

	def doGenerate(FInterface serviceInterface, IFileSystemAccess fsa){

		val path = getPackagePathWithJoynrPrefix(serviceInterface, File::separator) + File::separator

		var serviceName =  serviceInterface.joynrName

		var interfacesTemplate = templateFactory.createInterfacesTemplate(serviceInterface)
		generateFile(
			fsa,
			path + serviceName + ".java", interfacesTemplate
		);

		if (generateProxyCode) {
			var interfaceSyncTemplate = templateFactory.createInterfaceSyncTemplate(serviceInterface)
			generateFile(
				fsa,
				path + serviceName + "Sync.java",
				interfaceSyncTemplate
			);

			var interfaceAsyncTemplate = templateFactory.createInterfaceAsyncTemplate(serviceInterface)
			generateFile(
				fsa,
				path + serviceName + "Async.java",
				interfaceAsyncTemplate
			);

			var interfaceFireAndForgetTemplate = templateFactory.createInterfaceFireAndForgetTemplate(serviceInterface)
			generateFile(
				fsa,
				path + serviceName + "FireAndForget.java",
				interfaceFireAndForgetTemplate
			);

			var interfaceStatelessAsyncTemplate = templateFactory.createInterfaceStatelessAsyncTemplate(serviceInterface)
			generateFile(
				fsa,
				path + serviceName + "StatelessAsync.java",
				interfaceStatelessAsyncTemplate
			)

			var interfaceStatelessAsyncCallbackTemplate = templateFactory.createInterfaceStatelessAsyncCallbackTemplate(serviceInterface)
			generateFile(
				fsa,
				path + serviceName + "StatelessAsyncCallback.java",
				interfaceStatelessAsyncCallbackTemplate
			)

			if (serviceInterface.attributes.size>0){
				var interfaceSubscriptionTemplate = templateFactory.createInterfaceSubscriptionTemplate(serviceInterface)
				generateFile(
					fsa,
					path + serviceName + "SubscriptionInterface.java",
					interfaceSubscriptionTemplate
				);
			}
		}

		if (serviceInterface.broadcasts.size>0){
			var interfaceBroadcastTemplate = templateFactory.createInterfaceBroadcastTemplate(serviceInterface)
			generateFile(
				fsa,
				path + serviceName + "BroadcastInterface.java",
				interfaceBroadcastTemplate
			);
		}
	}
}
