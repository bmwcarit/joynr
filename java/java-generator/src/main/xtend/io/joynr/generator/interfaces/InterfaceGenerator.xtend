package io.joynr.generator.interfaces
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
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface
import io.joynr.generator.util.JoynrJavaGeneratorExtensions

class InterfaceGenerator {

	@Inject
	extension JoynrJavaGeneratorExtensions	

	@Inject
	InterfacesTemplate interfaces

	@Inject
	InterfaceSyncTemplate interfaceSync

	@Inject
	InterfaceAsyncTemplate interfaceAsync

	@Inject
	InterfaceSubscriptionTemplate interfaceSubscription

	@Inject
	InterfaceBroadcastTemplate interfaceBroadcast

	def doGenerate(FInterface serviceInterface, IFileSystemAccess fsa){

		val path = getPackagePathWithJoynrPrefix(serviceInterface, File::separator) + File::separator

		var serviceName =  serviceInterface.joynrName

		generateFile(
			fsa,
			path + serviceName + ".java",
			interfaces,
			serviceInterface
		);

		generateFile(
			fsa,
			path + serviceName + "Sync.java",
			interfaceSync,
			serviceInterface
		);

		generateFile(
			fsa,
			path + serviceName + "Async.java",
			interfaceAsync,
			serviceInterface
		);

		if (serviceInterface.attributes.size>0){
			generateFile(
				fsa,
				path + serviceName + "SubscriptionInterface.java",
				interfaceSubscription,
				serviceInterface
			);
		}
		
		if (serviceInterface.broadcasts.size>0){
			generateFile(
				fsa,
				path + serviceName + "BroadcastInterface.java",
				interfaceBroadcast,
				serviceInterface
			);
		}
	}
}