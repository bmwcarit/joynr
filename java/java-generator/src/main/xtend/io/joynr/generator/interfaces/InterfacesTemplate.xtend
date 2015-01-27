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
import io.joynr.generator.util.TemplateBase
import java.util.ArrayList
import java.util.Collection
import org.franca.core.franca.FInterface
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.InterfaceTemplate

class InterfacesTemplate implements InterfaceTemplate{
	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase

	override generate(FInterface serviceInterface) {
		val interfaceName =  serviceInterface.joynrName
		val className = interfaceName
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")
		'''

		«warning()»   
package «packagePath»;  	

import java.util.List;

import com.fasterxml.jackson.core.type.TypeReference;
«FOR datatype: getRequiredIncludesFor(serviceInterface)»
	import «datatype»;
«ENDFOR»

//The current generator is not able to check wether some of the imports are acutally necessary for this specific interface.
//Therefore some imports migth be unused in this version of the interface.
//To prevent warnings @SuppressWarnings("unused") is being used. 
//To prevent warnings about an unnecessary SuppressWarnings we have to import something that is not used. (e.g. TreeSet)
import java.util.TreeSet;
@SuppressWarnings("unused")
public interface «className»  {
	public static String INTERFACE_NAME = "«getPackagePathWithoutJoynrPrefix(serviceInterface, "/")»/«interfaceName.toLowerCase»";

	«FOR type : filterTypesByToken(getAllTypes(serviceInterface))»
		public static class «if (type.typeName==null) "Typename not found" else getTokenTypeForArrayType(type.typeName)»Token extends TypeReference<«getTokenTypeForArrayType(type.typeName)»> {}	
		public static class List«getTokenTypeForArrayType(type.typeName)»Token extends TypeReference<List<«getTokenTypeForArrayType(type.typeName)»> > {}	
	«ENDFOR»

}
'''	
	}

	def filterTypesByToken(Collection<Object> objects) {
		val result = new ArrayList<Object>()
		val tokens = new ArrayList<String>();
		for (object: objects){
			if (object!=null && !tokens.contains(getTokenTypeForArrayType(object.typeName))){
				result.add(object)
				tokens.add(getTokenTypeForArrayType(object.typeName))
			}
		}
		return result;
	}
}