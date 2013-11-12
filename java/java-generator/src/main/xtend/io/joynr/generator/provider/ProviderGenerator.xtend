package io.joynr.generator.provider
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
import org.franca.core.franca.FInterface
import io.joynr.generator.util.JoynrJavaGeneratorExtensions

class ProviderGenerator {
	
	@Inject
	extension JoynrJavaGeneratorExtensions	
	
	@Inject
	InterfaceProviderTemplate interfaceProvider
	
	@Inject
	InterfaceProviderImplTemplate interfaceProviderImpl
	
	@Inject
	InterfaceAbstractProviderTemplate interfaceAbstractProvider


	def doGenerate(FInterface fInterface, IFileSystemAccess fsa){
		val path = getPackagePathWithJoynrPrefix(fInterface, File::separator) + File::separator 

		var serviceName =  fInterface.joynrName
					
		
		fsa.generateFile(
			path + serviceName + "Provider.java",
			interfaceProvider.generate(fInterface).toString
		);			
		
		fsa.generateFile(
		    path + "Default" + serviceName + "Provider.java",
			interfaceProviderImpl.generate(fInterface).toString
		);		
		
		fsa.generateFile(
			path + serviceName + "AbstractProvider.java",
			interfaceAbstractProvider.generate(fInterface).toString	
		);
	}
}