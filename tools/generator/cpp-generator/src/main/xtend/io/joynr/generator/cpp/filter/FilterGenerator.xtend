package io.joynr.generator.cpp.filter
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
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FModel

class FilterGenerator {

	@Inject
	extension JoynrCppGeneratorExtensions

	@Inject
	extension NamingUtil

	@Inject
	FilterParameterTemplate filterParameterTemplate;

	@Inject
	FilterTemplate filterTemplate;

	def doGenerate(FModel model,
		IFileSystemAccess sourceFileSystem,
		IFileSystemAccess headerFileSystem,
		String sourceContainerPath,
		String headerContainerPath
	) {
		for(fInterface: model.interfaces){
			val generateVersioning = !commentContainsNoVersionGeneration(fInterface)
			val headerPath = headerContainerPath + 
				getPackagePathWithJoynrPrefix(fInterface, File::separator, generateVersioning) + File::separator
			var serviceName = fInterface.joynrName

			for (broadcast : fInterface.broadcasts) {
				if (broadcast.selective) {
					val filterParameterLocation = getFilterParameterLocation(headerPath, serviceName, broadcast)

					generateFile(
						headerFileSystem,
						filterParameterLocation,
						filterParameterTemplate,
						fInterface,
						broadcast,
						generateVersioning);

					val filterLocation = getFilterLocation(headerPath, serviceName, broadcast)
					generateFile(
						headerFileSystem,
						filterLocation,
						filterTemplate,
						fInterface,
						broadcast,
						generateVersioning);
				}
			}
		}
	}

	def getFilterLocation(String headerPath, String serviceName, FBroadcast broadcast) {
		headerPath + serviceName.toFirstUpper + broadcast.name.toFirstUpper + "BroadcastFilter.h"
	}

	def getFilterParameterLocation(String headerPath, String serviceName, FBroadcast broadcast) {
		headerPath + serviceName.toFirstUpper + broadcast.name.toFirstUpper + "BroadcastFilterParameters.h"
	}
}
