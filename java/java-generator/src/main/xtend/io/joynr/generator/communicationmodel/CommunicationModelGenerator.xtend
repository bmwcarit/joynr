package io.joynr.generator.communicationmodel
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
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FModel
import io.joynr.generator.util.JoynrJavaGeneratorExtensions

class CommunicationModelGenerator {
	
	@Inject
	extension JoynrJavaGeneratorExtensions
	
	@Inject
	EnumTemplate enumTemplate;
	
	@Inject
	ComplexTypeTemplate complexTypeTemplate;
	
	
	def doGenerate(FModel fModel, IFileSystemAccess fsa){
		for( type: getComplexDataTypes(fModel)){
			if(type instanceof FCompoundType) {
				val path = getPackagePathWithJoynrPrefix(type, File::separator) + File::separator
				
				fsa.generateFile(
					path + type.joynrName + ".java",
					complexTypeTemplate.generate(type).toString
				)
	
			}
		}
		
		for( type: getEnumDataTypes(fModel)){
			val path = getPackagePathWithJoynrPrefix(type, File::separator) + File::separator 
			if(type instanceof FEnumerationType) {
				fsa.generateFile(
					path + type.joynrName + ".java",
					enumTemplate.generate(type).toString
				)
			}
		}

	}
	


}